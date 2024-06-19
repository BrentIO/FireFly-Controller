/*
* Controller.ino
*
* Create software-defined lighting.
*
* (C) 2019-2024, P5 Software, LLC
*
*/


#define VERSION "2024.6.1"
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
#define CONFIGFS_PATH_DEVICES "/devices"
#define CONFIGFS_PATH_DEVICES_CONTROLLERS "/devices/controllers"


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
  eventLog.setCallback_info(&eventHandler_eventLogInfoEvent);
  eventLog.setCallback_notification(&eventHandler_eventLogNotificationEvent);
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

    /* Configure the web server.  
      IMPORTANT: *** Sequence below matters, they are sorted specific to generic *** 
    */

    httpServer.on("/api/version", http_handleVersion);
    httpServer.on("/api/events", http_handleEventLog);
    httpServer.on("/api/errors", http_handleErrorLog);
    httpServer.on("/auth", http_handleAuth);

    if(configFS_isMounted){
      httpServer.addHandler(new AsyncCallbackJsonWebHandler("^\/api/controllers\/([0-9a-f-]+)$", http_handleControllers_PUT));
      httpServer.on("^\/api/controllers$", ASYNC_HTTP_GET, http_handleListControllers);
      httpServer.on("^\/api/controllers\/([0-9a-f-]+)$", http_handleControllers);
      //httpServer.on("^\/certs\/([a-z0-9_.]+)$", http_handleCert);
      //httpServer.on("^/certs$", ASYNC_HTTP_ANY, http_handleCerts, http_handleCerts_Upload);
      //httpServer.addHandler(new AsyncCallbackJsonWebHandler("/api/ota/app", http_handleOTA_forced));
      //httpServer.addHandler(new AsyncCallbackJsonWebHandler("/api/ota/spiffs", http_handleOTA_forced));
      //httpServer.addHandler(new AsyncCallbackJsonWebHandler("/api/ota", http_handleOTA_POST));
      //httpServer.on("^\/api\/ota$", http_handleOTA);
      //setup_OtaFirmware();
    }else{
      log_e("configFS is not mounted");
      httpServer.on("^\/certs\/([a-z0-9_.]+)$", http_configFSNotMunted);
      httpServer.on("^/certs$", ASYNC_HTTP_ANY, http_configFSNotMunted);
      httpServer.on("/api/ota", http_configFSNotMunted);
    }

    if(wwwFS_isMounted){
      httpServer.serveStatic("/", wwwFS, "/");
      httpServer.rewrite("/ui/version", "/version.json");
      httpServer.rewrite("/", "/index.html");
    }

  httpServer.onNotFound(http_notFound);

  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Origin"), F("*")); //Ignore CORS

  #if WIFI_MODEL == ENUM_WIFI_MODEL_ESP32

    if(WiFi.getMode() == wifi_mode_t::WIFI_MODE_NULL){

      log_w("HTTP server will not be started because WiFi it has not been initialized (WIFI_MODE_NULL)");
    
    }else{
      httpServer.begin();
      eventLog.createEvent(F("Web server started"));

      log_i("HTTP server ready");
    }
  #endif

  #if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_W5500

    httpServer.begin();
    eventLog.createEvent(F("Web server started"));
    log_i("HTTP server ready");

  #endif


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

}

/** Handles changes in observed temperatures 
 * @param location the location where the change was observed
 * @param value the new temperature in degrees celsius
*/
void eventHandler_temperature(char* location, float value){

  char *text = new char[OLED_CHARACTERS_PER_LINE+1];
  snprintf(text, OLED_CHARACTERS_PER_LINE+1, "Temp: %.2f", value);
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


/** Handles the front panel button being pressed during startup */
void eventHandler_frontPanelButtonClosedAtBegin(){

  log_v("Front Panel button was closed on begin()");

  int i = 10;

  while(i>=0){
    oled.setFactoryResetValue(i);
    oled.setPage(managerOled::PAGE_FACTORY_RESET);

    if(frontPanel.getButtonState() == managerFrontPanel::inputState::STATE_OPEN){

      log_i("Front Panel button was released before confirmation timeout");
      return;
    }

    i--;

    delay(1000);
  }

  configFS.begin(false, "/configFS", (uint8_t)10U, "config");
  configFS.format();

  log_i("Factory reset done");
  eventLog.createEvent("Factory reset done");
  eventLog.createEvent("Release button now");
  oled.setPage(managerOled::PAGE_EVENT_LOG);

  while(frontPanel.getButtonState() != managerFrontPanel::inputState::STATE_OPEN){
    delay(100);
  }

  eventLog.createEvent("Rebooting...");
  delay(3000);

  #ifdef ESP32
    ESP.restart();
  #endif
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


/**
 * Sends a 404 response indicating the resource is not found
*/
void http_notFound(AsyncWebServerRequest *request) {
    request->send(404);
}


/**
 * Sends a 405 response indicating the method specified is not allowed
*/
void http_methodNotAllowed(AsyncWebServerRequest *request) {
    request->send(405);
}


/**
 * Sends a 401 response indicating the authentication is missing or invalid
*/
void http_unauthorized(AsyncWebServerRequest *request) {
    request->send(401);
}


/**
 * Sends a 500 response indicating an internal server or hardware failure
*/
void http_error(AsyncWebServerRequest *request, String message){

  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  StaticJsonDocument<256> doc;
  doc["message"] = message;

  serializeJson(doc, *response);
  response->setCode(500);
  request->send(response);
}


/**
 * Sends a 400 response indicating an invalid request from the caller
*/
void http_badRequest(AsyncWebServerRequest *request, String message){

  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  StaticJsonDocument<256> doc;
  doc["message"] = message;

  serializeJson(doc, *response);
  response->setCode(400);
  request->send(response);
}


/**
 * Sends a 403 response indicating an invalid request from the caller
*/
void http_forbiddenRequest(AsyncWebServerRequest *request, String message){

  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  StaticJsonDocument<256> doc;
  doc["message"] = message;

  serializeJson(doc, *response);
  response->setCode(403);
  request->send(response);
}


/**
 * Sends a 200 response for any OPTIONS requests
*/
void http_options(AsyncWebServerRequest *request) {
  request->send(200);
};


/**
 * Generic handler to return HTTP/500 responses when the configFS file system has not been mounted
*/
void http_configFSNotMunted(AsyncWebServerRequest *request){

  if(!request->hasHeader(F("visual-token"))){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
    http_unauthorized(request);
    return;
  }

  http_error(request, F("file system not mounted"));

}


/**
 * Handles the version endpoint requests
*/
void http_handleVersion(AsyncWebServerRequest *request){

  if(!request->hasHeader(F("visual-token"))){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
    http_unauthorized(request);
    return;
  }

  if(request->method() != ASYNC_HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  if(externalEEPROM.enabled == false){
    http_error(request, F("Cannot connect to external EEPROM"));
    return;
  }

  if(strcmp(externalEEPROM.data.uuid, "") == 0){
    http_error(request, F("Invalid EEPROM data"));
    return;
  }

  char product_hex[16] = {0};
  sprintf(product_hex, "0x%08X", PRODUCT_HEX);
 
  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  StaticJsonDocument<192> doc;
  doc["uuid"] = externalEEPROM.data.uuid;
  doc["product_id"] = externalEEPROM.data.product_id;
  doc["product_hex"] = product_hex;
  doc["application"] = VERSION;

  serializeJson(doc, *response);
  request->send(response);
}


/** 
 * Handle http requests for the event log
*/
void http_handleEventLog(AsyncWebServerRequest *request){

  if(!request->hasHeader(F("visual-token"))){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
    http_unauthorized(request);
    return;
  }

  if(request->method() != ASYNC_HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  if(eventLog.getEvents()->size() == 0){
    request->send(200, F("application/json"),"[]");
    return;
  }

  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  StaticJsonDocument<EVENT_LOG_MAXIMUM_ENTRIES * 100> doc;

  for(int i=0; i < eventLog.getEvents()->size(); i++){
    JsonObject entry = doc.createNestedObject();
    entry[F("time")] = eventLog.getEvents()->get(i).timestamp;

    switch(eventLog.getEvents()->get(i).level){
      case EventLog::LOG_LEVEL_ERROR:
        entry[F("level")] = F("error");
        break;

      case EventLog::LOG_LEVEL_NOTIFICATION:
        entry[F("level")] = F("notify");
        break;

      case EventLog::LOG_LEVEL_INFO:
        entry[F("level")] = F("info");
        break;

      default:
        entry[F("level")] = F("unknown");
        break;
    }

    entry[F("text")] = eventLog.getEvents()->get(i).text;
  }
 
  serializeJson(doc, *response);
  request->send(response);
}


/** 
 * Handle http requests for the error log
*/
void http_handleErrorLog(AsyncWebServerRequest *request){

  if(!request->hasHeader(F("visual-token"))){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
    http_unauthorized(request);
    return;
  }

  if(request->method() != ASYNC_HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  if(eventLog.getErrors()->size() == 0){
    request->send(200, F("application/json"),"[]");
    return;
  }

  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  StaticJsonDocument<EVENT_LOG_MAXIMUM_ENTRIES * 64> doc;

  for(int i=0; i < eventLog.getErrors()->size(); i++){
    JsonObject entry = doc.createNestedObject();

    entry[F("text")] = eventLog.getErrors()->get(i);
  }

  serializeJson(doc, *response);
  request->send(response);
}


/**
 * Creates a long-term authorization with the given visual token
*/
void http_handleAuth(AsyncWebServerRequest *request){
  
  if(request->method() != ASYNC_HTTP_POST){
    http_methodNotAllowed(request);
    return;
  }

  if(!request->hasHeader(F("visual-token"))){
    http_unauthorized(request);
    return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str(), true)){
    http_unauthorized(request);
    return;
  }

  request->send(204);
}


void http_handleControllers(AsyncWebServerRequest *request){
  switch(request->method()){

    case ASYNC_HTTP_OPTIONS:
      http_options(request);
      break;

    case ASYNC_HTTP_GET:

      http_handleControllers_GET(request);
      break;

    case ASYNC_HTTP_DELETE:

      http_handleControllers_DELETE(request);
      break;

    default:
      http_methodNotAllowed(request);
      break;
  }
}


/**
 * Handles Controller GETs
*/
void http_handleControllers_GET(AsyncWebServerRequest *request){

  /*if(!request->hasHeader(F("visual-token"))){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
    http_unauthorized(request);
    return;
  }*/                                                                                     //DEV ONLY
  
  String filename = CONFIGFS_PATH_DEVICES_CONTROLLERS + (String)"/" + request->pathArg(0);

  if(!configFS.exists(filename)){
    http_notFound(request);
    return;
  }

  File file = configFS.open(filename);
  request->send(file, request->pathArg(0), "application/json", false, NULL);
  file.close();
}


/**
 * Handles Controller DELETEs
*/
void http_handleControllers_DELETE(AsyncWebServerRequest *request){

  /*if(!request->hasHeader(F("visual-token"))){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
    http_unauthorized(request);
    return;
  }*/                                                                                     //DEV ONLY

  String filename = CONFIGFS_PATH_DEVICES_CONTROLLERS + (String)"/" + request->pathArg(0);

  if(!configFS.exists(filename)){
    http_notFound(request);
    return;
  }

  if(configFS.remove(filename)){
    request->send(204);
  }else{
    http_error(request, F("Failed when trying to delete file"));
  }
}


/**
 * Handles Controller PUTs
*/
void http_handleControllers_PUT(AsyncWebServerRequest *request, JsonVariant doc){

  /*if(!request->hasHeader(F("visual-token"))){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
    http_unauthorized(request);
    return;
  }*/                                                                                     //DEV ONLY

  if(request->method() != ASYNC_HTTP_PUT){
    http_methodNotAllowed(request);
    return;
  }

  if(request->pathArg(0).length() != 36){
    http_badRequest(request, F("UUID must be exactly 36 characters"));
    return;
  }

  if(!configFS.exists(CONFIGFS_PATH_DEVICES)){
    if(!configFS.mkdir(CONFIGFS_PATH_DEVICES)){
      http_error(request, F("Unable to create CONFIGFS_PATH_DEVICES directory"));
      return;
    };
  }

  if(!configFS.exists(CONFIGFS_PATH_DEVICES_CONTROLLERS)){
    if(!configFS.mkdir(CONFIGFS_PATH_DEVICES_CONTROLLERS)){
      http_error(request, F("Unable to create CONFIGFS_PATH_DEVICES_CONTROLLERS directory"));
      return;
    };
  }

  File file = configFS.open(CONFIGFS_PATH_DEVICES_CONTROLLERS + (String)"/" + request->pathArg(0) , "w");

  if(!file){
    file.close();
    http_error(request, F("Unable to open the file for writing"));
    return;
  }
  
  serializeJson(doc, file);
  file.close();  

  request->send(204);
}


/**
 * Handles List Controllers
*/
void http_handleListControllers(AsyncWebServerRequest *request){

  /*if(!request->hasHeader(F("visual-token"))){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
    http_unauthorized(request);
    return;
  }*/                                                                                     //DEV ONLY

  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  DynamicJsonDocument doc(1536);
  JsonArray array = doc.to<JsonArray>();

  File root = configFS.open(CONFIGFS_PATH_DEVICES_CONTROLLERS);

  File file = root.openNextFile();

  while(file){
      if(!file.isDirectory()){
        array.add((String)file.name());
      }
      file = root.openNextFile();
    }

  serializeJson(doc, *response);
  request->send(response);

}