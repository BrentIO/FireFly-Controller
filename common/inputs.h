#include "hardware.h"

class managerInputs{

    public:

        /** Collection of port and channel that is unique per controller. */
        struct portChannel{
            uint8_t port; /* RJ45 port */
            uint8_t channel; /* Wire within the RJ45 port */
        };


        /* Failure reason codes, roughly based on i2c return messages. */
        enum failureReason{
            SUCCESS_NO_ERROR = 0, 
            DATA_TRANSMIT_BUFFER_ERROR = 1,
            ADDRESS_OFFLINE = 2,
            TRANSMIT_NOT_ACKNOLWEDGED = 3,
            OTHER_ERROR = 4,
            TIMEOUT = 5,
            INVALID_HARDWARE_CONFIGURATION = 10,
            UNKNOWN_ERROR = 11
        };


    private:

        enum inputType{
            NORMALLY_OPEN = 0,
            NORMALLY_CLOSED = 1,
        };


        enum inputState{
            STATE_OPEN = LOW,
            STATE_CLOSED = HIGH
        };


        struct inputPin{
            int64_t timeChange = 0; /* Time (milliseconds) when the input state last changed.  Value is set to 0 when the state returns to its input type. Default 0.*/
            boolean changeHandled = true; /* If the change has been handled, to make the event singular. Default true. */
            boolean changeHandledLong = true; /* If the long change has been handled, to make the event singular. Default true. */
            inputState state = STATE_OPEN; /* The state entered at timeChange. Default STATE_OPEN.*/
            inputType type = NORMALLY_OPEN; /* Defines if the input is normally open or normally closed. Default NORMALLY_OPEN.*/
            boolean monitorLongChange = false; /* Defines if the pin should be monitored for long changes.  Should be _true_ for buttons and _false_ for reed switches. Default false.*/

        };


        struct ioExtender{
            #if MODEL_IO_EXTENDER == ENUM_MODEL_IO_EXTENDER_PCA9995
                PCA9555 hardware; /* Reference to the hardware. */
            #endif

            uint8_t interruptPin = 0; /* Interrupt pin. Default 0. */
            uint8_t address = 0; /* I2C address. Default 0.*/
            uint16_t previousRead = 0; /* Numeric value of the last read from the hardware. Default 0.*/
            inputPin inputs[COUNT_PINS_IO_EXTENDER]; /* Input pins connected to the hardware extender.*/
            bool enabled = true; /* Indicates if the controller is enabled. Default true.*/
            uint8_t portOffset = 0; /* Indicates the first RJ45 port handled by this ioExtender.*/
        };


        ioExtender inputControllers[COUNT_IO_EXTENDER];
        const uint8_t portChannelPinMap[COUNT_PINS_IO_EXTENDER][2] = IO_EXTENDER_CHANNELS;

        void (*ptrPublisherCallback)(portChannel, boolean);
        void (*ptrFailureCallback)(uint8_t, failureReason);

        /** Marks a given input controller as failed and raises a failed status */
        void failInputController(ioExtender *inputController, failureReason failureReason){

            if(inputController->enabled == false){
                return;
            }

            if(failureReason == failureReason::SUCCESS_NO_ERROR){
                #if DEBUG
                    Serial.println(F("[inputs] (failInputController) Function is being called with no error present."));
                #endif

                return;
            }

            inputController->enabled = false;

            if(this->ptrFailureCallback){
                this->ptrFailureCallback(inputController->address, failureReason);
            }
        }


        /** Convert a bit to inputState */
        inputState bitToInputState(boolean value){
            if(value == LOW){
                return inputState::STATE_CLOSED;
            }else{
                return inputState::STATE_OPEN;
            }
        }


        failureReason i2cResponseToFailureReason(uint8_t i2cError){

            switch(i2cError){

                case 0:
                    return failureReason::SUCCESS_NO_ERROR;
                    break;

                case 1:
                    return failureReason::DATA_TRANSMIT_BUFFER_ERROR;
                    break;

                case 2:
                    return failureReason::ADDRESS_OFFLINE;
                    break;

                case 3:
                    return failureReason::TRANSMIT_NOT_ACKNOLWEDGED;
                    break;

                case 4:
                    return failureReason::OTHER_ERROR;
                    break;

                case 5:
                    return failureReason::TIMEOUT;
                    break;

                case 10:
                    return failureReason::INVALID_HARDWARE_CONFIGURATION;
                    break;

                default:
                    return failureReason::UNKNOWN_ERROR;
            }
        }


        /** Checks the pins on the input controller for changes */
        void readInputPins(ioExtender *inputController){
            uint16_t pinRead = 0;

            //Read all of the pins in a single call to the hardware
            #if MODEL_IO_EXTENDER == ENUM_MODEL_IO_EXTENDER_PCA9555
                pinRead = inputController->hardware.read();

                if(inputController->hardware.i2c_error() != 0){
                    failInputController(inputController, i2cResponseToFailureReason(inputController->hardware.i2c_error()));
                    return;
                }

            #endif

            //Exit if the value returned from the controller is the same as the value that was previously read
            if(pinRead == inputController->previousRead){
                return;
            }

            //Set the controller's value to the updated value
            inputController->previousRead = pinRead;

            //Hardware detected a change; Process each pin on the specified IO extender
            for(int i = 0; i < COUNT_PINS_IO_EXTENDER; i++){

                inputState currentState = bitToInputState(bitRead(pinRead, i));

                //Check if the value returned in the read is the same as the last read
                if(inputController->inputs[i].state == currentState){
                    continue;
                }

                portChannel portChannel;

                portChannel.port = portChannelPinMap[i][0] + inputController->portOffset;
                portChannel.channel = portChannelPinMap[i][1];

                switch(inputController->inputs[i].type){

                    case inputType::NORMALLY_OPEN:

                        //Check if input is in an abnormal state
                        if(currentState == inputState::STATE_CLOSED){

                            inputController->inputs[i].timeChange = int(esp_timer_get_time()/1000);
                            inputController->inputs[i].changeHandled = false;
                            inputController->inputs[i].changeHandledLong = false;

                            break;
                        }

                        //Check if input is in normal state
                        if(currentState == inputState::STATE_OPEN){
                        
                            inputController->inputs[i].timeChange = 0;
                            inputController->inputs[i].changeHandled = true;
                            inputController->inputs[i].changeHandledLong = true;
                        
                            break;
                        }

                    case inputType::NORMALLY_CLOSED:

                        //Check if input is in an abnormal state
                        if(currentState == inputState::STATE_OPEN){

                            inputController->inputs[i].timeChange = int(esp_timer_get_time()/1000);
                            inputController->inputs[i].changeHandled = false;
                            inputController->inputs[i].changeHandledLong = false;
                            
                            break;
                        }

                        //Check if input is in normal state
                        if(currentState == inputState::STATE_CLOSED){
                        
                            inputController->inputs[i].timeChange = 0;
                            inputController->inputs[i].changeHandled = true;
                            inputController->inputs[i].changeHandledLong = true;

                            break;
                        }
                }

                //Set the value of the pin to the current state
                inputController->inputs[i].state = currentState;
            }
        }


        void processInputs(ioExtender *inputController){

            for(int i = 0; i < COUNT_PINS_IO_EXTENDER; i++){

                if(inputController->inputs[i].timeChange == 0){
                    continue;
                }

                if((int(esp_timer_get_time()/1000) - inputController->inputs[i].timeChange) < MINIMUM_CHANGE_DELAY){

                    //Change was not observed long enough yet
                    continue;
                }

                portChannel portChannel;

                portChannel.port = portChannelPinMap[i][0] + inputController->portOffset;
                portChannel.channel = portChannelPinMap[i][1];

                if(((int(esp_timer_get_time()/1000) - inputController->inputs[i].timeChange) > MINIMUM_CHANGE_DELAY) && ((int(esp_timer_get_time()/1000) - inputController->inputs[i].timeChange) < MINIMUM_LONG_CHANGE_DELAY)){

                    if(inputController->inputs[i].changeHandled == true){
                        continue;
                    }

                    inputController->inputs[i].changeHandled = true;

                    //Raise  an event for a short change
                    if(this->ptrPublisherCallback){
                        this->ptrPublisherCallback(portChannel, false);
                    }

                    continue;
                }

                if((int(esp_timer_get_time()/1000) - inputController->inputs[i].timeChange) > MINIMUM_LONG_CHANGE_DELAY){

                    if(inputController->inputs[i].monitorLongChange == false){
                        continue;
                    }

                    if(inputController->inputs[i].changeHandledLong == true){
                        continue;
                    }

                    inputController->inputs[i].changeHandledLong = true;

                    //Raise  an event for a long change
                    if(this->ptrPublisherCallback){
                        this->ptrPublisherCallback(portChannel, true);
                    }
                }
            }
        }


        bool _initialized = false; /* If the class has been initialized. */


    public:

        struct healthResult{
            uint8_t count = 0;
            structHealth inputControllers[COUNT_IO_EXTENDER];
        };


        void setCallback_publisher(void (*userDefinedCallback)(portChannel, boolean)) {
            ptrPublisherCallback = userDefinedCallback; }


        void setCallback_failure(void (*userDefinedCallback)(uint8_t, failureReason)) {
            ptrFailureCallback = userDefinedCallback; }


        void begin(){

            if(this->_initialized == true){
                return;
            }

            const uint8_t pinsInterruptIoExtender[] = PINS_INTERRUPT_IO_EXTENDER;
            const uint8_t addressesIoExtender[] = ADDRESSES_IO_EXTENDER;

            if(COUNT_IO_EXTENDER != sizeof(pinsInterruptIoExtender)/sizeof(uint8_t)){

                #if DEBUG
                    Serial.println(F("[inputs] (begin) COUNT_IO_EXTENDER and the length of PINS_INTERRUPT_IO_EXTENDER are mismatched in hardware.h; Disabling inputs."));
                #endif

                ioExtender invalid;
                invalid.address = 0;

                failInputController(&invalid, failureReason::INVALID_HARDWARE_CONFIGURATION);

                return;

            }

            if(COUNT_IO_EXTENDER != sizeof(addressesIoExtender)/sizeof(uint8_t)){

                #if DEBUG
                    Serial.println(F("[inputs] (begin) COUNT_IO_EXTENDER and the length of ADDRESSES_IO_EXTENDER are mismatched in hardware.h; Disabling inputs."));
                #endif

                ioExtender invalid;
                invalid.address = 0;

                failInputController(&invalid, failureReason::INVALID_HARDWARE_CONFIGURATION);

                return;

            }

            //Setup the input controllers
            for(int i = 0; i < COUNT_IO_EXTENDER; i++){

                this->inputControllers[i].interruptPin = pinsInterruptIoExtender[i];
                this->inputControllers[i].address = addressesIoExtender[i];
                this->inputControllers[i].portOffset = ((COUNT_PINS_IO_EXTENDER / COUNT_CHANNELS_PER_PORT) * i);

                pinMode(this->inputControllers[i].interruptPin, INPUT);
                
                #if MODEL_IO_EXTENDER == ENUM_MODEL_IO_EXTENDER_PCA9555
                    this->inputControllers[i].hardware.attach(Wire, inputControllers[i].address);
                    this->inputControllers[i].hardware.polarity(PCA95x5::Polarity::ORIGINAL_ALL);
                    this->inputControllers[i].hardware.direction(PCA95x5::Direction::IN_ALL);

                    //Ensure we connected to the controller
                    if(this->inputControllers[i].hardware.i2c_error() != 0){
                        failInputController(&this->inputControllers[i], i2cResponseToFailureReason(this->inputControllers[i].hardware.i2c_error()));
                    }

                #endif

                //Get the current input states
                this->readInputPins(&this->inputControllers[i]);

            }

            this->_initialized = true;

        };

        /** Returns the value of each input controller's bus status */
        healthResult health(){

            healthResult returnValue;

            if(this->_initialized != true){
                return returnValue;
            }

            for(int i = 0; i < COUNT_IO_EXTENDER; i++){
                returnValue.inputControllers[i].address = this->inputControllers[i].address;
                returnValue.inputControllers[i].enabled = this->inputControllers[i].enabled;
            }

            returnValue.count = COUNT_IO_EXTENDER;

            return returnValue;
        }

        void loop(){

            if(this->_initialized != true){
                return;
            }

            for(int i = 0; i < COUNT_IO_EXTENDER; i++){

                //Ignore disabled input controllers
                if(this->inputControllers[i].enabled == false){
                    continue;
                }
                
                //Need to read each input pin for LOW so we can detect intra-IO extender button press changes
                if(digitalRead(this->inputControllers[i].interruptPin) == LOW){
                    readInputPins(&this->inputControllers[i]);
                }

                //Read the inputs and see if there are any that have an unhandled change
                processInputs(&this->inputControllers[i]);
            }
        };
};