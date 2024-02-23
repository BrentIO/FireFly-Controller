/*
    Hardware Definitions
*/

#ifndef hardware_definitions_h
    #define hardware_definitions_h

    #ifndef ESP32
        #error Build failed, missing ESP32 definition. Ensure it was set in ./.vscode/arduino.json {"buildPreferences":[["build.extra_flags","-DESP32"]]}
    #endif


    /* Define hardware enumeration constants */
    #define ENUM_IO_EXTENDER_MODEL_PCA9995 0 /* PCA9555 */
    #define ENUM_OUTPUT_CONTROLLER_MODEL_PCA9685 0 /* PCA9685 */
    #define ENUM_EEPROM_EXTERNAL_MODEL_24LCXXX 0 /* 24LC024 + Family */
    #define ENUM_TEMPERATURE_SENSOR_MODEL_PCT2075 0 /* PCT2075 */
    #define ENUM_OLED_MODEL_SSD1306_128_32 0 /* SSD1306 128x32px */
    #define ENUM_ETHERNET_MODEL_W5500 1 /* WIZnet W5500 */
    #define ENUM_ETHERNET_MODEL_NONE 0 /* For hardware without Ethernet */
    #define ENUM_WIFI_MODEL_ESP32 1 /* ESP32 Embedded WiFi */
    #define ENUM_WIFI_MODEL_NONE 0 /* For hardware without WiFi */


    /* Hardware Types */
    #if PRODUCT_HEX == 0x32322211

        #define SUPPORTED_HARDWARE

        /* Input IO Extender */
        #define IO_EXTENDER_MODEL ENUM_IO_EXTENDER_MODEL_PCA9995 /* IO Extender Model */
        #define IO_EXTENDER_COUNT 8 /* The number of IO extenders */
        #define IO_EXTENDER_ADDRESSES {0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27} /* I2C addresses for the IO extenders, order must match the PINS_INTERRUPT_IO_EXTENDER and should be sequential with the port numbers */
        #define IO_EXTENDER_CHANNELS {{1,1},{1,2},{1,3},{2,6},{1,6},{2,3},{2,2},{2,1},{3,1},{3,2},{3,3},{4,6},{3,6},{4,3},{4,2},{4,1}} /* Physical mapping of the {port,channel} by pin */
        #define IO_EXTENDER_INTERRUPT_PINS {35,25,26,27,4,5,18,19} /* Interrupt pins for the IO extenders, order must match the ADDRESSES_IO_EXTENDER */
        
        /* Output Controller */
        #define OUTPUT_CONTROLLER_MODEL ENUM_OUTPUT_CONTROLLER_MODEL_PCA9685
        #define OUTPUT_CONTROLLER_COUNT 2 /* The number of output controllers */
        #define OUTPUT_CONTROLLER_ADDRESSES {0x40,0x42} /* DEFECT on this model I2C addresses for the output controllers */
        #define OUTPUT_CONTROLLER_PORTS {2,1,3,4,6,5,7,8,10,9,11,12,14,13,15,16} /* Physical mapping of the output port numbers in sequence by output controller address, based on the output controllers' pin numbers */

        /* External EEPROM */
        #define EEPROM_EXTERNAL_MODEL ENUM_EEPROM_EXTERNAL_MODEL_24LCXXX
        #define EEPROM_EXTERNAL_WRITE_PROTECT_PIN 23 /* EEPROM write protect pin.  When low, write protect is disabled */
        #define EEPROM_EXTERNAL_ADDRESS 0x50 /* I2C addresses of the external EEPROM */

        /* Temperature Sensor */
        #define TEMPERATURE_SENSOR_MODEL ENUM_TEMPERATURE_SENSOR_MODEL_PCT2075
        #define TEMPERATURE_SENSOR_COUNT 1 /* The number of temperature sensors */
        #define TEMPERATURE_SENSOR_ADDRESSES {0x48} /* I2C addresses of the on-board temperature sensors */
        #define TEMPERATURE_SENSOR_LOCATIONS {"CENTER"}

        /* OLED Display and Button */
        #define OLED_DISPLAY_MODEL ENUM_OLED_MODEL_SSD1306_128_32
        #define OLED_BUTTON_PIN 32 /* Button for the front panel */
        #define OLED_LED_PIN 33 /* LED for the front panel button */
        
        /* Ethernet */
        #define ETHERNET_MODEL ENUM_ETHERNET_MODEL_W5500
        #define ETHERNET_PIN 0 /* Ethernet hardware control flow pin */

        /* WiFi */
        #define WIFI_MODEL ENUM_WIFI_MODEL_ESP32

    #endif


    #if PRODUCT_HEX == 0x32322304

        #define SUPPORTED_HARDWARE
       
        /* Input IO Extender */
        #define IO_EXTENDER_MODEL ENUM_IO_EXTENDER_MODEL_PCA9995 /* IO Extender Model */
        #define IO_EXTENDER_COUNT 8 /* The number of IO extenders */
        #define IO_EXTENDER_ADDRESSES {0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27} /* I2C addresses for the IO extenders, order must match the PINS_INTERRUPT_IO_EXTENDER and should be sequential with the port numbers */
        #define IO_EXTENDER_CHANNELS {{1,1},{1,2},{1,3},{2,6},{1,6},{2,3},{2,2},{2,1},{3,1},{3,2},{3,3},{4,6},{3,6},{4,3},{4,2},{4,1}} /* Physical mapping of the {port,channel} by pin */
        #define IO_EXTENDER_INTERRUPT_PINS {35,25,26,27,4,5,18,19} /* Interrupt pins for the IO extenders, order must match the ADDRESSES_IO_EXTENDER */
        
        /* Output Controller */
        #define OUTPUT_CONTROLLER_MODEL ENUM_OUTPUT_CONTROLLER_MODEL_PCA9685
        #define OUTPUT_CONTROLLER_COUNT 2 /* The number of output controllers */
        #define OUTPUT_CONTROLLER_ADDRESSES {0x40,0x41} /* I2C addresses for the output controllers */
        #define OUTPUT_CONTROLLER_PORTS {2,1,3,4,6,5,7,8,10,9,11,12,14,13,15,16} /* Physical mapping of the output port numbers in sequence by output controller address, based on the output controllers' pin numbers */

        /* External EEPROM */
        #define EEPROM_EXTERNAL_MODEL ENUM_EEPROM_EXTERNAL_MODEL_24LCXXX
        #define EEPROM_EXTERNAL_WRITE_PROTECT_PIN 23 /* EEPROM write protect pin.  When low, write protect is disabled */
        #define EEPROM_EXTERNAL_ADDRESS 0x50 /* I2C addresses of the external EEPROM */

        /* Temperature Sensor */
        #define TEMPERATURE_SENSOR_MODEL ENUM_TEMPERATURE_SENSOR_MODEL_PCT2075
        #define TEMPERATURE_SENSOR_COUNT 1 /* The number of temperature sensors */
        #define TEMPERATURE_SENSOR_ADDRESSES {0x48} /* I2C addresses of the on-board temperature sensors */
        #define TEMPERATURE_SENSOR_LOCATIONS {"CENTER"}

        /* OLED Display and Button */
        #define OLED_DISPLAY_MODEL ENUM_OLED_MODEL_SSD1306_128_32
        #define OLED_BUTTON_PIN 32 /* Button for the front panel */
        #define OLED_LED_PIN 33 /* LED for the front panel button */
        
        /* Ethernet */
        #define ETHERNET_MODEL ENUM_ETHERNET_MODEL_W5500
        #define ETHERNET_PIN 0 /* Ethernet hardware control flow pin */

        /* WiFi */
        #define WIFI_MODEL ENUM_WIFI_MODEL_ESP32

    #endif


    #if PRODUCT_HEX == 0x08062305

        #define SUPPORTED_HARDWARE
      
        /* Input IO Extender */
        #define IO_EXTENDER_MODEL ENUM_IO_EXTENDER_MODEL_PCA9995 /* IO Extender Model */
        #define IO_EXTENDER_COUNT 2 /* The number of IO extenders */
        #define IO_EXTENDER_ADDRESSES {0x20,0x21} /* I2C addresses for the IO extenders, order must match the PINS_INTERRUPT_IO_EXTENDER and should be sequential with the port numbers */
        #define IO_EXTENDER_CHANNELS {{1,1},{1,2},{1,3},{2,6},{1,6},{2,3},{2,2},{2,1},{3,1},{3,2},{3,3},{4,6},{3,6},{4,3},{4,2},{4,1}} /* Physical mapping of the {port,channel} by pin */
        #define IO_EXTENDER_INTERRUPT_PINS {34,35} /* Interrupt pins for the IO extenders, order must match the ADDRESSES_IO_EXTENDER */
        
        /* Output Controller */
        #define OUTPUT_CONTROLLER_MODEL ENUM_OUTPUT_CONTROLLER_MODEL_PCA9685
        #define OUTPUT_CONTROLLER_COUNT 1 /* The number of output controllers */
        #define OUTPUT_CONTROLLER_COUNT_PINS 6 /* Special for this model */
        #define OUTPUT_CONTROLLER_ADDRESSES {0x40} /* I2C addresses for the output controllers */
        #define OUTPUT_CONTROLLER_PORTS {2,1,3,4,6,5} /* Physical mapping of the output port numbers in sequence by output controller address, based on the output controllers' pin numbers */

        /* External EEPROM */
        #define EEPROM_EXTERNAL_MODEL ENUM_EEPROM_EXTERNAL_MODEL_24LCXXX
        #define EEPROM_EXTERNAL_WRITE_PROTECT_PIN 23 /* EEPROM write protect pin.  When low, write protect is disabled */
        #define EEPROM_EXTERNAL_ADDRESS 0x50 /* I2C addresses of the external EEPROM */

        /* Temperature Sensor */
        #define TEMPERATURE_SENSOR_MODEL ENUM_TEMPERATURE_SENSOR_MODEL_PCT2075
        #define TEMPERATURE_SENSOR_COUNT 1 /* The number of temperature sensors */
        #define TEMPERATURE_SENSOR_ADDRESSES {0x48} /* I2C addresses of the on-board temperature sensors */
        #define TEMPERATURE_SENSOR_LOCATIONS {"CENTER"}

        /* OLED Display and Button */
        #define OLED_DISPLAY_MODEL ENUM_OLED_MODEL_SSD1306_128_32
        #define OLED_BUTTON_PIN 32 /* Button for the front panel */
        #define OLED_LED_PIN 33 /* LED for the front panel button */
        
        /* Ethernet */
        #define ETHERNET_MODEL ENUM_ETHERNET_MODEL_W5500
        #define ETHERNET_PIN 25 /* Ethernet hardware control flow pin */
        #define ETHERNET_PIN_RESET 26
        #define ETHERNET_PIN_INTERRUPT 19
        #define ETH_SPI_HOST SPI2_HOST /* Tell W5000 async library to use SPI2 */

        /* SPI Configuration */
        #define SPI_SCK_PIN 14
        #define SPI_MISO_PIN 12
        #define SPI_MOSI_PIN 13

    #endif


    #ifndef SUPPORTED_HARDWARE

        #error Build failed, Unknown PRODUCT_HEX. Ensure it was set in ./.vscode/arduino.json {"buildPreferences":[["build.extra_flags","-DPRODUCT_HEX=0x000000000"]]}

    #endif


    /* Include hardware-specific libraries */

    #ifndef DEBOUNCE_DELAY
        #define DEBOUNCE_DELAY 150 /* Milliseconds between changes for debouncing on the front panel button. */
    #endif


    #if IO_EXTENDER_MODEL == ENUM_IO_EXTENDER_MODEL_PCA9995
        #include <PCA95x5.h>

        #ifndef IO_EXTENDER_COUNT_PINS
            #define IO_EXTENDER_COUNT_PINS 16 /* The number of pins on each IO extender */
        #endif

        #ifndef IO_EXTENDER_COUNT_CHANNELS_PER_PORT
            #define IO_EXTENDER_COUNT_CHANNELS_PER_PORT 4 /* Number of channels (wires) per RJ-45 port that are usable */
        #endif

        #ifndef IO_EXTENDER_MINIMUM_CHANGE_DELAY
            #define IO_EXTENDER_MINIMUM_CHANGE_DELAY 100 /* Milliseconds between a change being detected and an event triggering. This also handles debouncing for the main I/O.  */
        #endif

        #ifndef IO_EXTENDER_MINIMUM_LONG_CHANGE_DELAY
            #define IO_EXTENDER_MINIMUM_LONG_CHANGE_DELAY 1000 /* Milliseconds between the MINIMUM_CHANGE_DELAY that the change is observed before it is raised as a long change event. */
        #endif

    #endif


    #if OUTPUT_CONTROLLER_MODEL == ENUM_OUTPUT_CONTROLLER_MODEL_PCA9685
        #include <PCA9685.h>

        #ifndef OUTPUT_CONTROLLER_MAXIMUM_PWM
            #define OUTPUT_CONTROLLER_MAXIMUM_PWM 4095 /* Maximum value for PWM */
        #endif

        #ifndef OUTPUT_CONTROLLER_FREQUENCY_PWM
            #define OUTPUT_CONTROLLER_FREQUENCY_PWM 1500 /* PWM Frequency in Hz */
        #endif

        #ifndef OUTPUT_CONTROLLER_COUNT_PINS
            #define OUTPUT_CONTROLLER_COUNT_PINS 16 /* The number of pins on each output controller. */
        #endif

    #endif


    #if EEPROM_EXTERNAL_MODEL == ENUM_EEPROM_EXTERNAL_MODEL_24LCXXX
        #include <I2C_eeprom.h>

        #ifndef EEPROM_EXTERNAL_SIZE
            #define EEPROM_EXTERNAL_SIZE 256 /* Six of the external EEPROM.  NOTE: Divide the device size in Kbits by 8. */
        #endif

    #endif


    #if TEMPERATURE_SENSOR_MODEL == ENUM_TEMPERATURE_SENSOR_MODEL_PCT2075
        #include <PCT2075.h>

        #ifndef TEMPERATURE_SENSOR_SLEEP_DURATION_MILLIS
            #define TEMPERATURE_SENSOR_SLEEP_DURATION_MILLIS 1000 /* Number of millis to wait between reading the temperatures. Default 1000. */
        #endif

        #ifndef TEMPERATURE_SENSOR_DEGREES_VARIATION_ALLOWED
             #define TEMPERATURE_SENSOR_DEGREES_VARIATION_ALLOWED 0.25 /* Number of degrees allowed variance between temperature reads before the new value is stored and reported. Default 0.25. */
        #endif

    #endif


    #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32

        #ifndef SSD1306_NO_SPLASH
            #error Build failed, missing SSD1306_NO_SPLASH to prevent the Adafruit logo from being uploaded. Ensure it was set in ./.vscode/arduino.json {"buildPreferences":[["build.extra_flags","-DSSD1306_NO_SPLASH"]]}
        #endif

        #include <Wire.h>
        #include <Adafruit_GFX.h>
        #include <Adafruit_SSD1306.h>

        #define OLED_DISPLAY_WIDTH 128 /* Width, in number of pixels */
        #define OLED_DISPLAY_HEIGHT 32 /* Height, in numberof pixel s*/
        #define OLED_SCROLL_BAR_WIDTH 2 /* Width of the scrollbar displayed on the right side of the OLED */
        #define OLED_SCROLL_BAR_HEIGHT (OLED_SCROLL_BAR_WIDTH * 2) /* Height of the scrollbar */
        #define OLED_CHARACTERS_PER_LINE 21 /* Number of characters that can be printed per line */
        #define OLED_NUMBER_OF_LINES 4 /* Number of lines that can be printed on the display */

        #ifndef OLED_ADDRESS
            #define OLED_ADDRESS 0x3C /* I2C addresses of the OLED Display.  NOTE: This can be modifed on the display itself, default is 0x3C */
        #endif
        
    #endif


    #if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_W5500
        #include <SPI.h>
        #include <Ethernet.h>
        #include <EthernetUdp.h>
        #include <AsyncWebServer_ESP32_W5500.h>
        #include <AsyncTCP.h>

        #define WIFI_MODEL ENUM_WIFI_MODEL_NONE     /* Disable WiFi when Ethernet is present */
        #define _ETHERNET_WEBSERVER_LOGLEVEL_ 0     /* Prevents messages from async web server */

        #if DEBUG > 2000
            #define _ETHERNET_WEBSERVER_LOGLEVEL_ 1
        #endif
        
        #if DEBUG > 3000
            #define _ETHERNET_WEBSERVER_LOGLEVEL_ 2
        #endif

        #if DEBUG > 4000
            #define _ETHERNET_WEBSERVER_LOGLEVEL_ 3
        #endif

        #if DEBUG > 5000
            #define _ETHERNET_WEBSERVER_LOGLEVEL_ 4
        #endif

        #ifndef SPI_SCK_PIN
            #error SPI_SCK_PIN not set
        #endif

        #ifndef SPI_MISO_PIN
            #error SPI_MISO_PIN not set
        #endif

        #ifndef SPI_MOSI_PIN
            #error SPI_MISO_PIN not set
        #endif

        #ifndef ETHERNET_PIN
            #error ETHERNET_PIN not set
        #endif

    #endif

    #if ETHERNET_MODEL != ENUM_ETHERNET_MODEL_NONE
        #define ETHERNET_TIMEOUT 10000 //Number of milliseconds before Ethernet will time out
    #endif


    #if WIFI_MODEL != ENUM_WIFI_MODEL_NONE

        #include <WiFi.h>

        #if WIFI_MODEL = ENUM_WIFI_MODEL_ESP32
            #include <ESPAsyncWebServer.h>
            #include <AsyncTCP.h>
        #endif

        #ifndef WIFI_TIMEOUT
            #define WIFI_TIMEOUT 10000 //Number of milliseconds before WiFi will time out
        #endif

    #endif


    #ifdef ESP32
        #include <esp_chip_info.h>
    #endif


    /* Check to ensure configuration is acceptable */
    #ifndef IO_EXTENDER_MODEL
        #error IO_EXTENDER_MODEL not set
    #endif


    #ifndef OUTPUT_CONTROLLER_MODEL
        #error OUTPUT_CONTROLLER_MODEL not set
    #endif


    #ifndef EEPROM_EXTERNAL_MODEL
        #error EEPROM_EXTERNAL_MODEL not set
    #endif


    #ifndef TEMPERATURE_SENSOR_MODEL
        #error TEMPERATURE_SENSOR_MODEL not set
    #endif


    #ifndef OLED_DISPLAY_MODEL
        #error OLED_DISPLAY_MODEL not set
    #endif


    /** Common health status for all peripherals. */
    struct structHealth{
        uint8_t address; /* I2C address.*/
        bool enabled; /* Indicates if the controller is enabled.*/
    };
    
#endif