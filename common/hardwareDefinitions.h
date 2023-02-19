/*
    Hardware Definitions
*/

#ifndef hardware_definitions_h
    #define hardware_definitions_h

    /* Define hardware enumeration constants */
    #define ENUM_MODEL_IO_EXTENDER_PCA9995 0 /* PCA9555 */
    #define ENUM_MODEL_OUTPUT_CONTROLLER_PCA9685 0 /* PCA9685 */
    #define ENUM_MODEL_EEPROM_EXTERNAL_24LCXXX 0 /* 24LC024 + Family */
    #define ENUM_MODEL_TEMPERATURE_SENSOR_PCT2075 0 /* PCT2075 */

    /* Hardware Types */
    #if PRODUCT_ID == 32322211

        #define SUPPORTED_HARDWARE
       
        /* Input IO Extender */
        #define COUNT_IO_EXTENDER 8 /* The number of IO extenders. */
        #define MODEL_IO_EXTENDER ENUM_MODEL_IO_EXTENDER_PCA9995 /* IO Extender Model. */
        #define COUNT_PINS_IO_EXTENDER 16 /* The number of pins on each IO extender. */
        
        /* Output Controller */
        #define COUNT_OUTPUT_CONTROLLER 2 /* The number of output controllers. */
        #define MODEL_OUTPUT_CONTROLLER ENUM_MODEL_OUTPUT_CONTROLLER_PCA9685
        #define COUNT_PINS_OUTPUT_CONTROLLER 16 /* The number of pins on each output controller. */

        /* External EEPROM */
        #define MODEL_EEPROM_EXTERNAL ENUM_MODEL_EEPROM_EXTERNAL_24LCXXX
        #define SIZE_EEPROM 256 /* Six of the external EEPROM.  NOTE: Divide the device size in Kbits by 8. */

        /* Temperature Sensor */
        #define COUNT_TEMPERATURE_SENSOR 1 /* The number of temperature sensors. */
        #define MODEL_TEMPERATURE_SENSOR ENUM_MODEL_TEMPERATURE_SENSOR_PCT2075
        #define MILLS_TEMPERATURE_SLEEP_DURATION 5000 /* Number of millis to wait between reading the temperatures. Default 500. */
        #define DEGREES_TEMPERATURE_VARIATION_ALLOWED 0.25 /* Number of degrees allowed variance between temperature reads before the new value is stored and reported. Default 0.1. */
        
        /* Pin Addresses */
        #define PIN_EEPROM_WP 23 /* EEPROM write protect pin.  When low, write protect is disabled. */
        #define PIN_ETHERNET 0 /* Ethernet hardware control flow pin. */
        #define PIN_OLED_BUTTON 32 /* Button for the front panel. */
        #define PIN_OLED_LED 33 /* LED for the front panel button. */
        #define _PINS_INTERRUPT_IO_EXTENDER {35,25,26,27,4,5,18,19} /* Interrupt pins for the IO extenders, order must match the _ADDRESSES_IO_EXTENDER. */

        /* I2C Addresses */
        #define _ADDRESSES_IO_EXTENDER {0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27} /* I2C addresses for the IO extenders, order must match the _PINS_INTERRUPT_IO_EXTENDER. */
        #define _ADDRESSES_OUTPUT_CONTROLLER {0x40,0x42} /* I2C addresses for the output controllers. */
        #define _ADDRESSES_TEMPERATURE_SENSOR {0x48} /* I2C addresses of the on-board temperature sensors. */
        #define ADDRESS_EEPROM 0x50 /* I2C addresses of the external EEPROM. */
        #define ADDRESS_OLED 0x3C /* I2C addresses of the OLED Display.  NOTE: This can be modifed on the display itself, default is 0x3C. */

    #endif


    #ifndef SUPPORTED_HARDWARE

        #error Build failed, Unknown PRODUCT_ID. Ensure it was set in ./.vscode/arduino.json {"buildPreferences":[["build.extra_flags","-DPRODUCT_ID=000000000"]]}

    #endif


    const uint8_t PINS_IO_EXTENDER[] = _PINS_INTERRUPT_IO_EXTENDER;
    const uint8_t ADDRESSES_IO_EXTENDER[] = _ADDRESSES_IO_EXTENDER;
    const uint8_t ADDRESSES_OUTPUT_CONTROLLER[] = _ADDRESSES_OUTPUT_CONTROLLER;
    const uint8_t ADDRESSES_TEMPERATURE_SENSORS[] = _ADDRESSES_TEMPERATURE_SENSOR;


    /* Include hardware-specific libraries */
    #if MODEL_IO_EXTENDER == ENUM_MODEL_IO_EXTENDER_PCA9995
        #include <PCA95x5.h> // https://github.com/semcneil/PCA95x5

        #define DEBOUNCE_DELAY 500 /* Milliseconds between changes for debouncing. */

    #endif


    #if MODEL_OUTPUT_CONTROLLER == ENUM_MODEL_OUTPUT_CONTROLLER_PCA9685
        #include <PCA9685.h> // https://github.com/RobTillaart/PCA9685_RT

        #define MAXIMUM_PWM 4095 /* Maximum value for PWM */
        #define FREQUENCY_PWM 1500 /* PWM Frequency in Hz */
    #endif


    #if MODEL_EEPROM_EXTERNAL == ENUM_MODEL_EEPROM_EXTERNAL_24LCXXX
        #include <I2C_eeprom.h> // https://github.com/RobTillaart/I2C_EEPROM
    #endif


    #if MODEL_TEMPERATURE_SENSOR == ENUM_MODEL_TEMPERATURE_SENSOR_PCT2075
        #include <PCT2075.h> // https://github.com/jpliew/PCT2075
    #endif


    typedef struct {
        char uuid[37];
        char product_id[33];
        char key[65];
    } deviceType;


    enum outputState{
        STATE_HIGH = HIGH,
        STATE_LOW = LOW
    };


    enum outputType{
        BINARY = 0,
        VARIABLE = 1
    };


    struct outputPin{
        outputState state = STATE_LOW;
        outputType type = BINARY;
    };


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
                return inputState::STATE_OPEN;
            }else{
                return inputState::STATE_CLOSED;
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

                //Values are different; Check if we are within the debounce delay
                if(millis() - inputController->inputs[i].timePreviousChange < DEBOUNCE_DELAY){

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
                        
                            //TODO: Something
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
                            
                            //TODO: Something
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

                //Setup the input controllers
                for(int i = 0; i < COUNT_IO_EXTENDER; i++){

                    pinMode(this->inputControllers[i].interruptPin, INPUT);

                    this->inputControllers[i].interruptPin = PINS_IO_EXTENDER[i];
                    this->inputControllers[i].address = ADDRESSES_IO_EXTENDER[i];
                    
                    #if MODEL_IO_EXTENDER == ENUM_MODEL_IO_EXTENDER_PCA9555
                        this->inputControllers[i].hardware.attach(Wire, inputControllers[i].address);
                        this->inputControllers[i].hardware.polarity(PCA95x5::Polarity::INVERTED_ALL);
                        this->inputControllers[i].hardware.direction(PCA95x5::Direction::IN_ALL);

                        //Ensure we connected to the controller
                        if(this->inputControllers[i].hardware.i2c_error() != 0){
                            this->inputControllers[i].enabled = false;
                            this->ptrFailureCallback();
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


    class managerFrontPanelButton{

        public:
            enum status{
                NORMAL = 0, //System is operating nomally
                TROUBLE = 1, //Temporary issue which can be corrected, such as temporary network outage
                FAILURE = 2 //Catastrophic failure that is non-recoverable
            };

            enum inputState{
                STATE_OPEN = LOW,
                STATE_CLOSED = HIGH
            };

            enum inputType{
                NORMALLY_OPEN = 0,
                NORMALLY_CLOSED = 1,
            };

        private:
            managerFrontPanelButton::status _ledStatus = TROUBLE;

        public:

            inputState state = STATE_OPEN; /* The state entered at timePreviousChange. Default STATE_OPEN.*/
            inputType type = NORMALLY_OPEN; /* Defines if the input is normally open or normally closed. Default NORMALLY_OPEN.*/

            void begin(){
                pinMode(PIN_OLED_BUTTON, INPUT);
                pinMode(PIN_OLED_LED, OUTPUT);
            }

            void setLED(managerFrontPanelButton::status value){

                if(_ledStatus == managerFrontPanelButton::FAILURE) { 
                    return; //Prevent the status from being returned to normal
                }

                switch(value){
                    case managerFrontPanelButton::status::FAILURE:
                        digitalWrite(PIN_OLED_LED, LOW);
                        break;

                    case managerFrontPanelButton::status::NORMAL:
                        digitalWrite(PIN_OLED_LED, HIGH);
                        break;

                    case managerFrontPanelButton::status::TROUBLE:
                        digitalWrite(PIN_OLED_LED, LOW);
                        break;                        

                    default:
                        digitalWrite(PIN_OLED_LED, LOW);
                        _ledStatus = managerFrontPanelButton::status::TROUBLE;
                        break;
                }

                _ledStatus = value;
            }
    };


    struct outputController{
        uint8_t address = 0; /* I2C address. Default 0.*/
        outputPin outputs[COUNT_PINS_OUTPUT_CONTROLLER];
        bool enabled = true; /* Indicates if the controller is enabled. Default true.*/

        #if MODEL_OUTPUT_CONTROLLER == ENUM_MODEL_OUTPUT_CONTROLLER_PCA9685
            PCA9685 hardware = PCA9685(0); /* Reference to the hardware. */
        #endif

    };


    class managerTemperatureSensors{

        enum temperatureSensorLocation{
            UNKNOWN = -1,
            CENTER = 0
        };

        struct temperatureSensor{
            uint8_t address = 0; /* I2C address. Default 0.*/

            #if MODEL_TEMPERATURE_SENSOR == ENUM_MODEL_TEMPERATURE_SENSOR_PCT2075
                PCT2075 hardware = PCT2075(0); /* Reference to the hardware. */
            #endif

            float previousRead = 0; /* Previous read temperature. Default 0. */
            unsigned long timePreviousRead = 0; /* Time (millis) when the sensor was last read. Default 0.*/
            temperatureSensorLocation location;
            bool enabled = true; /* Indicates if the sensor is enabled. Default true.*/
        };

        temperatureSensor temperatureSensors[COUNT_TEMPERATURE_SENSOR];

        /** Convert a temperatureSensorLocation to string */
        String locationToString(temperatureSensorLocation value){

            switch(value){

                case temperatureSensorLocation::CENTER:
                return F("CENTER");
                break;

                default:
                return F("UNKNOWN");
                break;
            }
        };

        void (*ptrPublisherCallback)(String, float);
        void (*ptrFailureCallback)(String);

        public:

            void setCallback_publisher(void (*userDefinedCallback)(String, float)) {
                      ptrPublisherCallback = userDefinedCallback; }

            void setCallback_failure(void (*userDefinedCallback)(String)) {
                      ptrFailureCallback = userDefinedCallback; }
            

            void begin(){

                for(int i = 0; i < COUNT_TEMPERATURE_SENSOR; i++){

                    this->temperatureSensors[i].address = ADDRESSES_TEMPERATURE_SENSORS[i];

                    #if MODEL_TEMPERATURE_SENSOR == ENUM_MODEL_TEMPERATURE_SENSOR_PCT2075
                        this->temperatureSensors[i].hardware = PCT2075(this->temperatureSensors[i].address);

                        if(this->temperatureSensors[i].hardware.getConfig() !=0){
                            temperatureSensors[i].enabled = false;
                            this->ptrFailureCallback(locationToString(temperatureSensors[i].location));
                        }

                    #endif

                    #if PRODUCT_ID == 32322211

                        switch(i){
                            case 0:
                                temperatureSensors[i].location = temperatureSensorLocation::CENTER;
                                break;
                            default:
                                temperatureSensors[i].location = temperatureSensorLocation::UNKNOWN;
                                break;
                        }
                    
                    #endif
                }
            };


            void loop(){

                //Loop through each temperature sensor on the board
                for(int i = 0; i < COUNT_TEMPERATURE_SENSOR; i++){

                    if(temperatureSensors[i].enabled == false){
                        continue;
                    }

                    //If the timer has expired OR if the temperature sensor has never been read, read it
                    if((millis() - temperatureSensors[i].timePreviousRead > MILLS_TEMPERATURE_SLEEP_DURATION) || (temperatureSensors[i].timePreviousRead == 0)){

                        //Temperatures will be reported in degrees C
                        #if MODEL_TEMPERATURE_SENSOR == ENUM_MODEL_TEMPERATURE_SENSOR_PCT2075
                            float currentRead = temperatureSensors[i].hardware.getTempC();
                        #endif

                        //Set the new read time time
                        temperatureSensors[i].timePreviousRead = millis();

                        //Ensure the hardware is still online
                        #if MODEL_TEMPERATURE_SENSOR == ENUM_MODEL_TEMPERATURE_SENSOR_PCT2075
                            if(this->temperatureSensors[i].hardware.getConfig() !=0){
                                temperatureSensors[i].enabled = false;
                                this->ptrFailureCallback(locationToString(temperatureSensors[i].location));
                            }
                        #endif
                        
                        //Check if the delta between the two reads is more than the DEGREES_TEMPERATURE_VARIATION_ALLOWED
                        if(abs(currentRead - temperatureSensors[i].previousRead) > DEGREES_TEMPERATURE_VARIATION_ALLOWED){

                            //Store the new temperature reading
                            temperatureSensors[i].previousRead = currentRead;

                            //Publish an event for the change
                            this->ptrPublisherCallback(locationToString(temperatureSensors[i].location), temperatureSensors[i].previousRead);
                        }      
                    }
                }

            };

    };

#endif