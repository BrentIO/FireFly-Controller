#define DEBUG 400

#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include "common/hardwareDefinitions.h"


ioExtender inputControllers[COUNT_IO_EXTENDER];
outputController outputControllers[COUNT_OUTPUT_CONTROLLER];
managerTemperatureSensors temperatureSensors;
managerFrontPanelButton frontPanelButton;


void setup() {

    #ifdef DEBUG
      Serial.begin(115200);
    #endif

    Wire.begin();

    //Configure the peripherals
    frontPanelButton.begin();

    setupInputs();
    setupOutputs();
    temperatureSensors.setCallback_publisher(&temperaturePublisher);
    temperatureSensors.setCallback_failure(&temperatureFailure);
    temperatureSensors.begin();

    //System has started, show normal state
    frontPanelButton.setLED(managerFrontPanelButton::status::NORMAL);

}


void loop() {

  loopInputs();
  temperatureSensors.loop();

}


/**Instantiate the input objects*/
void setupInputs(){

  //Setup the input controllers
  for(int i = 0; i < COUNT_IO_EXTENDER; i++){

    pinMode(inputControllers[i].interruptPin, INPUT);

    inputControllers[i].interruptPin = PINS_IO_EXTENDER[i];
    inputControllers[i].address = ADDRESSES_IO_EXTENDER[i];
    
    #if MODEL_IO_EXTENDER == ENUM_MODEL_IO_EXTENDER_PCA9555
      inputControllers[i].hardware.attach(Wire, inputControllers[i].address);
      inputControllers[i].hardware.polarity(PCA95x5::Polarity::INVERTED_ALL);
      inputControllers[i].hardware.direction(PCA95x5::Direction::IN_ALL);

      //Ensure we connected to the controller
      if(inputControllers[i].hardware.i2c_error() != 0){
        handleInputFailure(&inputControllers[i]);
      }

    #endif

    //Get the current input states and ignore the debounce delays
    readInputPins(&inputControllers[i], true);
  }

}


/**Instantiates the output objects*/
void setupOutputs(){

  //Setup output controllers
  for(int i = 0; i < COUNT_OUTPUT_CONTROLLER; i++){
    outputControllers[i].address = ADDRESSES_OUTPUT_CONTROLLER[i];

    #if MODEL_OUTPUT_CONTROLLER == ENUM_MODEL_OUTPUT_CONTROLLER_PCA9685
      outputControllers[i].hardware = PCA9685(outputControllers[i].address);
      outputControllers[i].hardware.begin();
      outputControllers[i].hardware.setFrequency(FREQUENCY_PWM);

      //Ensure the controller is online
      if(outputControllers[i].hardware.isConnected() == false){       
        handleOutputFailure(&outputControllers[i]);
      }

    #endif

  }

}


/**Checks each interrupt pin between the IO Extender and the ESP32*/
void loopInputs(){

  for(int i = 0; i < COUNT_IO_EXTENDER; i++){

    //Ignore disabled input controllers
    if(inputControllers[i].enabled == false){
      continue;
    }
    
    //Need to read each input pin for LOW so we can detect intra-PCA9555 button press changes
    if(digitalRead(inputControllers[i].interruptPin) == LOW){
      readInputPins(&inputControllers[i]);
    }
  }

}


/** Checks the pins on the ioExtenderIndex IO extender for changes */
void readInputPins(ioExtender *ioExtender){
  readInputPins(ioExtender, false);
}


/** Checks the pins on the ioExtenderIndex IO extender for changes, optionally ignoring the debounce delay */
void readInputPins(ioExtender *ioExtender, boolean ignoreDebounceDelay){

  #ifdef DEBUG
    #if DEBUG > 50
      if(ignoreDebounceDelay == true){
        Serial.println("(readInputPins) IO Extender: 0x" + String(ioExtender->address, HEX) + " Ignoring debounce delay.");
      }
    #endif
  #endif

  uint16_t pinRead = 0;

  #if MODEL_IO_EXTENDER == ENUM_MODEL_IO_EXTENDER_PCA9555

    pinRead = ioExtender->hardware.read();

  #endif

  //Exit if the value returned from the controller is the same as the value that was previously read
  if(pinRead == ioExtender->previousRead){
    return;
  }

  //Process each pin on the specified IO extender
  for(int i = 0; i < COUNT_PINS_IO_EXTENDER; i++){

    inputState currentState = bitToInputState(bitRead(pinRead, i));

    //Check if the value returned in the read is the same as the last read
    if(ioExtender->inputs[i].state == currentState){
      #ifdef DEBUG
        #if DEBUG > 500
          Serial.println("(readInputPins) IO Extender: 0x" + String(ioExtender->address, HEX) + " Pin: " + String(i) + " input states match. Previous: " + String(ioExtender->inputs[i].state, HEX) + " Current: " + String(currentState, HEX));
        #endif
      #endif
      continue;
    }

    //Values are different; Check if we are within the debounce delay
    if(millis() - ioExtender->inputs[i].timePreviousChange < DEBOUNCE_DELAY){

      //Check if the debounce delay should be checked (ignored on startup)
      if(ignoreDebounceDelay == false){
        #ifdef DEBUG
          #if DEBUG > 50
            Serial.println("(readInputPins) IO Extender: 0x" + String(ioExtender->address, HEX) + " Pin: " + String(i) + " DEBOUNCE_DELAY (" + String(DEBOUNCE_DELAY) + ") not satisfied. Time Previous Change: " + String(ioExtender->inputs[i].timePreviousChange) + " Current Time: " + String(millis()) + " Difference: " + String(millis() - ioExtender->inputs[i].timePreviousChange));
          #endif
        #endif

        continue;
      }

    }

    switch(ioExtender->inputs[i].type){

      case inputType::NORMALLY_OPEN:

        //Check if input is in an abnormal state
        if(currentState == inputState::STATE_CLOSED){

          ioExtender->inputs[i].timePreviousChange = millis();

          #ifdef DEBUG
            #if DEBUG > 50
              Serial.println("(readInputPins) IO Extender: 0x" + String(ioExtender->address, HEX) + " Pin: " + String(i) + " Type: " + String(ioExtender->inputs[i].type) + " New State: " + String(currentState) + " (Abnormal)");
            #endif
          #endif          
       
          //do something
          break;
        }

        //Check if input is in normal state
        if(currentState == inputState::STATE_OPEN){
          
          ioExtender->inputs[i].timePreviousChange = 0;

          #ifdef DEBUG
            #if DEBUG > 50
              Serial.println("(readInputPins) IO Extender: 0x" + String(ioExtender->address, HEX) + " Pin: " + String(i) + " Type: " + String(ioExtender->inputs[i].type) + " New State: " + String(currentState) + " (Normal)");
            #endif
          #endif
         
          //do something
          break;
        }

      case inputType::NORMALLY_CLOSED:

        //Check if input is in an abnormal state
        if(currentState == inputState::STATE_OPEN){

          ioExtender->inputs[i].timePreviousChange = millis();
          
          #ifdef DEBUG
            #if DEBUG > 50
              Serial.println("(readInputPins) IO Extender: " + String(ioExtender->address, HEX) + " Pin: " + String(i) + " Type: " + String(ioExtender->inputs[i].type) + " New State: " + String(currentState) + " (Abnormal)");
            #endif
          #endif
          
          //do something
          break;
        }

        //Check if input is in normal state
        if(currentState == inputState::STATE_CLOSED){
          
          ioExtender->inputs[i].timePreviousChange = 0;

          #ifdef DEBUG
            #if DEBUG > 50
              Serial.println("(readInputPins) IO Extender: " + String(ioExtender->address, HEX) + " Pin: " + String(i) + " Type: " + String(ioExtender->inputs[i].type) + " New State: " + String(currentState) + " (Normal)");
            #endif
          #endif

          //do something
          break;
        }

    }

    //Set the value of the pin to the current state
    ioExtender->inputs[i].state = currentState;

  }


  //Set the controller's value to the updated value
  ioExtender->previousRead = pinRead;
}


/** Convert a bit to inputState */
inputState bitToInputState(boolean value){

  if(value == LOW){
    return inputState::STATE_OPEN;
  }else{
    return inputState::STATE_CLOSED;
  }

}

void temperaturePublisher(String location, float value){

  #ifdef DEBUG
    Serial.println("New Temperature: " + String(value) + " at " + location);
  #endif

  //TODO: Add MQTT and stuff

};

void temperatureFailure(String location){

  #ifdef DEBUG
    Serial.println("Temperature sensor at " + location + " is offline.");
  #endif

  //TODO: Add MQTT and stuff

};


/** Handles failures of input controllers */
void handleInputFailure(ioExtender *controller){

  #ifdef DEBUG
    Serial.println("Input controller at 0x" + String(controller->address, HEX) + " is offline.");
  #endif

  //Disable the controller
  controller->enabled = false;

  //Set the LED to trouble
  frontPanelButton.setLED(managerFrontPanelButton::status::FAILURE);


  //TODO: Add MQTT and stuff

}

/** Handles failures of output controllers */
void handleOutputFailure(outputController *controller){

  #ifdef DEBUG
    Serial.println("Output controller at 0x" + String(controller->address, HEX) + " is offline.");
  #endif

  //Disable the controller
  controller->enabled = false;

  //Set the LED to trouble
  frontPanelButton.setLED(managerFrontPanelButton::status::FAILURE);


  //TODO: Add MQTT and stuff

}

