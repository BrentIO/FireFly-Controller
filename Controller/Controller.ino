#define DEBUG 400

#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include "common/hardware.h"
#include "common/outputs.h"
#include "common/inputs.h"
#include "common/temperature.h"
#include "common/frontPanel.h"


managerOutputs outputs;
managerInputs inputs;
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
    outputs.setCallback_failure(&outputFailure);
    outputs.setCallback_publisher(&outputFailure);
    outputs.begin();
    inputs.begin();

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
  frontPanelButton.setStatus(managerFrontPanelButton::status::FAILURE);

};


void inputPublisher(){

  #ifdef DEBUG
    Serial.println("An input was made.");
  #endif

  //TODO: Add MQTT and stuff

}


/** Handles failures of input controllers */
void inputFailure(){

  #ifdef DEBUG
    Serial.println("An input controller is offline.");
  #endif

  //TODO: Add MQTT and stuff
  frontPanelButton.setStatus(managerFrontPanelButton::status::FAILURE);

}


/** Handles failures of output controllers */
void outputFailure(){

  #ifdef DEBUG
    Serial.println("An output controller is offline.");
  #endif

  //TODO: Add MQTT and stuff
  frontPanelButton.setStatus(managerFrontPanelButton::status::FAILURE);


}


void outputPublisher(){

  #ifdef DEBUG
    Serial.println("An output was changed.");
  #endif

  //TODO: Add MQTT and stuff

}