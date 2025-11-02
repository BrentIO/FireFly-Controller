/*
    Hardware Definitions
*/

#ifndef hardware_definitions_h
    #define hardware_definitions_h

    #ifndef ESP32
        #error Build failed, missing ESP32 definition. Ensure it was set in ./.vscode/arduino.json {"buildPreferences":[["build.extra_flags","-DESP32"]]}
    #endif


    /* Define hardware enumeration constants */
    #define ENUM_IO_EXTENDER_MODEL_PCA9995 0 /* NXP Semiconductors PCA9555 16-bit I2C-bus and SMBus I/O port with interrupt */
    #define ENUM_OUTPUT_CONTROLLER_MODEL_PCA9685 0 /* NXP Semiconductors PCA9685 16-channel, 12-bit PWM Fm+ I2C-bus LED controller */
    #define ENUM_EEPROM_EXTERNAL_MODEL_24LCXXX 0 /* Microchip Technology 24LCXXX Family I2C Serial EEPROM */
    #define ENUM_TEMPERATURE_SENSOR_MODEL_PCT2075 0 /* NXP Semiconductors PCT2075 I2C-bus Fm+, 1 °C accuracy, digital temperature sensor and thermal watchdog */
    #define ENUM_OLED_MODEL_SSD1306_128_32 0 /* Vishay SSD1306 128x32px Graphic OLED Display */
    #define ENUM_ETHERNET_MODEL_W5500 1 /* WIZnet W5500 hardwired TCP/IP embedded Ethernet controller */
    #define ENUM_ETHERNET_MODEL_NONE 0 /* For hardware without Ethernet */
    #define ENUM_WIFI_MODEL_ESP32 1 /* Espressif Systems ESP32 Embedded WiFi */
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
        #define OUTPUT_CONTROLLER_MODEL ENUM_OUTPUT_CONTROLLER_MODEL_PCA9685 /* Output controller model */
        #define OUTPUT_CONTROLLER_COUNT 2 /* The number of output controllers */
        #define OUTPUT_CONTROLLER_ADDRESSES {0x40,0x42} /* DEFECT on this model I2C addresses for the output controllers */
        #define OUTPUT_CONTROLLER_PORTS {2,1,3,4,6,5,7,8,10,9,11,12,14,13,15,16} /* Physical mapping of the output port numbers in sequence by output controller address, based on the output controllers' pin numbers */

        /* External EEPROM */
        #define EEPROM_EXTERNAL_MODEL ENUM_EEPROM_EXTERNAL_MODEL_24LCXXX /* External EEPROM controller model */
        #define EEPROM_EXTERNAL_WRITE_PROTECT_PIN 23 /* EEPROM write protect pin.  When low, write protect is disabled */
        #define EEPROM_EXTERNAL_ADDRESS 0x50 /* I2C addresses of the external EEPROM */

        /* Temperature Sensor */
        #define TEMPERATURE_SENSOR_MODEL ENUM_TEMPERATURE_SENSOR_MODEL_PCT2075 /* Temperature sensor model */
        #define TEMPERATURE_SENSOR_COUNT 1 /* The number of temperature sensors */
        #define TEMPERATURE_SENSOR_ADDRESSES {0x48} /* I2C addresses of the on-board temperature sensors */
        #define TEMPERATURE_SENSOR_LOCATIONS {"Center"} /* Temperature locations as an array, maximum string length = TEMPERATURE_SENSOR_LOCATION_MAX_LENGTH */

        /* OLED Display and Button */
        #define OLED_DISPLAY_MODEL ENUM_OLED_MODEL_SSD1306_128_32 /* OLED display model */
        #define OLED_BUTTON_PIN 32 /* Button for the front panel */
        #define OLED_LED_PIN 33 /* LED for the front panel button */
        
        /* Ethernet */
        #define ETHERNET_MODEL ENUM_ETHERNET_MODEL_NONE // INOPERATIVE THIS MODEL

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
        #define OUTPUT_CONTROLLER_MODEL ENUM_OUTPUT_CONTROLLER_MODEL_PCA9685 /* Output controller model */
        #define OUTPUT_CONTROLLER_COUNT 2 /* The number of output controllers */
        #define OUTPUT_CONTROLLER_ADDRESSES {0x40,0x41} /* I2C addresses for the output controllers */
        #define OUTPUT_CONTROLLER_PORTS {2,1,3,4,6,5,7,8,10,9,11,12,14,13,15,16} /* Physical mapping of the output port numbers in sequence by output controller address, based on the output controllers' pin numbers */

        /* External EEPROM */
        #define EEPROM_EXTERNAL_MODEL ENUM_EEPROM_EXTERNAL_MODEL_24LCXXX /* External EEPROM controller model */
        #define EEPROM_EXTERNAL_WRITE_PROTECT_PIN 23 /* EEPROM write protect pin.  When low, write protect is disabled */
        #define EEPROM_EXTERNAL_ADDRESS 0x50 /* I2C addresses of the external EEPROM */

        /* Temperature Sensor */
        #define TEMPERATURE_SENSOR_MODEL ENUM_TEMPERATURE_SENSOR_MODEL_PCT2075 /* Temperature sensor model */
        #define TEMPERATURE_SENSOR_COUNT 1 /* The number of temperature sensors */
        #define TEMPERATURE_SENSOR_ADDRESSES {0x48} /* I2C addresses of the on-board temperature sensors */
        #define TEMPERATURE_SENSOR_LOCATIONS {"Center"} /* Temperature locations as an array, maximum string length = TEMPERATURE_SENSOR_LOCATION_MAX_LENGTH */

        /* OLED Display and Button */
        #define OLED_DISPLAY_MODEL ENUM_OLED_MODEL_SSD1306_128_32 /* OLED display model */
        #define OLED_BUTTON_PIN 32 /* Button for the front panel */
        #define OLED_LED_PIN 33 /* LED for the front panel button */
        
        /* Ethernet */
        #define ETHERNET_MODEL ENUM_ETHERNET_MODEL_NONE // INOPERATIVE THIS MODEL

        /* WiFi */
        #define WIFI_MODEL ENUM_WIFI_MODEL_ESP32

    #endif


    #if PRODUCT_HEX == 0x08062305

        #define SUPPORTED_HARDWARE
      
        /* Input IO Extender */
        #define IO_EXTENDER_MODEL ENUM_IO_EXTENDER_MODEL_PCA9995 /* IO Extender Model */
        #define IO_EXTENDER_COUNT 2 /* The number of IO extenders */
        #define IO_EXTENDER_ADDRESSES {0x20,0x21} /* I2C addresses for the IO extenders, order must match the PINS_INTERRUPT_IO_EXTENDER and should be sequential with the port numbers */
        #define IO_EXTENDER_CHANNELS {{2,1},{2,2},{2,3},{1,4},{2,4},{1,3},{1,2},{1,1},{4,1},{4,2},{4,3},{3,4},{4,4},{3,3},{3,2},{3,1}} /* Physical mapping of the {port,channel} by pin */
        #define IO_EXTENDER_INTERRUPT_PINS {34,35} /* Interrupt pins for the IO extenders, order must match the ADDRESSES_IO_EXTENDER */
        
        /* Output Controller */
        #define OUTPUT_CONTROLLER_MODEL ENUM_OUTPUT_CONTROLLER_MODEL_PCA9685 /* Output controller model */
        #define OUTPUT_CONTROLLER_COUNT 1 /* The number of output controllers */
        #define OUTPUT_CONTROLLER_COUNT_PINS 6 /* Special for this model */
        #define OUTPUT_CONTROLLER_ADDRESSES {0x40} /* I2C addresses for the output controllers */
        #define OUTPUT_CONTROLLER_PORTS {1,2,4,3,5,6} /* Physical mapping of the output port numbers in sequence by output controller address, based on the output controllers' pin numbers */

        /* External EEPROM */
        #define EEPROM_EXTERNAL_MODEL ENUM_EEPROM_EXTERNAL_MODEL_24LCXXX /* External EEPROM controller model */
        #define EEPROM_EXTERNAL_WRITE_PROTECT_PIN 23 /* EEPROM write protect pin.  When low, write protect is disabled */
        #define EEPROM_EXTERNAL_ADDRESS 0x50 /* I2C addresses of the external EEPROM */

        /* Temperature Sensor */
        #define TEMPERATURE_SENSOR_MODEL ENUM_TEMPERATURE_SENSOR_MODEL_PCT2075 /* Temperature sensor model */
        #define TEMPERATURE_SENSOR_COUNT 1 /* The number of temperature sensors */
        #define TEMPERATURE_SENSOR_ADDRESSES {0x48} /* I2C addresses of the on-board temperature sensors */
        #define TEMPERATURE_SENSOR_LOCATIONS {"Center"} /* Temperature locations as an array, maximum string length = TEMPERATURE_SENSOR_LOCATION_MAX_LENGTH */

        /* OLED Display and Button */
        #define OLED_DISPLAY_MODEL ENUM_OLED_MODEL_SSD1306_128_32 /* OLED display model */
        #define OLED_BUTTON_PIN 32 /* Button for the front panel */
        #define OLED_LED_PIN 33 /* LED for the front panel button */
        
        /* Ethernet */
        #define ETHERNET_MODEL ENUM_ETHERNET_MODEL_W5500 /* Ethernet controller model */
        #define ETHERNET_PIN 25 /* Ethernet hardware control flow pin */
        #define ETHERNET_PIN_RESET 26 /* Ethernet hardware reset pin */
        #define ETHERNET_PIN_INTERRUPT 19 /* Ethernet hardware interrupt pin */
        #define ETH_SPI_HOST SPI2_HOST /* Set W5000 async library to use SPI2 */

        /* WiFi */
        #define WIFI_MODEL ENUM_WIFI_MODEL_ESP32

        /* SPI Configuration */
        #define SPI_SCK_PIN 14 /* SPI clock interface pin */
        #define SPI_MISO_PIN 12 /* SPI MISO pin */
        #define SPI_MOSI_PIN 13 /* SPI MOSI pin */

    #endif


    #if PRODUCT_HEX == 0x32322505

        #define SUPPORTED_HARDWARE
       
        /* Input IO Extender */
        #define IO_EXTENDER_MODEL ENUM_IO_EXTENDER_MODEL_PCA9995 /* IO Extender Model */
        #define IO_EXTENDER_COUNT 8 /* The number of IO extenders */
        #define IO_EXTENDER_ADDRESSES {0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27} /* I2C addresses for the IO extenders, order must match the PINS_INTERRUPT_IO_EXTENDER and should be sequential with the port numbers */
        #define IO_EXTENDER_CHANNELS {{2,1},{2,2},{2,3},{1,4},{2,4},{1,3},{1,2},{1,1},{4,1},{4,2},{4,3},{3,4},{4,4},{3,3},{3,2},{3,1}} /* Physical mapping of the {port,channel} by pin */
        #define IO_EXTENDER_INTERRUPT_PINS {34,35,33,27,15,4,5,18} /* Interrupt pins for the IO extenders, order must match the ADDRESSES_IO_EXTENDER */
        
        /* Output Controller */
        #define OUTPUT_CONTROLLER_MODEL ENUM_OUTPUT_CONTROLLER_MODEL_PCA9685 /* Output controller model */
        #define OUTPUT_CONTROLLER_COUNT 2 /* The number of output controllers */
        #define OUTPUT_CONTROLLER_ADDRESSES {0x40,0x41} /* I2C addresses for the output controllers */
        #define OUTPUT_CONTROLLER_PORTS {2,1,3,4,6,5,7,8,10,9,11,12,14,13,15,16} /* Physical mapping of the output port numbers in sequence by output controller address, based on the output controllers' pin numbers */

        /* External EEPROM */
        #define EEPROM_EXTERNAL_MODEL ENUM_EEPROM_EXTERNAL_MODEL_24LCXXX /* External EEPROM controller model */
        #define EEPROM_EXTERNAL_WRITE_PROTECT_PIN 23 /* EEPROM write protect pin.  When low, write protect is disabled */
        #define EEPROM_EXTERNAL_ADDRESS 0x50 /* I2C addresses of the external EEPROM */

        /* Temperature Sensor */
        #define TEMPERATURE_SENSOR_MODEL ENUM_TEMPERATURE_SENSOR_MODEL_PCT2075 /* Temperature sensor model */
        #define TEMPERATURE_SENSOR_COUNT 1 /* The number of temperature sensors */
        #define TEMPERATURE_SENSOR_ADDRESSES {0x48} /* I2C addresses of the on-board temperature sensors */
        #define TEMPERATURE_SENSOR_LOCATIONS {"Center"} /* Temperature locations as an array, maximum string length = TEMPERATURE_SENSOR_LOCATION_MAX_LENGTH */

        /* OLED Display and Button */
        #define OLED_DISPLAY_MODEL ENUM_OLED_MODEL_SSD1306_128_32 /* OLED display model */
        #define OLED_BUTTON_PIN 19 /* Button for the front panel */
        #define OLED_LED_PIN 2 /* LED for the front panel button */
        
        /* Ethernet */
        #define ETHERNET_MODEL ENUM_ETHERNET_MODEL_W5500 /* Ethernet controller model */
        #define ETHERNET_PIN 25 /* Ethernet hardware control flow pin */
        #define ETHERNET_PIN_RESET 26 /* Ethernet hardware reset pin */
        #define ETHERNET_PIN_INTERRUPT 32 /* Ethernet hardware interrupt pin */
        #define ETH_SPI_HOST SPI2_HOST /* Set W5000 async library to use SPI2 */

        /* WiFi */
        #define WIFI_MODEL ENUM_WIFI_MODEL_ESP32

        /* SPI Configuration */
        #define SPI_SCK_PIN 14 /* SPI clock interface pin */
        #define SPI_MISO_PIN 12 /* SPI MISO pin */
        #define SPI_MOSI_PIN 13 /* SPI MOSI pin */

    #endif


    #if PRODUCT_HEX == 0x08062505

        #define SUPPORTED_HARDWARE
      
        /* Input IO Extender */
        #define IO_EXTENDER_MODEL ENUM_IO_EXTENDER_MODEL_PCA9995 /* IO Extender Model */
        #define IO_EXTENDER_COUNT 2 /* The number of IO extenders */
        #define IO_EXTENDER_ADDRESSES {0x20,0x21} /* I2C addresses for the IO extenders, order must match the PINS_INTERRUPT_IO_EXTENDER and should be sequential with the port numbers */
        #define IO_EXTENDER_CHANNELS {{2,1},{2,2},{2,3},{1,4},{2,4},{1,3},{1,2},{1,1},{4,1},{4,2},{4,3},{3,4},{4,4},{3,3},{3,2},{3,1}} /* Physical mapping of the {port,channel} by pin */
        #define IO_EXTENDER_INTERRUPT_PINS {34,35} /* Interrupt pins for the IO extenders, order must match the ADDRESSES_IO_EXTENDER */
        
        /* Output Controller */
        #define OUTPUT_CONTROLLER_MODEL ENUM_OUTPUT_CONTROLLER_MODEL_PCA9685 /* Output controller model */
        #define OUTPUT_CONTROLLER_COUNT 1 /* The number of output controllers */
        #define OUTPUT_CONTROLLER_COUNT_PINS 6 /* Special for this model */
        #define OUTPUT_CONTROLLER_ADDRESSES {0x40} /* I2C addresses for the output controllers */
        #define OUTPUT_CONTROLLER_PORTS {1,2,4,3,5,6} /* Physical mapping of the output port numbers in sequence by output controller address, based on the output controllers' pin numbers */

        /* External EEPROM */
        #define EEPROM_EXTERNAL_MODEL ENUM_EEPROM_EXTERNAL_MODEL_24LCXXX /* External EEPROM controller model */
        #define EEPROM_EXTERNAL_WRITE_PROTECT_PIN 23 /* EEPROM write protect pin.  When low, write protect is disabled */
        #define EEPROM_EXTERNAL_ADDRESS 0x50 /* I2C addresses of the external EEPROM */

        /* Temperature Sensor */
        #define TEMPERATURE_SENSOR_MODEL ENUM_TEMPERATURE_SENSOR_MODEL_PCT2075 /* Temperature sensor model */
        #define TEMPERATURE_SENSOR_COUNT 1 /* The number of temperature sensors */
        #define TEMPERATURE_SENSOR_ADDRESSES {0x48} /* I2C addresses of the on-board temperature sensors */
        #define TEMPERATURE_SENSOR_LOCATIONS {"Center"} /* Temperature locations as an array, maximum string length = TEMPERATURE_SENSOR_LOCATION_MAX_LENGTH */

        /* OLED Display and Button */
        #define OLED_DISPLAY_MODEL ENUM_OLED_MODEL_SSD1306_128_32 /* OLED display model */
        #define OLED_BUTTON_PIN 32 /* Button for the front panel */
        #define OLED_LED_PIN 2 /* LED for the front panel button */
        
        /* Ethernet */
        #define ETHERNET_MODEL ENUM_ETHERNET_MODEL_W5500 /* Ethernet controller model */
        #define ETHERNET_PIN 25 /* Ethernet hardware control flow pin */
        #define ETHERNET_PIN_RESET 26 /* Ethernet hardware reset pin */
        #define ETHERNET_PIN_INTERRUPT 19 /* Ethernet hardware interrupt pin */
        #define ETH_SPI_HOST SPI2_HOST /* Set W5000 async library to use SPI2 */

        /* WiFi */
        #define WIFI_MODEL ENUM_WIFI_MODEL_ESP32

        /* SPI Configuration */
        #define SPI_SCK_PIN 14 /* SPI clock interface pin */
        #define SPI_MISO_PIN 12 /* SPI MISO pin */
        #define SPI_MOSI_PIN 13 /* SPI MOSI pin */

    #endif


    #ifndef SUPPORTED_HARDWARE
        #error Build failed, Unknown PRODUCT_HEX. Ensure it was set in ./.vscode/arduino.json {"buildPreferences":[["build.extra_flags","-DPRODUCT_HEX=0x000000000"]]}
    #endif


    /* Include hardware-specific libraries */

    #ifndef DEBOUNCE_DELAY
        #define DEBOUNCE_DELAY 350 /* Milliseconds between changes for debouncing on the front panel button. */
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
        #define SSD1306_NO_SPLASH

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
        #include <LittleFS.h>
        #include <Regexp.h>

        #define _ETHERNET_WEBSERVER_LOGLEVEL_ CORE_DEBUG_LEVEL /* Match the Ethernet debug level to the core debug level */
       
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
        #define ETHERNET_TIMEOUT 10000 /* Number of milliseconds before Ethernet will time out */
    #endif


    #if WIFI_MODEL != ENUM_WIFI_MODEL_NONE
        #include <WiFi.h>

        #if WIFI_MODEL == ENUM_WIFI_MODEL_ESP32
            #ifndef _AsyncWebServer_ESP32_W5500_H_
                #include <ESPAsyncWebServer.h>
            #endif
            #include <AsyncTCP.h>
            #include <LittleFS.h>
            #include <Regexp.h>
        #endif

        #ifndef WIFI_TIMEOUT
            #define WIFI_TIMEOUT 10000 /* Number of milliseconds before WiFi will time out */
        #endif

    #endif


    #ifdef ESP32
        #include <esp_chip_info.h>
    #endif


    #ifndef EVENT_LOG_MAXIMUM_ENTRIES
        #define EVENT_LOG_MAXIMUM_ENTRIES 20 /* Maximum number of entries the event log will retain. */
    #endif


    #ifndef EVENT_LOG_ENTRY_MAX_LENGTH
        #define EVENT_LOG_ENTRY_MAX_LENGTH OLED_CHARACTERS_PER_LINE /* Maximum length of the event log text */
    #endif


    #ifndef PORT_ID_MAX_LENGTH
        #define PORT_ID_MAX_LENGTH 8 /* Maximum number of characters in a port's ID; must match Swagger */
    #endif


    #ifndef OUTPUT_ID_MAX_LENGTH
        #define OUTPUT_ID_MAX_LENGTH 8 /* Maximum number of characters in an output's ID; must match Swagger */
    #endif


    #ifndef OUTPUT_NAME_MAX_LENGTH
        #define OUTPUT_NAME_MAX_LENGTH 20 /* Maximum number of characters in an output's name; must match Swagger */
    #endif


    #ifndef MQTT_RECONNECT_WAIT_MILLISECONDS
        #define MQTT_RECONNECT_WAIT_MILLISECONDS 5000 /* Number of milliseconds to wait between MQTT reconnect attempts */
    #endif


    #ifndef HARDWARE_MANUFACTURER_NAME
        #define HARDWARE_MANUFACTURER_NAME "P5 Software LLC"
    #endif


    #ifndef CONTROLLER_NAME_MAX_LENGTH
        #define CONTROLLER_NAME_MAX_LENGTH 20 /* Maximum number of characters in a controller's device name; must match Swagger */
    #endif


    #ifndef CONTROLLER_AREA_MAX_LENGTH
        #define CONTROLLER_AREA_MAX_LENGTH 20 /* Maximum number of characters in a controller's area; must match Swagger */
    #endif


    #ifndef LFS_NAME_MAX
        #define LFS_NAME_MAX 64 /* Set LittleFS filename max length to 64 characters */
    #endif


    #ifndef MEMORY_USAGE_REPORT_SECONDS
        #define MEMORY_USAGE_REPORT_SECONDS 15 /* Number of seconds between each memory usage report in the main loop */
    #endif


    #ifndef TEMPERATURE_SENSOR_LOCATION_MAX_LENGTH
        #define TEMPERATURE_SENSOR_LOCATION_MAX_LENGTH 8 /* Maximum length for a temperature sensor location */
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

    #if WIFI_MODEL == ENUM_WIFI_MODEL_NONE && ETHERNET_MODEL == ENUM_ETHERNET_MODEL_NONE
        #error WIFI_MODEL and ETHERNET_MODEL are both set to NONE
    #endif

    #if EVENT_LOG_MAXIMUM_ENTRIES > 255
        #error EVENT_LOG_MAXIMUM_ENTRIES cannot be > 255 because it depends on size uint8_t
    #endif


    /** Common health status for all peripherals. */
    struct structHealth{
        uint8_t address; /* I2C address.*/
        bool enabled; /* Indicates if the controller is enabled.*/
    };
    
#endif