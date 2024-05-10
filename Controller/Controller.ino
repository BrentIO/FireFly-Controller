/*
* Controller.ino
*
* Create software-defined lighting.
*
* (C) 2019-2024, P5 Software, LLC
*
*/


#define VERSION "2024.01.0001"
#define APPLICATION_NAME "FireFly Controller"

#include "common/hardware.h"
#include "common/externalEEPROM.h"
#include "common/oled.h"
#include "common/frontPanel.h"
#include "common/inputs.h"
#include "common/temperature.h"
#include "common/outputs.h"
#include "common/eventLog.h"
#include "common/authorizationToken.h"
#include "common/otaConfig.h"
#include <ArduinoJson.h>
#include "AsyncJson.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <esp32FOTA.hpp>

unsigned long bootTime = 0; /* Approximate Epoch time the device booted */
AsyncWebServer httpServer(80);
managerExternalEEPROM externalEEPROM; /* External EEPROM instance */
managerOled oled; /* OLED instance */
managerFrontPanel frontPanel; /* Front panel instance */
managerInputs inputs; /* Inputs collection */
nsOutputs::managerOutputs outputs; /* Outputs collection */
managerTemperatureSensors temperatureSensors; /* Temperature sensors */
authorizationToken authToken;

#if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_W5500 || WIFI_MODEL == ENUM_WIFI_MODEL_ESP32
  WiFiUDP wifiNtpUdp;
  NTPClient timeClient(wifiNtpUdp); /* WiFi NTP client for handling time requests */
#endif

EventLog eventLog(&timeClient); /* Event Log instance */
uint64_t ntpSleepUntil = 0;

void updateNTPTime(bool force = false);

esp32FOTA otaFirmware(APPLICATION_NAME, VERSION, false); /* OTA firmware update class */
#define FIRMWARE_CHECK_SECONDS 86400 /* Number of seconds between OTA firmware checks */
uint64_t otaFirmware_lastCheckedTime = 0; /* The time (millis() or equivalent) when the firmware was last checked against the remote system */
bool otaFirmware_enabled = false; /* Determines if the OTA firmware automation should be run */
LinkedList<forcedOtaUpdateConfig> otaFirmware_pending;

fs::LittleFSFS wwwFS;
fs::LittleFSFS configFS;

#define CONFIGFS_PATH_CERTS "/certs/"


/**
 * One-time setup
*/
void setup() {
    bool wwwFS_isMounted = false;
    bool configFS_isMounted = false;

    eventLog.createEvent(F("Event log started"));
    
    Wire.begin();

    /* Start the auth token service */
    authToken.begin();


    //Configure the peripherals
    oled.setCallback_failure(&failureHandler_oled);
    oled.begin();
    oled.setEventLog(&eventLog);
    oled.setAuthorizationToken(&authToken);
    authToken.setCallback_visualTokenChanged(&eventHandler_visualAuthChanged);


    /* Set event log callbacks to the OLED */
    eventLog.setCallback_info(eventHandler_eventLogInfoEvent);
    eventLog.setCallback_notification(eventHandler_eventLogNotificationEvent);
    eventLog.setCallback_error(&eventHandler_eventLogErrorEvent);
    eventLog.setCallback_resolveError(&eventHandler_eventLogResolvedErrorEvent);


    /* Startup the front panel */
    frontPanel.setCallback_publisher(&eventHandler_frontPanelButtonPress);
    frontPanel.setCallback_state_closed_at_begin(&eventHandler_frontPanelButtonClosedAtBegin);
    frontPanel.begin();


    /* Determine hostname */
    #ifdef ESP32
      uint8_t baseMac[6];
      esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
      char hostname[18] = {0};
      sprintf(hostname, "FireFly-%02X%02X%02X", baseMac[3], baseMac[4], baseMac[5]);
    #endif


    /* Start networking */
    #if WIFI_MODEL == ENUM_WIFI_MODEL_ESP32

      WiFi.softAP(hostname);
      log_i("Started SoftAP %s", WiFi.softAPSSID());
    
      oled.setWiFiInfo(&WiFi);

    #endif


    #if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_W5500 && defined(ESP32)

      ESP32_W5500_onEvent();
      ESP32_W5500_setCallback_connected(&eventHandler_ethernetConnect);
      ESP32_W5500_setCallback_disconnected(&eventHandler_ethernetDisconnect);

      log_i("Setting up Ethernet on W5500");

      ETH.setHostname(hostname);
      esp_read_mac(baseMac, ESP_MAC_WIFI_STA);

      unsigned long ethernet_start_time = millis();
      ETH.begin(SPI_MISO_PIN, SPI_MOSI_PIN, SPI_SCK_PIN, ETHERNET_PIN, ETHERNET_PIN_INTERRUPT, SPI_CLOCK_MHZ, ETH_SPI_HOST, baseMac); //Use the base MAC, not the Ethernet MAC.  Library will automatically adjust it, else future calls to get MAC addresses are skewed

      while(!ESP32_W5500_isConnected()){

        delay(100);

        if(millis() > ethernet_start_time + ETHERNET_TIMEOUT){
            log_w("Ethernet connection timeout");
          break;
        }
      }

      if(ESP32_W5500_isConnected()){

        timeClient.begin();
        updateNTPTime(true);

        if(timeClient.isTimeSet()){
          bootTime = timeClient.getEpochTime();
        }
      }

      log_i("Ethernet IP: %s", ETH.localIP().toString().c_str());
      oled.setEthernetInfo(&ETH);

    #endif


    /* Start external EEPROM */
    externalEEPROM.setCallback_failure(&failureHandler_eeprom);
    externalEEPROM.begin();

    if(externalEEPROM.enabled == true){

      oled.setProductID(externalEEPROM.data.product_id);
      oled.setUUID(externalEEPROM.data.uuid);

      log_i("EEPROM UUID: %s", externalEEPROM.data.uuid);
      log_i("EEPROM Product ID: %s", externalEEPROM.data.product_id);
      log_i("EEPROM Key: %s", externalEEPROM.data.key);
    }


    /* Start inputs */
    inputs.setCallback_failure(&failureHandler_inputs);
    inputs.setCallback_publisher(&eventHandler_inputs);
    inputs.begin();


    /* Start outputs */
    outputs.setCallback_failure(&failureHandler_outputs);
    outputs.begin();


    /* Start temperature sensors */
    temperatureSensors.setCallback_publisher(&eventHandler_temperature);
    temperatureSensors.setCallback_failure(&failureHandler_temperatureSensors);
    temperatureSensors.begin();


    /* Start LittleFS for www */
    if (wwwFS.begin(false, "/wwwFS", (uint8_t)10U, "www"))
    {
      wwwFS_isMounted = true;
    }
    else{
      eventLog.createEvent(F("wwwFS mount fail"), EventLog::LOG_LEVEL_ERROR);
      log_e("An Error has occurred while mounting www");
    }


    /* Start LittleFS for config */
    if (configFS.begin(false, "/configFS", (uint8_t)10U, "config"))
    {
      configFS_isMounted = true;
    }
    else{
      eventLog.createEvent(F("configFS mount fail"), EventLog::LOG_LEVEL_ERROR);
      log_e("An Error has occurred while mounting configFS");
    }


    /**
     * OTHER HTTP SETUP GOES HERE
    */


    /**
      * CONFIG READING STUFF GOES HERE
    */


    /** 
     * I/O SETUP GOES HERE
    */
    

    oled.setPage(managerOled::PAGE_EVENT_LOG);

}


/**
 * Main loop
*/
void loop() {

  /** CHECK NTP HERE */

  /** CHECK FIRMWARE UPDATES HERE */

  oled.loop();
  authToken.loop();
  frontPanel.loop();
  inputs.loop();
  temperatureSensors.loop();

  outputs.configurePort(2, nsOutputs::outputPin::VARIABLE);                                                 //FOR DEBUG ONLY

}

/** Handles changes in observed temperatures 
 * @param location the location where the change was observed
 * @param value the new temperature in degrees celsius
*/
void eventHandler_temperature(char* location, float value){

  char *text = new char[OLED_CHARACTERS_PER_LINE+1];
  snprintf(text, OLED_CHARACTERS_PER_LINE+1, "Temp: %d%c C", value, char(0xF8));
  eventLog.createEvent(text);

  //TODO: Add MQTT and stuff

};


/** 
 * Callback function which handles failures of any temperature sensor 
*/
void failureHandler_temperatureSensors(uint8_t address, managerTemperatureSensors::failureReason failureReason){

  char *text = new char[OLED_CHARACTERS_PER_LINE+1];

  if(failureReason == managerTemperatureSensors::failureReason::ADDRESS_OFFLINE){
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "Temp sen 0x%02X offline", address);
  }else{
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "Temp sen 0x%02X fail %i", address, failureReason);
  }
  
  eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
  frontPanel.setStatus(managerFrontPanel::status::FAILURE);
}


/** Handles changes in observed inputs 
 * @param portChannel the port and channel where the change was observed
 * @param longChange when true, the change observed was a long in duration
*/
void eventHandler_inputs(managerInputs::portChannel portChannel, boolean longChange){

  log_d(" A %s input was made on port %i channel %i", longChange ? "long":"short", portChannel.port, portChannel.channel);

  #ifdef DEBUG
    if(longChange == false){
      Serial.println("[main] (eventHandler_inputs) A short input was made on port " + String(portChannel.port) + " channel " + String(portChannel.channel));

      uint8_t output_port = 2;
      uint8_t value = outputs.getPortValue(output_port);

      if(portChannel.port == 1){

        switch(portChannel.channel){

          case 2:
            Serial.println(outputs.setPortValue(output_port, (value + 10)));
            Serial.println("Increasing, New brightness: " + String(outputs.getPortValue(output_port)));

            break;


          case 6:
            Serial.println(outputs.setPortValue(output_port, (value - 10)));
            Serial.println("Decreasing, New brightness: " + String(outputs.getPortValue(output_port)));
            break;
        }


      }


      

      /*Serial.println("Current Brightness: " + String(value));

      

      if(value == 0){
        increasing = true;
        
        Serial.println("4 New brightness: " + String(outputs.getPortValue(output_port)));
        return;
      }

      if(value < 100 && increasing == true){
        outputs.setPortValue(output_port, (((value + 5) + 2) % 5) * 5);
        Serial.println("1 New brightness: " + String(outputs.getPortValue(output_port)));
        return;
      }

      if(value < 100 && increasing == false){
        outputs.setPortValue(output_port, (((value - 5) + 2) % 5) * 5);
        Serial.println("2 New brightness: " + String(outputs.getPortValue(output_port)));
        return;
      }

      if(value == 100){
        increasing = false;
        outputs.setPortValue(output_port, value - (((value - 5) + 2) % 5) * 5);
        Serial.println("3 New brightness: " + String(outputs.getPortValue(output_port)));
        return;
      }*/

      /*if(value == 0){
        Serial.println(outputs.setPortValue(output_port, 77));
        Serial.println("New brightness: " + String(outputs.getPortValue(output_port)));
      }

      if(value != 0){
        Serial.println(outputs.setPortValue(output_port, 0));
        Serial.println("New brightness: " + String(outputs.getPortValue(output_port)));
      }*/



    }
  #endif

  //TODO: Add MQTT and stuff

}



void eventHandler_frontPanelButtonPress(){

  log_v("Front Panel button was pressed");
  oled.nextPage();

}


void eventHandler_frontPanelButtonClosedAtBegin(){

  log_v("Front Panel button was closed on begin()");

  int i = 10;

  while(i>0){
    oled.setFactoryResetValue(i);
    oled.setPage(managerOled::PAGE_FACTORY_RESET);

    if(frontPanel.getButtonState() == managerFrontPanel::inputState::STATE_OPEN){

      log_i("Front Panel button was released before confirmation timeout");
      return;
    }

    i--;

    delay(1000);
  }

  log_i("Front Panel button was held to completion.  Config will be deleted.");

  //TODO: Add MQTT and stuff

}


/** Handles failures of the OLED display */
void failureHandler_oled(uint8_t address, managerOled::failureReason failureReason){

  char *text = new char[OLED_CHARACTERS_PER_LINE+1];

  if(failureReason == managerOled::failureReason::ADDRESS_OFFLINE){
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "OLED 0x%02X offline", address);
  }else{
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "OLED 0x%02X fail %i", address, failureReason);
  }
  
  eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
  frontPanel.setStatus(managerFrontPanel::status::FAILURE);

}


/**
 * Replace the auth token page with the event log so that the OLED will sleep when the visual token changes
*/
void eventHandler_visualAuthChanged(){
  if(oled.getPage() == managerOled::PAGE_AUTH_TOKEN){
    oled.setPage(managerOled::PAGE_EVENT_LOG);
  }
}

/** 
 * Handles events where the event log type was info
*/
void eventHandler_eventLogInfoEvent(){

  if(oled.isSleeping()){
    return;
  }

  if(oled.getPage() == managerOled::PAGE_EVENT_LOG){
    oled.setPage(managerOled::PAGE_EVENT_LOG);
  }
}


/** 
 * Handles events where the event log type was notification
*/
void eventHandler_eventLogNotificationEvent(){
  oled.setPage(managerOled::PAGE_EVENT_LOG);
}


/**
 * Handles events where the event log type was error
*/
void eventHandler_eventLogErrorEvent(){
  oled.setPage(managerOled::PAGE_ERROR);
  frontPanel.setStatus(managerFrontPanel::status::TROUBLE);
}


/**
 * Handles events where an error was resolved
*/
void eventHandler_eventLogResolvedErrorEvent(){

  if(eventLog.getErrors()->size() == 0){
    oled.setPage(managerOled::PAGE_EVENT_LOG);
    frontPanel.setStatus(managerFrontPanel::status::NORMAL);
  }else{
      oled.setPage(managerOled::PAGE_ERROR);
      frontPanel.setStatus(managerFrontPanel::status::TROUBLE);
  }
}


#if ETHERNET_MODEL != ENUM_ETHERNET_MODEL_NONE

  /** 
   * Handle Ethernet being connected
  */
  void eventHandler_ethernetConnect(){
    updateNTPTime(true);
    eventLog.createEvent(F("Ethernet connected"));
    eventLog.resolveError(F("Ethernet disconnected"));
  }


  /** 
   * Handle Ethernet being disconnected
  */
  void eventHandler_ethernetDisconnect(){
    eventLog.createEvent(F("Ethernet disconnected"), EventLog::LOG_LEVEL_ERROR);
  }

#endif


/** 
 * Callback function which handles failures of the external EERPOM 
*/
void failureHandler_eeprom(uint8_t address, managerExternalEEPROM::failureReason failureReason){

  char *text = new char[OLED_CHARACTERS_PER_LINE+1];

  if(failureReason == managerExternalEEPROM::failureReason::ADDRESS_OFFLINE){
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "ExEEPROM 0x%02X offline", address);
  }else{
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "ExEEPROM 0x%02X fail %i", address, failureReason);
  }
  
  eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
  frontPanel.setStatus(managerFrontPanel::status::FAILURE);

}


/** 
 * Callback function which handles failures of any input 
*/
void failureHandler_inputs(uint8_t address, managerInputs::failureReason failureReason){
	
	char *text = new char[OLED_CHARACTERS_PER_LINE+1];

  if(failureReason == managerInputs::failureReason::ADDRESS_OFFLINE){
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "Inpt ctl 0x%02X offline", address);
  }else{
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "Inpt ctl 0x%02X fail %i", address, failureReason);
  }
  
  eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
  frontPanel.setStatus(managerFrontPanel::status::FAILURE);
}


/** 
 * Callback function which handles failures of any output 
*/
void failureHandler_outputs(uint8_t address, nsOutputs::failureReason failureReason){

  char *text = new char[OLED_CHARACTERS_PER_LINE+1];

  if(failureReason == nsOutputs::failureReason::ADDRESS_OFFLINE){
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "Out ctl 0x%02X offline", address);
  }else{
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "Out ctl 0x%02X fail %i", address, failureReason);
  }
  
  eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
  frontPanel.setStatus(managerFrontPanel::status::FAILURE);
}


/**
 * Updates the NTP time from the server with special handling
*/
void updateNTPTime(bool force){

  if((esp_timer_get_time() > ntpSleepUntil) || force == true){
      /*
        Workaround until https://github.com/arduino-libraries/NTPClient/pull/163 is merged

        If time client update is unsuccessful, stop trying for 5 minutes
      */

      if(timeClient.update()){
        ntpSleepUntil = 0;
      }else{
        ntpSleepUntil = esp_timer_get_time() + 300000000;
      }
    }

}