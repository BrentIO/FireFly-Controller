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
        #define _PIN_OLED_BUTTON 32 /* Button for the front panel. */
        #define _PIN_OLED_LED 33 /* LED for the front panel button. */
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


    enum inputType{
        NORMALLY_OPEN = 0,
        NORMALLY_CLOSED = 1,
    };


    enum inputState{
        STATE_OPEN = LOW,
        STATE_CLOSED = HIGH
    };


    enum outputState{
        STATE_HIGH = HIGH,
        STATE_LOW = LOW
    };


    enum outputType{
        BINARY = 0,
        VARIABLE = 1
    };


    enum temperatureSensorLocation{
        UNKNOWN = -1,
        CENTER = 0
    };


    struct inputPin{
        unsigned long timePreviousChange = 0; /* Time (millis) when the input state last changed.  Value is set to 0 when the state returns to its input type. Default 0.*/
        inputState state = STATE_OPEN; /* The state entered at timePreviousChange. Default STATE_OPEN.*/
        inputType type = NORMALLY_OPEN; /* Defines if the input is normally open or normally closed. Default NORMALLY_OPEN.*/
        boolean monitorLongChange = false; /* Defines if the pin should be monitored for long changes.  Should be _true_ for buttons and _false_ for reed switches. Default false.*/
    };


    struct outputPin{
        outputState state = STATE_LOW;
        outputType type = BINARY;
    };


    struct ioExtender{

        #if MODEL_IO_EXTENDER == ENUM_MODEL_IO_EXTENDER_PCA9995
            PCA9555 hardware; /* Reference to the hardware. */
        #endif

        uint8_t interruptPin = 0; /* Interrupt pin. Default 0. */
        uint8_t address = 0; /* I2C address. Default 0.*/
        uint16_t previousRead = 0; /* Numeric value of the last read from the hardware. Default 0.*/
        inputPin inputs[COUNT_PINS_IO_EXTENDER]; /* Input pins connected to the hardware.*/


    class oledLEDButton{

        public:
            enum status{
                NORMAL = 0, //System is operating nomally
                TROUBLE = 1, //Temporary issue which can be corrected, such as temporary network outage
                FAILURE = 2 //Catastrophic failure that is non-recoverable
            };

        private:
            oledLEDButton::status _ledStatus = TROUBLE;

        public:
            uint8_t interruptPin = _PIN_OLED_BUTTON; /* Interrupt pin.*/
            uint8_t ledPin = _PIN_OLED_LED; /* LED output pin.*/
            unsigned long timePreviousChange = 0; /* Time (millis) when the input state last changed.  Value is set to 0 when the state returns to its input type. Default 0.*/
            inputState state = STATE_OPEN; /* The state entered at timePreviousChange. Default STATE_OPEN.*/
            inputType type = NORMALLY_OPEN; /* Defines if the input is normally open or normally closed. Default NORMALLY_OPEN.*/

            void begin(){
                pinMode(this->interruptPin, INPUT);
                pinMode(this->ledPin, OUTPUT);
            }

            void setLED(oledLEDButton::status value){

                if(_ledStatus == oledLEDButton::FAILURE) { 
                    return; //Prevent the status from being returned to normal
                }

                switch(value){
                    case oledLEDButton::status::FAILURE:
                        digitalWrite(ledPin, LOW);
                        break;

                    case oledLEDButton::status::NORMAL:
                        digitalWrite(ledPin, HIGH);
                        break;

                    case oledLEDButton::status::TROUBLE:
                        digitalWrite(ledPin, LOW);
                        break;                        

                    default:
                        digitalWrite(ledPin, LOW);
                        _ledStatus = oledLEDButton::status::TROUBLE;
                        break;
                }

                _ledStatus = value;
            }
    };


    struct outputController{
        uint8_t address = 0; /* I2C address. Default 0.*/
        outputPin outputs[COUNT_PINS_OUTPUT_CONTROLLER];

        #if MODEL_OUTPUT_CONTROLLER == ENUM_MODEL_OUTPUT_CONTROLLER_PCA9685
            PCA9685 hardware = PCA9685(0); /* Reference to the hardware. */
        #endif

    };


    struct temperatureSensor{
        uint8_t address = 0; /* I2C address. Default 0.*/

        #if MODEL_TEMPERATURE_SENSOR == ENUM_MODEL_TEMPERATURE_SENSOR_PCT2075
            PCT2075 hardware = PCT2075(0); /* Reference to the hardware. */
        #endif
        
        float previousRead = 0; /* Previous read temperature. Default 0. */
        unsigned long timePreviousRead = 0; /* Time (millis) when the sensor was last read. Default 0.*/
        temperatureSensorLocation location;
    };

#endif