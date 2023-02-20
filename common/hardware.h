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
        #define PINS_INTERRUPT_IO_EXTENDER {35,25,26,27,4,5,18,19} /* Interrupt pins for the IO extenders, order must match the ADDRESSES_IO_EXTENDER. */

        /* I2C Addresses */
        #define ADDRESSES_IO_EXTENDER {0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27} /* I2C addresses for the IO extenders, order must match the PINS_INTERRUPT_IO_EXTENDER. */
        #define ADDRESSES_OUTPUT_CONTROLLER {0x40,0x41} /* I2C addresses for the output controllers. */
        #define ADDRESSES_TEMPERATURE_SENSOR {0x48} /* I2C addresses of the on-board temperature sensors. */
        #define ADDRESS_EEPROM 0x50 /* I2C addresses of the external EEPROM. */
        #define ADDRESS_OLED 0x3C /* I2C addresses of the OLED Display.  NOTE: This can be modifed on the display itself, default is 0x3C. */

    #endif


    #ifndef SUPPORTED_HARDWARE

        #error Build failed, Unknown PRODUCT_ID. Ensure it was set in ./.vscode/arduino.json {"buildPreferences":[["build.extra_flags","-DPRODUCT_ID=000000000"]]}

    #endif


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

    
#endif