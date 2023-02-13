#include <PCA95x5.h> // https://github.com/semcneil/PCA95x5
#include <PCA9685.h> // https://github.com/RobTillaart/PCA9685_RT
#include <PCT2075.h> // https://github.com/jpliew/PCT2075
#include <I2C_eeprom.h> // https://github.com/RobTillaart/I2C_EEPROM
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include "common/hardwareDefinitions.h"


ioExtender inputControllers[COUNT_IO_EXTENDER];
outputController outputControllers[COUNT_OUTPUT_CONTROLLER];


#define DEBUG 1000


void setup() {

    #ifdef DEBUG
      Serial.begin(115200);
    #endif

    Wire.begin();

    //Instantiate the input objects
    #if MODEL_IO_EXTENDER == ENUM_MODEL_IO_EXTENDER_PCA9555

      for(int i = 0; i < COUNT_IO_EXTENDER; i++){

        pinMode(inputControllers[i].interruptPin, INPUT);

        inputControllers[i].interruptPin = PINS_IO_EXTENDER[i];
        inputControllers[i].address = ADDRESSES_IO_EXTENDER[i];
        inputControllers[i].hardware.attach(Wire, inputControllers[i].address);
        inputControllers[i].hardware.polarity(PCA95x5::Polarity::INVERTED_ALL);
        inputControllers[i].hardware.direction(PCA95x5::Direction::IN_ALL);

        //Get the current input states and ignore the debounce delays
        checkInputPins(i, false);

      }

    #endif

    //Instantiate the output objects
    for(int i = 0; i < COUNT_OUTPUT_CONTROLLER; i++){
      outputControllers[i].address = ADDRESSES_OUTPUT_CONTROLLER[i];
    }

    pinMode(PIN_OLED_LED, OUTPUT);

    digitalWrite(PIN_OLED_LED, HIGH); //Turn on the OLED button LED, we are ready
}

void loop() {

  checkInputs();  

}

void checkInputs(){

  for(int i = 0; i < COUNT_IO_EXTENDER; i++){
    
    //Need to read each input pin for LOW so we can detect intra-PCA9555 button press changes
    if(digitalRead(inputControllers[i].interruptPin) == LOW){
      checkInputPins(i);
    }

  }

}

void checkInputPins(int ioExtenderIndex){

  checkInputPins(ioExtenderIndex, true);

}


/** Checks the pins on the ioExtenderIndex IO extender for changes */
void checkInputPins(int ioExtenderIndex, boolean respectDebounceDelay){

  #ifdef DEBUG
    #if DEBUG > 50
      if(respectDebounceDelay == false){
        Serial.println("(checkInputPins) IO Extender: 0x" + String(inputControllers[ioExtenderIndex].address, HEX) + " Ignoring debounce delay.");
      }
    #endif
  #endif

  uint16_t pinRead = 0;

  #if MODEL_IO_EXTENDER == ENUM_MODEL_IO_EXTENDER_PCA9555

    pinRead = inputControllers[ioExtenderIndex].hardware.read();

  #endif

  //Exit if the value returned from the controller is the same as the value that was previously read
  if(pinRead == inputControllers[ioExtenderIndex].previousRead){
    return;
  }

  //Process each pin on the specified IO extender
  for(int i = 0; i < COUNT_PINS_IO_EXTENDER; i++){

    inputState currentState = bitReadToInputState(bitRead(pinRead, i));

    //Check if the value returned in the read is the same as the last read
    if(inputControllers[ioExtenderIndex].inputs[i].state == currentState){
      #ifdef DEBUG
        #if DEBUG > 50
          Serial.println("(checkInputPins) IO Extender: 0x" + String(inputControllers[ioExtenderIndex].address, HEX) + " Pin: " + String(i) + " input states match. Previous: " + String(inputControllers[ioExtenderIndex].inputs[i].state, HEX) + " Current: " + String(currentState, HEX));
        #endif
      #endif
      continue;
    }

    //Values are different; Check if we are within the debounce delay
    if(millis() - inputControllers[ioExtenderIndex].inputs[i].timePreviousChange < DEBOUNCE_DELAY){

      //Check if the debounce delay should be checked (ignored on startup)
      if(respectDebounceDelay == true){
        #ifdef DEBUG
          #if DEBUG > 50
            Serial.println("(checkInputPins) IO Extender: 0x" + String(inputControllers[ioExtenderIndex].address, HEX) + " Pin: " + String(i) + " DEBOUNCE_DELAY (" + String(DEBOUNCE_DELAY) + ") not satisfied. Time Previous Change: " + String(inputControllers[ioExtenderIndex].inputs[i].timePreviousChange) + " Current Time: " + String(millis()) + " Difference: " + String(millis() - inputControllers[ioExtenderIndex].inputs[i].timePreviousChange));
          #endif
        #endif

        continue;
      }

    }

    switch(inputControllers[ioExtenderIndex].inputs[i].type){

      case inputType::NORMALLY_OPEN:

        //Check if input is in an abnormal state
        if(currentState == inputState::STATE_CLOSED){

          inputControllers[ioExtenderIndex].inputs[i].timePreviousChange = millis();

          #ifdef DEBUG
            #if DEBUG > 50
              Serial.println("(checkInputPins) IO Extender: 0x" + String(inputControllers[ioExtenderIndex].address, HEX) + " Pin: " + String(i) + " Type: " + String(inputControllers[ioExtenderIndex].inputs[i].type) + " New State: " + String(currentState) + " (Abnormal)");
            #endif
          #endif          
       
          //do something
          break;
        }

        //Check if input is in normal state
        if(currentState == inputState::STATE_OPEN){
          
          inputControllers[ioExtenderIndex].inputs[i].timePreviousChange = 0;

          #ifdef DEBUG
            #if DEBUG > 50
              Serial.println("(checkInputPins) IO Extender: 0x" + String(inputControllers[ioExtenderIndex].address, HEX) + " Pin: " + String(i) + " Type: " + String(inputControllers[ioExtenderIndex].inputs[i].type) + " New State: " + String(currentState) + " (Normal)");
            #endif
          #endif
         
          //do something
          break;
        }

      case inputType::NORMALLY_CLOSED:

        //Check if input is in an abnormal state
        if(currentState == inputState::STATE_OPEN){

          inputControllers[ioExtenderIndex].inputs[i].timePreviousChange = millis();
          
          #ifdef DEBUG
            #if DEBUG > 50
              Serial.println("(checkInputPins) IO Extender: " + String(inputControllers[ioExtenderIndex].address, HEX) + " Pin: " + String(i) + " Type: " + String(inputControllers[ioExtenderIndex].inputs[i].type) + " New State: " + String(currentState) + " (Abnormal)");
            #endif
          #endif
          
          //do something
          break;
        }

        //Check if input is in normal state
        if(currentState == inputState::STATE_CLOSED){
          
          inputControllers[ioExtenderIndex].inputs[i].timePreviousChange = 0;

          #ifdef DEBUG
            #if DEBUG > 50
              Serial.println("(checkInputPins) IO Extender: " + String(inputControllers[ioExtenderIndex].address, HEX) + " Pin: " + String(i) + " Type: " + String(inputControllers[ioExtenderIndex].inputs[i].type) + " New State: " + String(currentState) + " (Normal)");
            #endif
          #endif

          //do something
          break;
        }

    }

    //Set the value of the pin to the current state
    inputControllers[ioExtenderIndex].inputs[i].state = currentState;

  }


  //Set the controller's value to the updated value
  inputControllers[ioExtenderIndex].previousRead = pinRead;
}

/** Convert a boolean value to an inputState type */
inputState bitReadToInputState(boolean value){

  if(value == LOW){
    return inputState::STATE_OPEN;
  }else{
    return inputState::STATE_CLOSED;
  }

}