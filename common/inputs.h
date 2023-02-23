#include "hardware.h"

class managerInputs{

    enum inputType{
        NORMALLY_OPEN = 0,
        NORMALLY_CLOSED = 1,
    };


    enum inputState{
        STATE_OPEN = LOW,
        STATE_CLOSED = HIGH
    };


    struct inputPin{
        unsigned long timePreviousChange = 0; /* Time (millis) when the input state last changed.  Value is set to 0 when the state returns to its input type. Default 0.*/
        inputState state = STATE_OPEN; /* The state entered at timePreviousChange. Default STATE_OPEN.*/
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
        inputPin inputs[COUNT_PINS_IO_EXTENDER]; /* Input pins connected to the hardware.*/
        bool enabled = true; /* Indicates if the controller is enabled. Default true.*/
    };


    ioExtender inputControllers[COUNT_IO_EXTENDER];

    void (*ptrPublisherCallback)(void); //TODO: Determine correct signature
    void (*ptrFailureCallback)(void); //TODO: Determine correct signature


    /** Convert a bit to inputState */
    inputState bitToInputState(boolean value){

        if(value == LOW){
            return inputState::STATE_CLOSED;
        }else{
            return inputState::STATE_OPEN;
        }

    }


    /** Checks the pins on the input controller for changes */
    void readInputPins(ioExtender *inputController){
        readInputPins(inputController, false);
    }


    /** Checks the pins on the input controller for changes, optionally ignoring the debounce delay */
    void readInputPins(ioExtender *inputController, boolean ignoreDebounceDelay){

        #ifdef DEBUG
            #if DEBUG > 50
                if(ignoreDebounceDelay == true){
                    Serial.println("(readInputPins) IO Extender: 0x" + String(inputController->address, HEX) + " Ignoring debounce delay.");
                }
            #endif
        #endif

        uint16_t pinRead = 0;

        #if MODEL_IO_EXTENDER == ENUM_MODEL_IO_EXTENDER_PCA9555
            pinRead = inputController->hardware.read();
        #endif

        //Exit if the value returned from the controller is the same as the value that was previously read
        if(pinRead == inputController->previousRead){
            return;
        }

        //Process each pin on the specified IO extender
        for(int i = 0; i < COUNT_PINS_IO_EXTENDER; i++){

            inputState currentState = bitToInputState(bitRead(pinRead, i));

            //Check if the value returned in the read is the same as the last read
            if(inputController->inputs[i].state == currentState){

                #ifdef DEBUG
                    #if DEBUG > 500
                        Serial.println("(readInputPins) IO Extender: 0x" + String(inputController->address, HEX) + " Pin: " + String(i) + " input states match. Previous: " + String(ioExtender->inputs[i].state, HEX) + " Current: " + String(currentState, HEX));
                    #endif
                #endif

                continue;
            }

            //Values are different; Check if we are within the debounce delay and that the status isn't its normal state
            if((millis() - inputController->inputs[i].timePreviousChange < DEBOUNCE_DELAY) && (inputController->inputs[i].type != currentState)){ // TO DO: FIX THIS

                //Check if the debounce delay should be checked (ignored on startup)
                if(ignoreDebounceDelay == false){
                    
                    #ifdef DEBUG
                        #if DEBUG > 50
                            Serial.println("(readInputPins) IO Extender: 0x" + String(inputController->address, HEX) + " Pin: " + String(i) + " DEBOUNCE_DELAY (" + String(DEBOUNCE_DELAY) + ") not satisfied. Time Previous Change: " + String(inputController->inputs[i].timePreviousChange) + " Current Time: " + String(millis()) + " Difference: " + String(millis() - inputController->inputs[i].timePreviousChange));
                        #endif
                    #endif

                    continue;
                }

            }

            switch(inputController->inputs[i].type){

                case inputType::NORMALLY_OPEN:

                    //Check if input is in an abnormal state
                    if(currentState == inputState::STATE_CLOSED){

                        inputController->inputs[i].timePreviousChange = millis();

                        #ifdef DEBUG
                            #if DEBUG > 50
                                Serial.println("(readInputPins) IO Extender: 0x" + String(inputController->address, HEX) + " Pin: " + String(i) + " Type: " + String(inputController->inputs[i].type) + " New State: " + String(currentState) + " (Abnormal)");
                            #endif
                        #endif

                        if(this->ptrPublisherCallback){
                            this->ptrPublisherCallback();
                        }

                        break;
                    }

                    //Check if input is in normal state
                    if(currentState == inputState::STATE_OPEN){
                    
                        inputController->inputs[i].timePreviousChange = 0;

                        #ifdef DEBUG
                            #if DEBUG > 50
                                Serial.println("(readInputPins) IO Extender: 0x" + String(inputController->address, HEX) + " Pin: " + String(i) + " Type: " + String(inputController->inputs[i].type) + " New State: " + String(currentState) + " (Normal)");
                            #endif
                        #endif
                        
                        //TODO: Something
                        break;
                    }

                case inputType::NORMALLY_CLOSED:

                    //Check if input is in an abnormal state
                    if(currentState == inputState::STATE_OPEN){

                        inputController->inputs[i].timePreviousChange = millis();
                        
                        #ifdef DEBUG
                            #if DEBUG > 50
                                Serial.println("(readInputPins) IO Extender: " + String(inputController->address, HEX) + " Pin: " + String(i) + " Type: " + String(inputController->inputs[i].type) + " New State: " + String(currentState) + " (Abnormal)");
                            #endif
                        #endif

                        if(this->ptrPublisherCallback){
                            this->ptrPublisherCallback();
                        }

                        break;
                    }

                    //Check if input is in normal state
                    if(currentState == inputState::STATE_CLOSED){
                    
                        inputController->inputs[i].timePreviousChange = 0;

                        #ifdef DEBUG
                            #if DEBUG > 50
                                Serial.println("(readInputPins) IO Extender: " + String(inputController->address, HEX) + " Pin: " + String(i) + " Type: " + String(inputController->inputs[i].type) + " New State: " + String(currentState) + " (Normal)");
                            #endif
                        #endif

                        //TODO: Something
                        break;
                    }

            }

            //Set the value of the pin to the current state
            inputController->inputs[i].state = currentState;

        }

        //Set the controller's value to the updated value
        inputController->previousRead = pinRead;
    }


    public:

        void setCallback_publisher(void (*userDefinedCallback)(void)) {
            ptrPublisherCallback = userDefinedCallback; }

        void setCallback_failure(void (*userDefinedCallback)(void)) {
            ptrFailureCallback = userDefinedCallback; }

        void begin(){

            const uint8_t pinsInterruptIoExtender[] = PINS_INTERRUPT_IO_EXTENDER;
            const uint8_t addressesIoExtender[] = ADDRESSES_IO_EXTENDER;

            //Setup the input controllers
            for(int i = 0; i < COUNT_IO_EXTENDER; i++){

                pinMode(this->inputControllers[i].interruptPin, INPUT);

                this->inputControllers[i].interruptPin = pinsInterruptIoExtender[i];
                this->inputControllers[i].address = addressesIoExtender[i];
                
                #if MODEL_IO_EXTENDER == ENUM_MODEL_IO_EXTENDER_PCA9555
                    this->inputControllers[i].hardware.attach(Wire, inputControllers[i].address);
                    this->inputControllers[i].hardware.polarity(PCA95x5::Polarity::INVERTED_ALL);
                    this->inputControllers[i].hardware.direction(PCA95x5::Direction::IN_ALL);

                    //Ensure we connected to the controller
                    if(this->inputControllers[i].hardware.i2c_error() != 0){
                        this->inputControllers[i].enabled = false;

                        if(this->ptrFailureCallback){
                            this->ptrFailureCallback();
                        }
                    }

                #endif

                //Get the current input states and ignore the debounce delays
                this->readInputPins(&this->inputControllers[i], true);
            }

        };

        void loop(){

            for(int i = 0; i < COUNT_IO_EXTENDER; i++){

                //Ignore disabled input controllers
                if(this->inputControllers[i].enabled == false){
                    continue;
                }
                
                //Need to read each input pin for LOW so we can detect intra-IO extender button press changes
                if(digitalRead(this->inputControllers[i].interruptPin) == LOW){
                    readInputPins(&this->inputControllers[i]);
                }
            }
        };
};