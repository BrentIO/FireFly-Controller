#define DEBUG 400

#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include "common/hardware.h"
#include "common/outputs.h"
#include "common/inputs.h"
#include "common/temperature.h"
#include "common/frontPanel.h"


managerInputs inputs;
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

    inputs.setCallback_failure(&handleInputFailure);
    inputs.begin();

    setupOutputs();
    temperatureSensors.setCallback_publisher(&temperaturePublisher);
    temperatureSensors.setCallback_failure(&temperatureFailure);
    temperatureSensors.begin();

    //System has started, show normal state
    frontPanelButton.setLED(managerFrontPanelButton::status::NORMAL);

}


void loop() {

  inputs.loop();
  temperatureSensors.loop();

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
void handleInputFailure(){

  #ifdef DEBUG
    Serial.println("An input controller is offline.");
  #endif

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