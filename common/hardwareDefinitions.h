/*
    Hardware Definitions
*/

#ifndef hardware_definitions_h
    #define hardware_definitions_h

    #include <PCA95x5.h> // https://github.com/semcneil/PCA95x5


    /* Define hardware enumeration constants */
    #define ENUM_MODEL_IO_EXTENDER_PCA9995 0 /* PCA9555 */
    #define ENUM_MODEL_OUTPUT_CONTROLLER_PCA9685 0 /* PCA9685 */

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
        
        /* Pin Addresses */
        #define PIN_EEPROM_WP 23 /* EEPROM write protect pin.  When low, write protect is disabled. */
        #define PIN_ETHERNET 0 /* Ethernet hardware control flow pin. */
        #define PIN_OLED_BUTTON 32 /* Button for the front panel. */
        #define PIN_OLED_LED 33 /* LED for the front panel button. */
        #define _PINS_INTERRUPT_IO_EXTENDER {35,25,26,27,4,5,18,19} /* Interrupt pins for the IO extenders, order must match the _ADDRESSES_IO_EXTENDER. */

        /* I2C Addresses */
        #define _ADDRESSES_IO_EXTENDER {0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27} /* I2C addresses for the IO extenders, order must match the _PINS_INTERRUPT_IO_EXTENDER. */
        #define _ADDRESSES_OUTPUT_CONTROLLER {0x40,0x42} /* I2C addresses for the output controllers. */
        #define ADDRESS_TEMPERATURE 0x48 /* I2C addresses of the on-board temperature sensor. */
        #define ADDRESS_EEPROM 0x50 /* I2C addresses of the external EEPROM. */
        #define ADDRESS_OLED 0x3C /* I2C addresses of the OLED Display.  NOTE: This can be modifed on the display itself, default is 0x3C. */

        #define SIZE_EEPROM 256 /* Six of the external EEPROM.  NOTE: Divide the device size in Kbits by 8. */

    #endif

    #ifndef SUPPORTED_HARDWARE

        #error Build failed, Unknown PRODUCT_ID. Ensure it was set in ./.vscode/arduino.json {"buildPreferences":[["build.extra_flags","-DPRODUCT_ID=000000000"]]}

    #endif


    const uint8_t PINS_IO_EXTENDER[] = _PINS_INTERRUPT_IO_EXTENDER;
    const uint8_t ADDRESSES_IO_EXTENDER[] = _ADDRESSES_IO_EXTENDER;
    const uint8_t ADDRESSES_OUTPUT_CONTROLLER[] = _ADDRESSES_OUTPUT_CONTROLLER;


 typedef struct {
    char uuid[37];
    char product_id[33];
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


  /* Input-specifc structures */
  #if MODEL_IO_EXTENDER == ENUM_MODEL_IO_EXTENDER_PCA9555

      #define DEBOUNCE_DELAY 500 /* Milliseconds between changes for debouncing. */

      struct ioExtender{
          PCA9555 hardware; /* Reference to the hardware. */
          uint8_t interruptPin = 0; /* Interrupt pin. Default 0. */
          uint8_t address = 0; /* I2C address. Default 0.*/
          uint16_t previousRead = 0; /* Numeric value of the last read from the hardware. Default 0.*/
          inputPin inputs[COUNT_PINS_IO_EXTENDER]; /* Input pins connected to the hardware.*/
      };

  #endif


  /* Output-specifc structures */
  #if MODEL_OUTPUT_CONTROLLER == ENUM_MODEL_OUTPUT_CONTROLLER_PCA9685

      struct outputController{
          uint8_t address = 0; /* I2C address. Default 0.*/
          outputPin outputs[COUNT_PINS_OUTPUT_CONTROLLER];
      };

  #endif

#endif