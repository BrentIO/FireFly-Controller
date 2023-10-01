#include "hardware.h"


/** Input Manager
 * 
 * Configures and monitors the main inputs on the system using the IO Extenders.
 * 
 * ### Usage
 *  Minimum usage includes `begin()`, which should be placed in the main `setup()` function, and `loop()`, which should be placed in the main `loop()` function.
 * 
 * 
 * ### Callbacks
 *  Two callback functions are supported:
 * - `setCallback_publisher` which will be called when an input changes from its normal status to an abnormal status
 * - `setCallback_failure` which will be called if there is an error during `begin()` or if one of the input controllers falls off the bus after being initialized
 */
class managerInputs{

    /**
     * Constants are defined in hardware.h
     * 
     * Definitions:
     * 
     *  -   Pin: Represents the input controller's physical pin connections to the motherboard
     * 
     *  -   Input Controller: Senses the state changes on a given pin.  When a state change is detected, the Input Controller makes the interrupt pin HIGH
     * 
     *  -   Interrupt Pin: The pin being monitored by the CPU for changes being reported by the Input Controller
     * 
     *  -   Port: RJ-45 port on the device, which is (typically) comprised of 4 input channels
     * 
     *  -   Channel: The wire within the RJ-45 port that has the ability to raise a signal high or low.  It is connected to a Pin on an Input Controller
     * 
    */

    public:

        /** Collection of port and channel that is unique */
        struct portChannel{
            uint8_t port; /* RJ-45 port */
            uint8_t channel; /* Wire within the RJ-45 port */
        };


        /** Failure reason codes, roughly based on i2c return messages */
        enum failureReason{
            /// @brief Request was successful, no error was returned
            SUCCESS_NO_ERROR = 0, 

            /// @brief Data too long to fit in transmit buffer
            DATA_TRANSMIT_BUFFER_ERROR = 1,

            /// @brief Received NACK on transmit of address
            ADDRESS_OFFLINE = 2,

            /// @brief Received NACK on transmit of data
            TRANSMIT_NOT_ACKNOLWEDGED = 3,

            /// @brief Other error
            OTHER_ERROR = 4,

            /// @brief Timeout
            TIMEOUT = 5,

            /// @brief Invalid hardware configuration
            INVALID_HARDWARE_CONFIGURATION = 10,

            /// @brief Unknown/undocumented failure
            UNKNOWN_ERROR = 11
        };


    private:

        /** Input types */
        enum inputType{
            /// @brief Input is normally open (typically a button)
            NORMALLY_OPEN = 0,

            /// @brief Input is normally closed (typically a reed switch)
            NORMALLY_CLOSED = 1,
        };


        /** Input states that can be observed */
        enum inputState{

            /// @brief Input state is open/low.  This is not an indication of an abnormal state
            STATE_OPEN = LOW,

            /// @brief Input state is closed/high.  This is not an indication of a normal state
            STATE_CLOSED = HIGH
        };


        /** Input pin represents the individual input on the ioExtender.  Each pin on the ioExtender chip will have an inputPin associated.  
         * 
         * It contains current information, such as the time of last change, if the change has been handled (for both short and long changes), input type, and the current input state.
         * The input pin can also be configured to monitor for long changes. 
         * */
        struct inputPin{
            int64_t timeChange = 0; /* Time (milliseconds) when the input state last changed.  Value is set to 0 when the state returns to its input type. Default 0.*/
            boolean changeHandled = true; /* If the change has been handled, to make the event singular. Default true. */
            boolean changeHandledLong = true; /* If the long change has been handled, to make the event singular. Default true. */
            inputState state = STATE_OPEN; /* The state entered at timeChange. Default STATE_OPEN.*/
            inputType type = NORMALLY_OPEN; /* Defines if the input is normally open or normally closed. Default NORMALLY_OPEN.*/
            boolean monitorLongChange = false; /* Defines if the pin should be monitored for long changes.  Should be _true_ for buttons and _false_ for reed switches. Default false.*/
        };


        /** ioExtender represents a physical integrated circuit chip installed on the hardware.
         * 
         * Each ioExtender is configured with an i2c address and interrupt pin.
         * 
         * It contains current information, such as the previous read value from the chip, if it is enabled or disabled, as well as contains a collection if inputPins.
        */
        struct ioExtender{
            #if MODEL_IO_EXTENDER == ENUM_MODEL_IO_EXTENDER_PCA9995
                PCA9555 hardware; /* Reference to the hardware. */
            #endif

            uint8_t interruptPin = 0; /* Interrupt pin. Default 0. */
            uint8_t address = 0; /* I2C address. Default 0.*/
            uint16_t previousRead = 0; /* Numeric value of the last read from the hardware. Default 0.*/
            inputPin inputs[COUNT_PINS_IO_EXTENDER]; /* Input pins connected to the hardware extender.*/
            bool enabled = true; /* Indicates if the controller is enabled. Default true.*/
            uint8_t portOffset = 0; /* Indicates the first RJ-45 port handled by this ioExtender.*/
        };


        /** Collection of input controllers (ioExtnder) attached to the motherboard */
        ioExtender inputControllers[COUNT_IO_EXTENDER];


        /** Defines the port and channel for a given input controller */
        const uint8_t portChannelPinMap[COUNT_PINS_IO_EXTENDER][2] = IO_EXTENDER_CHANNELS;


        /** Reference to the callback function that will be called when an input is sensed */
        void (*ptrPublisherCallback)(portChannel, boolean);


        /** Reference to the callback function that will be called when an input controller has failed */
        void (*ptrFailureCallback)(uint8_t, failureReason);


        /** Marks a given input controller as failed and raises a callback event, if configured 
         * @param inputController The input controller being reported
         * @param failureReason Reason code for the failure
        */
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


        /** Convert a bit to inputState 
         * @param value As the value read from the hardware, where LOW/0 = CLOSED, and HIGH/1 = OPEN
        */
        inputState bitToInputState(boolean value){
            if(value == LOW){
                return inputState::STATE_CLOSED;
            }else{
                return inputState::STATE_OPEN;
            }
        }


        /** Enumerates the i2c bus failure codes to a failureReason 
         * @param i2cError The value returned from the i2c wire endTransmission() function
         * @returns A failureReason enumeration mapped to the error code passed in
        */
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

        /** Object containing the count of controllers and a list of each controller's bus status */
        struct healthResult{
            uint8_t count = 0; /** The number of input controllers */
            structHealth inputControllers[COUNT_IO_EXTENDER]; /** Array of input controller health */
        };

        /** Callback function that is called when an input is made */
        void setCallback_publisher(void (*userDefinedCallback)(portChannel, boolean)) {
            ptrPublisherCallback = userDefinedCallback; }


        /** Callback function that is called when an input controller failure occurs */
        void setCallback_failure(void (*userDefinedCallback)(uint8_t, failureReason)) {
            ptrFailureCallback = userDefinedCallback; }


        /** Initializes all input controllers. If unsuccessful, the failure callback will be called */
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

        /** Get the health of the input controllers 
         * @returns All input controllers and their bus state; the count of input controllers
        */
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

        /** Observes changes in inputs with each main loop() cycle */
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