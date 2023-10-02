#define VERSION "2023.09.0001"

#define DEBUG 501

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
unsigned long bootTime;


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
      Serial.println("[main] (setup) Version: " + String(VERSION));
      Serial.println("[main] (setup) Product ID: " + String(externalEEPROM.data.product_id));
      Serial.println("[main] (setup) UUID: " + String(externalEEPROM.data.uuid));
      Serial.println("[main] (setup) Key: " + String(externalEEPROM.data.key));
    #endif

    oled.setProductID(externalEEPROM.data.product_id);
    oled.setUUID(externalEEPROM.data.uuid);

    connectWiFi();

    configTime(0,0, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);

    setBootTime();    

    //System has started, show normal state
    frontPanel.setStatus(managerFrontPanel::status::NORMAL);

    oled.showPage(managerOled::PAGE_EVENT_LOG);
     
}


void connectWiFi(){

  WiFi.begin(ssid, password);

  #ifdef DEBUG
    Serial.println("[main] (connectWiFi) Connecting to WiFi");
  #endif

  const unsigned long time_now = millis();

  while(WiFi.status() != WL_CONNECTED){

    if((unsigned long)(millis() - time_now) >= WIFI_TIMEOUT){

      #ifdef DEBUG
        Serial.println("[main] (connectWiFi) WiFi Timeout");
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
        Serial.println("[main] (connectWiFi) WiFi Connected");
      #endif

      oled.logEvent("WiFi Connected",managerOled::LOG_LEVEL_INFO);
    }
  

  oled.setWiFiInfo(&WiFi);

}


unsigned long getTime() {
  /* Retrieves current GMT time. */

  time_t now;
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    
    #ifdef DEBUG
      Serial.println("[main] (getTime) unable to getLocalTime");
    #endif

    return(0);
  }

  return time(&now);

}


void setBootTime(){

  bootTime = getTime();

  if(bootTime == 0){
    oled.logEvent("Unknown boot time", managerOled::logLevel::LOG_LEVEL_INFO);

      #ifdef DEBUG
        Serial.println("[main] (setBootTime) Failed to set boot time, defaulting to 0.");
      #endif

  }else{
    oled.logEvent("Boot time set", managerOled::logLevel::LOG_LEVEL_INFO);

    #ifdef DEBUG > 1000
      Serial.println("[main] (setBootTime) Boot time set to " + String(bootTime));
    #endif
  }

}


void loop() {

  inputs.loop();
  frontPanel.loop();
  temperatureSensors.loop();
  oled.loop();

}

/** Handles changes in observed temperatures 
 * @param location the location where the change was observed
 * @param value the new temperature in degrees celsius
*/
void temperaturePublisher(char* location, float value){

  #ifdef DEBUG
    Serial.println("[main] (temperaturePublisher) New Temperature: " + String(value) + " at " + String(location));
  #endif

  oled.logEvent(("Temp: " + String(value) + char(0xF8) + "C").c_str(),managerOled::LOG_LEVEL_INFO);

  //TODO: Add MQTT and stuff

};


/** Handles failures of temperature sensors 
 * @param location the location of the sensor that has failed
 * @param failureReason the reason for the failure
*/
void temperatureFailure(char* location, managerTemperatureSensors::failureReason failureReason){

  #ifdef DEBUG
    Serial.print("[main] (temperatureFailure) ");
    Serial.print("Temperature sensor at ");
    Serial.print(location);
    Serial.println(" is being failed for reason " + String(failureReason));
  #endif

  oled.showError(("Temp sens at " + String(location) + " fail " + String(failureReason)).c_str());

  frontPanel.setStatus(managerFrontPanel::status::FAILURE);

  //TODO: Add MQTT and stuff

};


/** Handles changes in observed inputs 
 * @param portChannel the port and channel where the change was observed
 * @param longChange when true, the change observed was a long in duration
*/
void inputPublisher(managerInputs::portChannel portChannel, boolean longChange){

  #ifdef DEBUG
    if(longChange == false){
      Serial.println("[main] (inputPublisher) A short input was made on port " + String(portChannel.port) + " channel " + String(portChannel.channel));
    }else{
      Serial.println("[main] (inputPublisher) A long input was made on port " + String(portChannel.port) + " channel " + String(portChannel.channel));
    }
  #endif

  //TODO: Add MQTT and stuff

}


/** Handles failures of input controllers 
 * @param address the hexidecimal address of the input controller that has failed
 * @param failureReason the reason for the failure
*/
void inputFailure(uint8_t address, managerInputs::failureReason failureReason){

  #ifdef DEBUG
    Serial.print("[main] (inputFailure) ");
    Serial.print("Input controller at address 0x");
    Serial.print(address, HEX);
    Serial.println(" is being failed for reason " + String(failureReason));
  #endif

  oled.showError(("Inpt Ctl 0x" + String(address, HEX) + " fail " + String(failureReason)).c_str());

  frontPanel.setStatus(managerFrontPanel::status::FAILURE);

  //TODO: Add MQTT and stuff

}


/** Handles failures of output controllers 
 * @param address the hexidecimal address of the output controller that has failed
 * @param failureReason the reason for the failure
*/
void outputFailure(uint8_t address, managerOutputs::failureReason failureReason){

  #ifdef DEBUG
    Serial.print("[main] (outputFailure) ");
    Serial.print("Output controller at address 0x");
    Serial.print(address, HEX);
    Serial.println(" is being failed for reason " + String(failureReason));
  #endif

  oled.showError(("Out Ctl 0x" + String(address, HEX) + " fail " + String(failureReason)).c_str());

  frontPanel.setStatus(managerFrontPanel::status::FAILURE);
}


/** Handles failures of output controllers */
void eepromFailure(){

  #ifdef DEBUG
    Serial.println("[main] (eepromFailure) EEPROM failure was called");
  #endif

  //TODO: Add MQTT and stuff
  frontPanel.setStatus(managerFrontPanel::status::FAILURE);

}


void frontPanelButtonPress(){

  #ifdef DEBUG
    Serial.println("[main] (frontPanelButtonPress) Front Panel button was pressed");
  #endif

  oled.nextPage();

  //TODO: Add MQTT and stuff

}


void frontPanelButtonClosedAtBegin(){

  #ifdef DEBUG
    Serial.println("[main] (frontPanelButtonClosedAtBeginning) Front Panel button was closed on begin()");
  #endif

  int i = 10;

  while(i>0){
    oled.setFactoryResetValue(i);
    oled.showPage(managerOled::PAGE_FACTORY_RESET);

    if(frontPanel.getButtonState() == managerFrontPanel::inputState::STATE_OPEN){

      #ifdef DEBUG
        Serial.println("[main] (frontPanelButtonClosedAtBeginning) Front Panel button was released before confirmation timeout.");
      #endif

      return;
    }

    i--;

    delay(1000);
  }

  #ifdef DEBUG
    Serial.println("[main] (frontPanelButtonClosedAtBeginning) Front Panel button was held to completion; EEPROM will be deleted.");
  #endif



  //TODO: Add MQTT and stuff

}


/** Handles failures of the OLED display */
void oledFailure(managerOled::failureCode failureCode){

  switch(failureCode){
    case managerOled::failureCode::NOT_ON_BUS:
      #ifdef DEBUG
        Serial.println("[main] (oledFailure) Error: OLED not found on bus");
      #endif
      break;

    case managerOled::failureCode::UNABLE_TO_START:
      #ifdef DEBUG
        Serial.println("[main] (oledFailure) Error: Unable to start OLED");
      #endif
      break;

    default:
      #ifdef DEBUG
        Serial.println("[main] (oledFailure) Error: Unknown OLED failure");
      #endif
      break;
  }

  //TODO: Add MQTT and stuff
  frontPanel.setStatus(managerFrontPanel::status::FAILURE);

}