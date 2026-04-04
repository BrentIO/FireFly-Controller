#pragma once

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
