#define VERSION "2023.04.0002"

#define DEBUG 400

#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "0.north-america.pool.ntp.org"
#define NTP_SERVER_3 "0.europe.pool.ntp.org"
#define WIFI_TIMEOUT 10000 //Number of milliseconds before WiFi will time out


#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include "common/hardware.h"
#include "common/outputs.h"
#include "common/inputs.h"
#include "common/temperature.h"
#include "common/frontPanel.h"
#include "common/externalEEPROM.h"
#include "common/oled.h"
#include <WiFi.h>
#include "time.h"


managerOutputs outputs;
managerInputs inputs;
managerTemperatureSensors temperatureSensors;
managerFrontPanel frontPanel;
managerExternalEEPROM externalEEPROM;
managerOled oled;


struct tm bootTime;
time_t now;


void setup() {

    #ifdef DEBUG
      Serial.begin(115200);
    #endif

    Wire.begin();

    //Configure the peripherals
    oled.setCallback_failure(&oledFailure);
    oled.begin();

    frontPanel.setCallback_publisher(&frontPanelButtonPress);
    frontPanel.setCallback_state_closed_at_begin(&frontPanelButtonClosedAtBegin);
    frontPanel.begin();

    outputs.setCallback_failure(&outputFailure);
    outputs.setCallback_publisher(&outputFailure);
    outputs.begin();

    inputs.setCallback_failure(&inputFailure);
    inputs.setCallback_publisher(&inputPublisher);
    inputs.begin();

    temperatureSensors.setCallback_publisher(&temperaturePublisher);
    temperatureSensors.setCallback_failure(&temperatureFailure);
    temperatureSensors.begin();

    externalEEPROM.setCallback_failure(&eepromFailure);
    externalEEPROM.begin();
    

    #ifdef DEBUG
      Serial.println("Version: " + String(VERSION));
      Serial.println("Product ID: " + String(externalEEPROM.data.product_id));
      Serial.println("UUID: " + String(externalEEPROM.data.uuid));
      Serial.println("Key: " + String(externalEEPROM.data.key));
    #endif

    oled.setProductID(externalEEPROM.data.product_id);
    oled.setUUID(externalEEPROM.data.uuid);

    connectWiFi();

    setBootTime();

    //System has started, show normal state
    frontPanel.setStatus(managerFrontPanel::status::NORMAL);

    oled.showPage(managerOled::PAGE_EVENT_LOG);

    //oled.showError("Example error text");
  
}

void connectWiFi(){

  WiFi.begin(ssid, password);

  #ifdef DEBUG
    Serial.println("Connecting to WiFi");
  #endif

  const unsigned long time_now = millis();

  while(WiFi.status() != WL_CONNECTED){

    if((unsigned long)(millis() - time_now) >= WIFI_TIMEOUT){

      #ifdef DEBUG
        Serial.println("WiFi Timeout");
        oled.logEvent("WiFi Timeout",managerOled::LOG_LEVEL_INFO);
        break;

      #endif
    }

    #ifdef DEBUG
      Serial.print(".");
    #endif

    delay(100);
  }

  
    if(WiFi.status() == WL_CONNECTED){

      #ifdef DEBUG
        Serial.println("WiFi Connected");
      #endif

      oled.logEvent("WiFi Connected",managerOled::LOG_LEVEL_INFO);
    }
  

  oled.setWiFiInfo(&WiFi);

}


void setBootTime(){

    configTime(0,0, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    bootTime = timeinfo;

}



void loop() {
  inputs.loop();
  frontPanel.loop();
  temperatureSensors.loop();
  oled.loop();
}


void temperaturePublisher(String location, float value){

  #ifdef DEBUG
    Serial.println("New Temperature: " + String(value) + " at " + location);
  #endif

  oled.logEvent(("Temp: " + String(value) + char(0xF8) + "C").c_str(),managerOled::LOG_LEVEL_NOTIFICATION);

  //TODO: Add MQTT and stuff

};


/** Handles failures of temperature sensors */
void temperatureFailure(String location){

  #ifdef DEBUG
    Serial.println("Temperature sensor at " + location + " is offline");
  #endif

  //TODO: Add MQTT and stuff
  frontPanel.setStatus(managerFrontPanel::status::FAILURE);

};


void inputPublisher(){

  #ifdef DEBUG
    Serial.println("An input was made");
  #endif

  //TODO: Add MQTT and stuff

}


/** Handles failures of input controllers */
void inputFailure(){

  #ifdef DEBUG
    Serial.println("An input controller is offline");
  #endif

  //TODO: Add MQTT and stuff
  frontPanel.setStatus(managerFrontPanel::status::FAILURE);

}


/** Handles failures of output controllers */
void outputFailure(){

  #ifdef DEBUG
    Serial.println("An output controller is offline");
  #endif

  //TODO: Add MQTT and stuff
  frontPanel.setStatus(managerFrontPanel::status::FAILURE);


}


void outputPublisher(){

  #ifdef DEBUG
    Serial.println("An output was changed");
  #endif

  //TODO: Add MQTT and stuff

}


/** Handles failures of output controllers */
void eepromFailure(){

  #ifdef DEBUG
    Serial.println("EEPROM failure was called");
  #endif

  //TODO: Add MQTT and stuff
  frontPanel.setStatus(managerFrontPanel::status::FAILURE);

}


void frontPanelButtonPress(){

  #ifdef DEBUG
    Serial.println("Front Panel button was pressed");
  #endif

  oled.nextPage();

  //TODO: Add MQTT and stuff

}


void frontPanelButtonClosedAtBegin(){

  #ifdef DEBUG
    Serial.println("Front Panel button was closed on begin()");
  #endif

  //TODO: Add MQTT and stuff

}


/** Handles failures of the OLED display */
void oledFailure(managerOled::failureCode failureCode){

  switch(failureCode){
    case managerOled::failureCode::NOT_ON_BUS:
      #if DEBUG
        Serial.println("Error: OLED not found on bus");
      #endif
      break;

    case managerOled::failureCode::UNABLE_TO_START:
      #if DEBUG
        Serial.println("Error: Unable to start OLED");
      #endif
      break;

    default:
      #if DEBUG
        Serial.println("Error: Unknown OLED failure");
      #endif
  }

  //TODO: Add MQTT and stuff
  frontPanel.setStatus(managerFrontPanel::status::FAILURE);

};
