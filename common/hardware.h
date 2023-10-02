/*
    Hardware Definitions
*/

#ifndef hardware_definitions_h
    #define hardware_definitions_h

    #ifndef ESP32
        #error Build failed, missing ESP32 definition. Ensure it was set in ./.vscode/arduino.json {"buildPreferences":[["build.extra_flags","-DESP32"]]}
    #endif

    /* Define hardware enumeration constants */
    #define ENUM_MODEL_IO_EXTENDER_PCA9995 0 /* PCA9555 */
    #define ENUM_MODEL_OUTPUT_CONTROLLER_PCA9685 0 /* PCA9685 */
    #define ENUM_MODEL_EEPROM_EXTERNAL_24LCXXX 0 /* 24LC024 + Family */
    #define ENUM_MODEL_TEMPERATURE_SENSOR_PCT2075 0 /* PCT2075 */
    #define ENUM_MODEL_OLED_SSD1306_128_32 0 /* SSD1306 128x32px */

    #define DEBOUNCE_DELAY 150 /* Milliseconds between changes for debouncing on the front panel button. */
    #define MINIMUM_CHANGE_DELAY 100 /* Milliseconds between a change being detected and an event triggering. This also handles debouncing for the main I/O.  */
    #define MINIMUM_LONG_CHANGE_DELAY 1000 /* Milliseconds between the MINIMUM_CHANGE_DELAY that the change is observed before it is raised as a long change event. */

    /* Hardware Types */
    #if PRODUCT_HEX == 0x32322211

        #define SUPPORTED_HARDWARE
      
        /* Input IO Extender */
        #define COUNT_IO_EXTENDER 8 /* The number of IO extenders. */
        #define MODEL_IO_EXTENDER ENUM_MODEL_IO_EXTENDER_PCA9995 /* IO Extender Model. */
        #define COUNT_PINS_IO_EXTENDER 16 /* The number of pins on each IO extender. */
        #define COUNT_CHANNELS_PER_PORT 4 /* Number of channels (wires) per RJ-45 port that are usable. */
        #define IO_EXTENDER_CHANNELS {{1,1},{1,2},{1,3},{2,6},{1,6},{2,3},{2,2},{2,1},{3,1},{3,2},{3,3},{4,6},{3,6},{4,3},{4,2},{4,1}} /* Physical mapping of the {port,channel} by pin */
        
        /* Output Controller */
        #define COUNT_OUTPUT_CONTROLLER 2 /* The number of output controllers. */
        #define MODEL_OUTPUT_CONTROLLER ENUM_MODEL_OUTPUT_CONTROLLER_PCA9685
        
        /* External EEPROM */
        #define MODEL_EEPROM_EXTERNAL ENUM_MODEL_EEPROM_EXTERNAL_24LCXXX
        #define SIZE_EEPROM 256 /* Six of the external EEPROM.  NOTE: Divide the device size in Kbits by 8. */

        /* Temperature Sensor */
        #define COUNT_TEMPERATURE_SENSOR 1 /* The number of temperature sensors. */
        #define MODEL_TEMPERATURE_SENSOR ENUM_MODEL_TEMPERATURE_SENSOR_PCT2075
        #define LOCATION_TEMPERATURE_SENSOR {"CENTER"}

        /* OLED Display */
        #define MODEL_OLED_DISPLAY ENUM_MODEL_OLED_SSD1306_128_32
        
        /* Pin Addresses */
        #define PIN_EEPROM_WP 23 /* EEPROM write protect pin.  When low, write protect is disabled. */
        #define PIN_ETHERNET 0 /* Ethernet hardware control flow pin. */
        #define PIN_OLED_BUTTON 32 /* Button for the front panel. */
        #define PIN_OLED_LED 33 /* LED for the front panel button. */
        #define PINS_INTERRUPT_IO_EXTENDER {35,25,26,27,4,5,18,19} /* Interrupt pins for the IO extenders, order must match the ADDRESSES_IO_EXTENDER. */

        /* I2C Addresses */
        #define ADDRESSES_IO_EXTENDER {0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27} /* I2C addresses for the IO extenders, order must match the PINS_INTERRUPT_IO_EXTENDER and should be sequential with the port numbers. */
        #define ADDRESSES_OUTPUT_CONTROLLER {0x40,0x42} /* I2C addresses for the output controllers. */
        #define ADDRESSES_TEMPERATURE_SENSOR {0x48} /* I2C addresses of the on-board temperature sensors. */
        #define ADDRESS_EEPROM 0x50 /* I2C addresses of the external EEPROM. */
        #define ADDRESS_OLED 0x3C /* I2C addresses of the OLED Display.  NOTE: This can be modifed on the display itself, default is 0x3C. */

    #endif


    #if PRODUCT_HEX == 0x32322304

        #define SUPPORTED_HARDWARE
       
        /* Input IO Extender */
        #define COUNT_IO_EXTENDER 8 /* The number of IO extenders. */
        #define MODEL_IO_EXTENDER ENUM_MODEL_IO_EXTENDER_PCA9995 /* IO Extender Model. */
        #define COUNT_PINS_IO_EXTENDER 16 /* The number of pins on each IO extender. */
        #define COUNT_CHANNELS_PER_PORT 4 /* Number of channels (wires) per RJ-45 port that are usable. */
        #define IO_EXTENDER_CHANNELS {{1,1},{1,2},{1,3},{2,6},{1,6},{2,3},{2,2},{2,1},{3,1},{3,2},{3,3},{4,6},{3,6},{4,3},{4,2},{4,1}} /* Physical mapping of the {port,channel} by pin */
        
        /* Output Controller */
        #define COUNT_OUTPUT_CONTROLLER 2 /* The number of output controllers. */
        #define MODEL_OUTPUT_CONTROLLER ENUM_MODEL_OUTPUT_CONTROLLER_PCA9685
        #define OUTPUT_CONTROLLER_PORTS {2,1,3,4,6,5,7,8,10,9,11,12,14,13,15,16} /* Physical mapping of the output port numbers in sequence by output controller address, based on the output controllers' pin numbers */

        /* External EEPROM */
        #define MODEL_EEPROM_EXTERNAL ENUM_MODEL_EEPROM_EXTERNAL_24LCXXX
        #define SIZE_EEPROM 256 /* Six of the external EEPROM.  NOTE: Divide the device size in Kbits by 8. */

        /* Temperature Sensor */
        #define COUNT_TEMPERATURE_SENSOR 1 /* The number of temperature sensors. */
        #define MODEL_TEMPERATURE_SENSOR ENUM_MODEL_TEMPERATURE_SENSOR_PCT2075
        #define LOCATION_TEMPERATURE_SENSOR {"CENTER"}

        /* OLED Display */
        #define MODEL_OLED_DISPLAY ENUM_MODEL_OLED_SSD1306_128_32
        
        /* Pin Addresses */
        #define PIN_EEPROM_WP 23 /* EEPROM write protect pin.  When low, write protect is disabled. */
        #define PIN_ETHERNET 0 /* Ethernet hardware control flow pin. */
        #define PIN_OLED_BUTTON 32 /* Button for the front panel. */
        #define PIN_OLED_LED 33 /* LED for the front panel button. */
        #define PINS_INTERRUPT_IO_EXTENDER {34,35,26,27,4,5,18,19} /* Interrupt pins for the IO extenders, order must match the ADDRESSES_IO_EXTENDER. */

        /* I2C Addresses */
        #define ADDRESSES_IO_EXTENDER {0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27} /* I2C addresses for the IO extenders, order must match the PINS_INTERRUPT_IO_EXTENDER. */
        #define ADDRESSES_OUTPUT_CONTROLLER {0x40,0x41} /* I2C addresses for the output controllers. */
        #define ADDRESSES_TEMPERATURE_SENSOR {0x48} /* I2C addresses of the on-board temperature sensors. */
        #define ADDRESS_EEPROM 0x50 /* I2C addresses of the external EEPROM. */
        #define ADDRESS_OLED 0x3C /* I2C addresses of the OLED Display.  NOTE: This can be modifed on the display itself, default is 0x3C. */

    #endif

    #if PRODUCT_HEX == 0x08062305

        #define SUPPORTED_HARDWARE
      
        /* Input IO Extender */
        #define COUNT_IO_EXTENDER 2 /* The number of IO extenders. */
        #define MODEL_IO_EXTENDER ENUM_MODEL_IO_EXTENDER_PCA9995 /* IO Extender Model. */
        #define COUNT_PINS_IO_EXTENDER 16 /* The number of pins on each IO extender. */
        #define COUNT_CHANNELS_PER_PORT 4 /* Number of channels (wires) per RJ-45 port that are usable. */
        #define IO_EXTENDER_CHANNELS {{1,1},{1,2},{1,3},{2,6},{1,6},{2,3},{2,2},{2,1},{3,1},{3,2},{3,3},{4,6},{3,6},{4,3},{4,2},{4,1}} /* Physical mapping of the {port,channel} by pin */
        
        /* Output Controller */
        #define COUNT_OUTPUT_CONTROLLER 1 /* The number of output controllers. */
        #define MODEL_OUTPUT_CONTROLLER ENUM_MODEL_OUTPUT_CONTROLLER_PCA9685
        #define OUTPUT_CONTROLLER_PORTS {2,1,3,4,6,5,7,8,10,9,11,12,14,13,15,16} /* Physical mapping of the output port numbers in sequence by output controller address, based on the output controllers' pin numbers */

        /* External EEPROM */
        #define MODEL_EEPROM_EXTERNAL ENUM_MODEL_EEPROM_EXTERNAL_24LCXXX
        #define SIZE_EEPROM 256 /* Six of the external EEPROM.  NOTE: Divide the device size in Kbits by 8. */

        /* Temperature Sensor */
        #define COUNT_TEMPERATURE_SENSOR 1 /* The number of temperature sensors. */
        #define MODEL_TEMPERATURE_SENSOR ENUM_MODEL_TEMPERATURE_SENSOR_PCT2075
        #define LOCATION_TEMPERATURE_SENSOR {"CENTER"}

        /* OLED Display */
        #define MODEL_OLED_DISPLAY ENUM_MODEL_OLED_SSD1306_128_32
        
        /* Pin Addresses */
        #define PIN_EEPROM_WP 23 /* EEPROM write protect pin.  When low, write protect is disabled. */
        #define PIN_ETHERNET 0 /* Ethernet hardware control flow pin. */
        #define PIN_OLED_BUTTON 32 /* Button for the front panel. */
        #define PIN_OLED_LED 33 /* LED for the front panel button. */
        #define PINS_INTERRUPT_IO_EXTENDER {34,35} /* Interrupt pins for the IO extenders, order must match the ADDRESSES_IO_EXTENDER. */

        /* I2C Addresses */
        #define ADDRESSES_IO_EXTENDER {0x20,0x21} /* I2C addresses for the IO extenders, order must match the PINS_INTERRUPT_IO_EXTENDER and should be sequential with the port numbers. */
        #define ADDRESSES_OUTPUT_CONTROLLER {0x40} /* I2C addresses for the output controllers. */
        #define ADDRESSES_TEMPERATURE_SENSOR {0x48} /* I2C addresses of the on-board temperature sensors. */
        #define ADDRESS_EEPROM 0x50 /* I2C addresses of the external EEPROM. */
        #define ADDRESS_OLED 0x3C /* I2C addresses of the OLED Display.  NOTE: This can be modifed on the display itself, default is 0x3C. */

    #endif


    #ifndef SUPPORTED_HARDWARE

        #error Build failed, Unknown PRODUCT_HEX. Ensure it was set in ./.vscode/arduino.json {"buildPreferences":[["build.extra_flags","-DPRODUCT_HEX=0x000000000"]]}

    #endif


    /* Include hardware-specific libraries */
    #if MODEL_IO_EXTENDER == ENUM_MODEL_IO_EXTENDER_PCA9995
        #include <PCA95x5.h> // https://github.com/BrentIO/PCA95x5
    #endif


    #if MODEL_OUTPUT_CONTROLLER == ENUM_MODEL_OUTPUT_CONTROLLER_PCA9685
        #include <PCA9685.h> // https://github.com/RobTillaart/PCA9685_RT

        #ifndef MAXIMUM_PWM
            #define MAXIMUM_PWM 4095 /* Maximum value for PWM */
        #endif

        #ifndef FREQUENCY_PWM
            #define FREQUENCY_PWM 1500 /* PWM Frequency in Hz */
        #endif

        #ifndef COUNT_PINS_OUTPUT_CONTROLLER
            #define COUNT_PINS_OUTPUT_CONTROLLER 16 /* The number of pins on each output controller. */
        #endif

    #endif


    #if MODEL_EEPROM_EXTERNAL == ENUM_MODEL_EEPROM_EXTERNAL_24LCXXX
        #include <I2C_eeprom.h> // https://github.com/RobTillaart/I2C_EEPROM
    #endif


    #if MODEL_TEMPERATURE_SENSOR == ENUM_MODEL_TEMPERATURE_SENSOR_PCT2075
        #include <PCT2075.h> // https://github.com/BrentIO/PCT2075

        #define MILLS_TEMPERATURE_SLEEP_DURATION 1000 /* Number of millis to wait between reading the temperatures. Default 1000. */
        #define DEGREES_TEMPERATURE_VARIATION_ALLOWED 0.25 /* Number of degrees allowed variance between temperature reads before the new value is stored and reported. Default 0.25. */

    #endif


    #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32

        #ifndef SSD1306_NO_SPLASH
            #error Build failed, missing SSD1306_NO_SPLASH to prevent the Adafruit logo from being uploaded. Ensure it was set in ./.vscode/arduino.json {"buildPreferences":[["build.extra_flags","-DSSD1306_NO_SPLASH"]]}
        #endif

        #include <Wire.h>
        #include <Adafruit_GFX.h>
        #include <Adafruit_SSD1306.h> //https://github.com/adafruit/Adafruit_SSD1306

        #define DISPLAY_WIDTH 128
        #define DISPLAY_HEIGHT 32
        #define SCROLL_BAR_WIDTH 2
        #define SCROLL_BAR_HEIGHT (SCROLL_BAR_WIDTH * 2)
        #define CHARACTERS_PER_LINE 21
        #define NUMBER_OF_LINES 4
    #endif


    /** Common health status for all peripherals. */
    struct structHealth{
        uint8_t address; /* I2C address.*/
        bool enabled; /* Indicates if the controller is enabled.*/
    };
    
#endif