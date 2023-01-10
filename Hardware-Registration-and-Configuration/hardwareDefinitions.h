/*
    Hardware Definitions
*/

#ifndef hardware_definitions_h
#define hardware_definitions_h


#if PRODUCT_ID == 32322211

    #define _SUPPORTED_HARDWARE

    /* Capabilities */
    #define _INPUT_CONTROLLER_COUNT 8;
    #define _OUTPUT_CONTROLLER_COUNT 2;

    /* Pin Addresses */
    #define _PIN_EEPROM_WP 23;
    #define _PIN_ETHERNET 0; //Hardware defect, not connected
    #define _PIN_OLED_BUTTON 32;

    #define _PIN_INPUT_PORTS_GROUP1_INTERRUPT 35; //U3
    #define _PIN_INPUT_PORTS_GROUP2_INTERRUPT 34; //U4
    #define _PIN_INPUT_PORTS_GROUP3_INTERRUPT 26; //U5
    #define _PIN_INPUT_PORTS_GROUP4_INTERRUPT 27; //U6
    #define _PIN_INPUT_PORTS_GROUP5_INTERRUPT 4; //U7
    #define _PIN_INPUT_PORTS_GROUP6_INTERRUPT 5; //U8
    #define _PIN_INPUT_PORTS_GROUP7_INTERRUPT 18; //U9
    #define _PIN_INPUT_PORTS_GROUP8_INTERRUPT 19; //U10

    /* I2C Addresses */
    #define _ADDRESS_TEMPERATURE 0x48;
    #define _ADDRESS_EEPROM 0x50;
    #define _ADDRESS_OLED 0x3C; //Default display configuration, consult display hardware

    #define _ADDRESS_INPUT_PORTS_GROUP1 0x20; //U3
    #define _ADDRESS_INPUT_PORTS_GROUP2 0x21; //U4
    #define _ADDRESS_INPUT_PORTS_GROUP3 0x22; //U5
    #define _ADDRESS_INPUT_PORTS_GROUP4 0x23; //U6
    #define _ADDRESS_INPUT_PORTS_GROUP5 0x24; //U7
    #define _ADDRESS_INPUT_PORTS_GROUP6 0x25; //U8
    #define _ADDRESS_INPUT_PORTS_GROUP7 0x26; //U9
    #define _ADDRESS_INPUT_PORTS_GROUP8 0x27; //U10

    #define _ADDRESS_OUTPUT_PORTS_GROUP1 0x40; //U1
    #define _ADDRESS_OUTPUT_PORTS_GROUP2 0x42; //U2     Hardware defect, should be 41
    #define _ADDRESS_OUTPUT_ALL 0x70;

    #define _SIZE_EEPROM 256; //Max size in bytes of the device (divide your device size in Kbits by 8)

#endif

#if PRODUCT_ID == 32322301

    #define _SUPPORTED_HARDWARE

    /* Capabilities */
    #define _INPUT_CONTROLLER_COUNT 8;
    #define _OUTPUT_CONTROLLER_COUNT 2;

    /* Pin Addresses */
    #define _PIN_EEPROM_WP 23;
    #define _PIN_ETHERNET 25;
    #define _PIN_OLED_BUTTON 32;

    #define _PIN_INPUT_PORTS_GROUP1_INTERRUPT 35; //U3
    #define _PIN_INPUT_PORTS_GROUP2_INTERRUPT 34; //U4
    #define _PIN_INPUT_PORTS_GROUP3_INTERRUPT 26; //U5
    #define _PIN_INPUT_PORTS_GROUP4_INTERRUPT 27; //U6
    #define _PIN_INPUT_PORTS_GROUP5_INTERRUPT 4; //U7
    #define _PIN_INPUT_PORTS_GROUP6_INTERRUPT 5; //U8
    #define _PIN_INPUT_PORTS_GROUP7_INTERRUPT 18; //U9
    #define _PIN_INPUT_PORTS_GROUP8_INTERRUPT 19; //U10

    /* I2C Addresses */
    #define _ADDRESS_TEMPERATURE 0x48;
    #define _ADDRESS_EEPROM 0x50;
    #define _ADDRESS_OLED 0x3C; //Default display configuration, consult display hardware

    #define _ADDRESS_INPUT_PORTS_GROUP1 0x20; //U3
    #define _ADDRESS_INPUT_PORTS_GROUP2 0x21; //U4
    #define _ADDRESS_INPUT_PORTS_GROUP3 0x22; //U5
    #define _ADDRESS_INPUT_PORTS_GROUP4 0x23; //U6
    #define _ADDRESS_INPUT_PORTS_GROUP5 0x24; //U7
    #define _ADDRESS_INPUT_PORTS_GROUP6 0x25; //U8
    #define _ADDRESS_INPUT_PORTS_GROUP7 0x26; //U9
    #define _ADDRESS_INPUT_PORTS_GROUP8 0x27; //U10

    #define _ADDRESS_OUTPUT_PORTS_GROUP1 0x40; //U1
    #define _ADDRESS_OUTPUT_PORTS_GROUP2 0x41; //U2
    #define _ADDRESS_OUTPUT_ALL 0x70;

    #define _SIZE_EEPROM 256; //Max size in bytes of the device (divide your device size in Kbits by 8)

#endif


#ifndef _SUPPORTED_HARDWARE

    #error Build failed, Unknown PRODUCT_ID. Ensure it was set in ./.vscode/arduino.json {"buildPreferences":[["build.extra_flags","-DPRODUCT_ID=000000000"]]}

#endif

static const unsigned int INPUT_CONTROLLER_COUNT = _INPUT_CONTROLLER_COUNT;
static const unsigned int OUTPUT_CONTROLLER_COUNT = _OUTPUT_CONTROLLER_COUNT;

static const unsigned int PIN_EEPROM_WP = _PIN_EEPROM_WP;
static const unsigned int PIN_ETHERNET = _PIN_ETHERNET;
static const unsigned int PIN_OLED_BUTTON = _PIN_OLED_BUTTON;

static const unsigned int PIN_INPUT_PORTS_GROUP1_INTERRUPT = _PIN_INPUT_PORTS_GROUP1_INTERRUPT;
static const unsigned int PIN_INPUT_PORTS_GROUP2_INTERRUPT = _PIN_INPUT_PORTS_GROUP2_INTERRUPT;
static const unsigned int PIN_INPUT_PORTS_GROUP3_INTERRUPT = _PIN_INPUT_PORTS_GROUP3_INTERRUPT;
static const unsigned int PIN_INPUT_PORTS_GROUP4_INTERRUPT = _PIN_INPUT_PORTS_GROUP4_INTERRUPT;
static const unsigned int PIN_INPUT_PORTS_GROUP5_INTERRUPT = _PIN_INPUT_PORTS_GROUP5_INTERRUPT;
static const unsigned int PIN_INPUT_PORTS_GROUP6_INTERRUPT = _PIN_INPUT_PORTS_GROUP6_INTERRUPT;
static const unsigned int PIN_INPUT_PORTS_GROUP7_INTERRUPT = _PIN_INPUT_PORTS_GROUP7_INTERRUPT;
static const unsigned int PIN_INPUT_PORTS_GROUP8_INTERRUPT = _PIN_INPUT_PORTS_GROUP8_INTERRUPT;

static const unsigned int ADDRESS_TEMPERATURE = _ADDRESS_TEMPERATURE;
static const unsigned int ADDRESS_EEPROM = _ADDRESS_EEPROM;

static const unsigned int ADDRESS_OLED = _ADDRESS_OLED;

static const unsigned int ADDRESS_INPUT_PORTS_GROUP1 = _ADDRESS_INPUT_PORTS_GROUP1;
static const unsigned int ADDRESS_INPUT_PORTS_GROUP2 = _ADDRESS_INPUT_PORTS_GROUP2;
static const unsigned int ADDRESS_INPUT_PORTS_GROUP3 = _ADDRESS_INPUT_PORTS_GROUP3;
static const unsigned int ADDRESS_INPUT_PORTS_GROUP4 = _ADDRESS_INPUT_PORTS_GROUP4;
static const unsigned int ADDRESS_INPUT_PORTS_GROUP5 = _ADDRESS_INPUT_PORTS_GROUP5;
static const unsigned int ADDRESS_INPUT_PORTS_GROUP6 = _ADDRESS_INPUT_PORTS_GROUP6;
static const unsigned int ADDRESS_INPUT_PORTS_GROUP7 = _ADDRESS_INPUT_PORTS_GROUP7;
static const unsigned int ADDRESS_INPUT_PORTS_GROUP8 = _ADDRESS_INPUT_PORTS_GROUP8;

static const unsigned int ADDRESS_OUTPUT_PORTS_GROUP1 = _ADDRESS_OUTPUT_PORTS_GROUP1;
static const unsigned int ADDRESS_OUTPUT_PORTS_GROUP2 = _ADDRESS_OUTPUT_PORTS_GROUP2;
static const unsigned int ADDRESS_OUTPUT_ALL = _ADDRESS_OUTPUT_ALL;

static const unsigned int SIZE_EEPROM = _SIZE_EEPROM;

#endif