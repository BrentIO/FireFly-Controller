/*
* Controller.ino
*
* Create software-defined lighting.
*
* (C) 2019-2025, P5 Software, LLC
*
*/

#define VERSION "2025.4.5"
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
#include <StreamUtils.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "common/extendedPubSubClient.h"
#include "common/provisioningMode.h"

unsigned long bootTime = 0; /* Approximate Epoch time the device booted */
AsyncWebServer httpServer(80);
managerExternalEEPROM externalEEPROM; /* External EEPROM instance */
managerOled oled; /* OLED instance */
managerFrontPanel frontPanel; /* Front panel instance */
managerInputs inputs; /* Inputs collection */
nsOutputs::managerOutputs outputs; /* Outputs collection */
managerTemperatureSensors temperatureSensors; /* Temperature sensors */
authorizationToken authToken;
managerProvisioningMode provisioningMode;

#if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_W5500 || WIFI_MODEL == ENUM_WIFI_MODEL_ESP32
  WiFiClient ethClient;
#endif

exPubSubClient mqttClient(ethClient);

#if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_W5500 || WIFI_MODEL == ENUM_WIFI_MODEL_ESP32
  WiFiUDP wifiNtpUdp;
  NTPClient timeClient(wifiNtpUdp); /* WiFi NTP client for handling time requests */
#endif

EventLog eventLog(&timeClient); /* Event Log instance */
uint64_t ntpSleepUntil = 0;

void updateNTPTime(bool force = false);

exEsp32FOTA otaFirmware(APPLICATION_NAME, VERSION, false); /* OTA firmware update class */

fs::LittleFSFS wwwFS;
fs::LittleFSFS configFS;
bool wwwFS_isMounted = false;
bool configFS_isMounted = false;

#define CONFIGFS_PATH_CERTS "/certs"
#define CONFIGFS_PATH_CONTROLLERS "/controllers"
#define CONFIGFS_PATH_CLIENTS "/clients"


enum outputAction{
  /// @brief The output should toggle the opposite of its current state
  TOGGLE = 0,

  /// @brief The output should increase its state
  INCREASE = 1,

  /// @brief The output should decrease its state
  DECREASE = 2,

  /// @brief Set the output to the maximum potential state (maximum brightness)
  INCREASE_MAXIMUM = 3,

  /// @brief Set the output the the minimum potential state (turn off)
  DECREASE_MAXIMUM = 4

};

struct inputAction{
  /// @brief The output port number to take the action against
  uint8_t output;

  /// @brief The action to take against the output port
  outputAction action;

  /// @brief The change state that is required for this action to be taken
  managerInputs::changeState changeState;

};

struct inputChannel{
  /// @brief The physical channel number, typically 1, 2, 3, or 6
  uint8_t channel;

  /// @brief List of actions to take when the channel state changes from its normal position
  LinkedList<inputAction> actions;
};

/***
 * Reference to an input port, used 
 */
struct inputPort{
  /// @brief The human-readable ID of the input port
  char id[PORT_ID_MAX_LENGTH + 1];

  /// @brief Reference to the channels on this input port
  inputChannel channels[IO_EXTENDER_COUNT_CHANNELS_PER_PORT];
};

inputPort inputPorts[(IO_EXTENDER_COUNT_PINS / IO_EXTENDER_COUNT_CHANNELS_PER_PORT) * IO_EXTENDER_COUNT];


/**
 * One-time setup
*/
void setup() {

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


  /* Set callbacks for provisioning mode */
  provisioningMode.setCallback_active(&eventHandler_provisioningModeActive);
  provisioningMode.setCallback_inactive(&eventHandler_provisioningModeInactive);
  provisioningMode.setCallback_rogueClient(&eventHandler_rogueClient);


  /* Determine hostname */
  #ifdef ESP32
    uint8_t baseMac[6];
    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
    char hostname[18] = {0};
    sprintf(hostname, "FireFly-%02X%02X%02X", baseMac[3], baseMac[4], baseMac[5]);
  #endif


  #if WIFI_MODEL == ENUM_WIFI_MODEL_ESP32
    oled.setWiFiInfo(&WiFi);
    provisioningMode.setWiFI(&WiFi);
  #endif


  #if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_W5500 && defined(ESP32)

    ESP32_W5500_onEvent();
    ESP32_W5500_setCallback_connected(&eventHandler_ethernetConnect);
    ESP32_W5500_setCallback_disconnected(&eventHandler_ethernetDisconnect);

    log_d("Setting up Ethernet on W5500");

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
    }

    log_d("Ethernet IP: %s", ETH.localIP().toString().c_str());
    oled.setEthernetInfo(&ETH);

  #endif


  /* Start external EEPROM */
  externalEEPROM.setCallback_failure(&failureHandler_eeprom);
  externalEEPROM.begin();

  if(externalEEPROM.enabled == true){

    oled.setProductID(externalEEPROM.data.product_id);
    oled.setUUID(externalEEPROM.data.uuid);

    log_d("EEPROM UUID: %s", externalEEPROM.data.uuid);
    log_d("EEPROM Product ID: %s", externalEEPROM.data.product_id);
    log_d("EEPROM Key: %s", externalEEPROM.data.key);
  }


  /* Start inputs */
  inputs.setCallback_failure(&failureHandler_inputs);
  inputs.setCallback_publisher(&eventHandler_inputs);
  inputs.begin();


  /* Start outputs */
  outputs.setCallback_failure(&failureHandler_outputs);
  outputs.setCallback_outputValueChanged(&mqtt_publishOutputValueChanged);
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

    if(!configFS.exists(CONFIGFS_PATH_CERTS + (String)"/")){
      if(!configFS.mkdir(CONFIGFS_PATH_CERTS)){
        eventLog.createEvent(F("Err mkdir certs"), EventLog::LOG_LEVEL_ERROR);
        configFS_isMounted = false;
      };
    }

    if(!configFS.exists(CONFIGFS_PATH_CONTROLLERS + (String)"/")){
      if(!configFS.mkdir(CONFIGFS_PATH_CONTROLLERS)){
        eventLog.createEvent(F("Err mkdir ctlrs"), EventLog::LOG_LEVEL_ERROR);
        configFS_isMounted = false;
      };
    }

    if(!configFS.exists(CONFIGFS_PATH_CLIENTS + (String)"/")){
      if(!configFS.mkdir(CONFIGFS_PATH_CLIENTS)){
        eventLog.createEvent(F("Err mkdir clients"), EventLog::LOG_LEVEL_ERROR);
        configFS_isMounted = false;
      };
    }

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
  httpServer.on("/files", http_handleFileList_GET);

  if(configFS_isMounted){
    httpServer.addHandler(new AsyncCallbackJsonWebHandler("^\/api/controllers\/([0-9a-f-]+)$", http_handleControllers_PUT, 65536,65535));
    httpServer.on("^\/api/controllers$", ASYNC_HTTP_ANY, http_handleListControllers);
    httpServer.on("^\/api/controllers\/([0-9a-f-]+)$", http_handleControllers);
    httpServer.addHandler(new AsyncCallbackJsonWebHandler("^\/api/clients\/([0-9a-f-]+)$", http_handleClients_PUT, 65536,65535));
    httpServer.on("^\/api/clients$", ASYNC_HTTP_ANY, http_handleListClients);
    httpServer.on("^\/api/clients\/([0-9a-f-]+)$", http_handleClients);
    httpServer.addHandler(new AsyncCallbackJsonWebHandler("/backup", http_handleBackup_PUT, 65536,65535));
    httpServer.on("/backup", http_handleBackup);
    httpServer.on("/api/provisioning", http_handleProvisioning);
    httpServer.on("^\/certs\/([a-z0-9_.]+)$", http_handleCert);
    httpServer.on("^/certs$", ASYNC_HTTP_ANY, http_handleCerts, http_handleCerts_Upload);
    httpServer.on("/api/ota/app", ASYNC_HTTP_OPTIONS, http_options);
    httpServer.addHandler(new AsyncCallbackJsonWebHandler("/api/ota/app", http_handleOTA_forced_POST));
    httpServer.on("/api/ota/spiffs", ASYNC_HTTP_OPTIONS, http_options);
    httpServer.addHandler(new AsyncCallbackJsonWebHandler("/api/ota/spiffs", http_handleOTA_forced_POST));
    httpServer.on("/ui/version", http_handleUIVersion);
    setup_OtaFirmware();
  }else{
    log_e("configFS is not mounted");
    httpServer.on("^\/certs\/([a-z0-9_.]+)$", http_configFSNotMunted);
    httpServer.on("^/certs$", ASYNC_HTTP_ANY, http_configFSNotMunted);
    httpServer.on("/api/controllers", http_configFSNotMunted);
    httpServer.on("/api/clients", http_configFSNotMunted);
    httpServer.on("/backup", http_configFSNotMunted);
    httpServer.on("/ui/version", http_configFSNotMunted);
  }

  if(wwwFS_isMounted){
    httpServer.serveStatic("/", wwwFS, "/");
    httpServer.rewrite("/", "/index.html");
  }

  httpServer.onNotFound(http_notFound);

  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Origin"), F("*")); //Ignore CORS
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Headers"), F("visual-token, Content-Type")); //Ignore CORS
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Methods"), F("GET, POST, OPTIONS, PUT, DELETE")); //Ignore CORS


  #if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_W5500 || WIFI_MODEL == ENUM_WIFI_MODEL_ESP32

    httpServer.begin();
    eventLog.createEvent(F("Web server started"));
    log_d("HTTP server ready");

  #endif

  setupMQTT();

  setupIO();

  setControllerNameOnOLED();

  oled.setPage(managerOled::PAGE_EVENT_LOG);

}


/**
 * Main loop
*/
void loop() {

  updateNTPTime();

  if(otaFirmware.enabled && esp_timer_get_time() > 30000000){ //Wait 30 seconds after booting before checking the firmware

    if((esp_timer_get_time() - otaFirmware.lastCheckedTime) / 1000000 > FIRMWARE_CHECK_SECONDS || otaFirmware.lastCheckedTime == 0){
      if(otaFirmware.updateInProcess == false){
        otaFirmware.execHTTPcheck();
      }
      otaFirmware.lastCheckedTime = esp_timer_get_time();
    }
  }

  otaFirmware_checkPending();

  oled.loop();
  authToken.loop();
  frontPanel.loop();
  inputs.loop();
  temperatureSensors.loop();
  provisioningMode.loop();

  #if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_W5500
    if(ESP32_W5500_isConnected()){
      if(!mqttClient.loop() && mqttClient.enabled == true){
        mqtt_reconnect();
      };
    }
  #endif

  #if ETHERNET_MODEL != ENUM_ETHERNET_MODEL_W5500
    #error MQTT will not automatically reconnect with this ethernet model
  #endif

}

/** Handles changes in observed temperatures 
 * @param location the location where the change was observed
 * @param value the new temperature in degrees celsius
*/
void eventHandler_temperature(const char* location, float value){

  char* temperature = new char[6];
  snprintf(temperature, 6, "%.2f", value);

  char *oledText = new char[OLED_CHARACTERS_PER_LINE+1];
  snprintf(oledText, OLED_CHARACTERS_PER_LINE+1, "Temp: %s", temperature);
  eventLog.createEvent(oledText);

  char* topic = new char[MQTT_TOPIC_TEMPERATURE_STATE_PATTERN_LENGTH+1];
  snprintf(topic, MQTT_TOPIC_TEMPERATURE_STATE_PATTERN_LENGTH+1, MQTT_TOPIC_TEMPERATURE_STATE_PATTERN, externalEEPROM.data.uuid, location);

  mqttClient.publish(topic, temperature, true);
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
 * @param changeState the state of the change observed
*/
void eventHandler_inputs(managerInputs::portChannel portChannel, managerInputs::changeState changeState){

  char* state_topic = new char[MQTT_TOPIC_INPUT_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, MQTT_TOPIC_INPUT_STATE_PATTERN_LENGTH+1, MQTT_TOPIC_INPUT_STATE_PATTERN, inputPorts[portChannel.port-1].id, (portChannel.channel+portChannel.offset));

  switch(changeState){
    case managerInputs::changeState::CHANGE_STATE_NORMAL:
      mqttClient.publish(state_topic, "NORMAL");
      break;
    case managerInputs::changeState::CHANGE_STATE_LONG_DURATION:
      mqttClient.publish(state_topic, "LONG");
      break;
    case managerInputs::changeState::CHANGE_STATE_SHORT_DURATION:
      mqttClient.publish(state_topic, "SHORT");
    break;
  }

  for(int i=0; i < IO_EXTENDER_COUNT_CHANNELS_PER_PORT; i++){

    if(inputPorts[portChannel.port-1].channels[i].channel == portChannel.channel){

      switch(changeState){

        case managerInputs::changeState::CHANGE_STATE_NORMAL:
          break;

        case managerInputs::changeState::CHANGE_STATE_SHORT_DURATION:
        case managerInputs::changeState::CHANGE_STATE_LONG_DURATION:
       
          for(int j=0; j < inputPorts[portChannel.port-1].channels[i].actions.size(); j++){
            if(inputPorts[portChannel.port-1].channels[i].actions.get(j).changeState == changeState){
              nsOutputs::set_result result = actionOutputPort(inputPorts[portChannel.port-1].channels[i].actions.get(j).output, inputPorts[portChannel.port-1].channels[i].actions.get(j).action);
              
              if(result == nsOutputs::set_result::EXCESSIVE){
                mqttClient.publish(state_topic, "EXCESSIVE");
              }
            }
          }
          break;
      }
      break;
    }
  }
}


/**
 * Handles input actions against output ports
 * @param port as the physical port number of the output
 * @param action as the action to take on the port
 */
nsOutputs::set_result actionOutputPort(uint8_t port, outputAction action){
  uint8_t currentPortValue = outputs.getPortValue(port);

  nsOutputs::set_result returnValue;

  switch(action){

    case outputAction::INCREASE:
      returnValue = outputs.setPortValue(port, (currentPortValue + 10));
      break;

    case outputAction::INCREASE_MAXIMUM:
      returnValue = outputs.setPortValue(port, 100);
      break;

    case outputAction::DECREASE:
      returnValue = outputs.setPortValue(port, (currentPortValue - 10));
      break;

    case outputAction::DECREASE_MAXIMUM:
      returnValue = outputs.setPortValue(port, 0);
      break;

    case outputAction::TOGGLE:

      if(currentPortValue > 0){
        returnValue = outputs.setPortValue(port, 0);
      }else{
        returnValue = outputs.setPortValue(port, 100);
      }
      break;
  }

  return returnValue;
}


/** 
 * Handles front panel button press events
 */
void eventHandler_frontPanelButtonPress(){

  log_v("Front Panel button was pressed");
  oled.nextPage();

}


/** 
 * Handles the front panel button being held during startup
 */
void eventHandler_frontPanelButtonClosedAtBegin(){

  log_v("Front Panel button was closed on begin()");

  int i = 10;

  while(i>=0){
    oled.setFactoryResetValue(i);
    oled.setPage(managerOled::PAGE_FACTORY_RESET);

    if(frontPanel.getButtonState() == managerFrontPanel::inputState::STATE_OPEN){

      log_w("Front Panel button was released before confirmation timeout");
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

  mqtt_publishCountErrors();
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

  mqtt_publishCountErrors();
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

    if(timeClient.isTimeSet() && bootTime == 0){
      bootTime = timeClient.getEpochTime() - (esp_timer_get_time()/1000000);
      mqtt_publishStartTime();
    }
}


/** 
 * Sets the controller name on the OLED display
 */
void setControllerNameOnOLED(){

  String filename = CONFIGFS_PATH_CONTROLLERS + (String)"/" + externalEEPROM.data.uuid;

  if(configFS.exists(filename)){
    StaticJsonDocument<16> filter;
    filter["name"] = true;

    StaticJsonDocument<64> doc;

    File file = configFS.open(filename, "r");

    deserializeJson(doc, file, DeserializationOption::Filter(filter));

    file.close();

    oled.setName(doc["name"]);

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

  if(request->method() == ASYNC_HTTP_OPTIONS){
    http_options(request);
    return;
  }

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

  if(request->method() == ASYNC_HTTP_OPTIONS){
    http_options(request);
    return;
  }

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

  if(request->method() == ASYNC_HTTP_OPTIONS){
    http_options(request);
    return;
  }

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

  switch(request->method()){

    case ASYNC_HTTP_OPTIONS:
      http_options(request);
      break;

    case ASYNC_HTTP_POST:
      if(!request->hasHeader(F("visual-token"))){
        http_unauthorized(request);
        return;
      }
    
      if(!authToken.authenticate(request->header(F("visual-token")).c_str(), true)){
        http_unauthorized(request);
        return;
      }
    
      request->send(204);
      break;

    default:
      http_methodNotAllowed(request);
      break;
  } 
}


/**
 * Generic handler for /api/controllers
 */
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

  if(!request->hasHeader(F("visual-token"))){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
    http_unauthorized(request);
    return;
  }
  
  String filename = CONFIGFS_PATH_CONTROLLERS + (String)"/" + request->pathArg(0);

  if(!configFS.exists(filename)){
    http_notFound(request);
    return;
  }

  AsyncWebServerResponse *response = request->beginResponse(configFS, filename, F("application/json"));
  request->send(response);

}


/**
 * Handles Controller DELETEs
*/
void http_handleControllers_DELETE(AsyncWebServerRequest *request){

  if(!request->hasHeader(F("visual-token"))){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
    http_unauthorized(request);
    return;
  }

  String filename = CONFIGFS_PATH_CONTROLLERS + (String)"/" + request->pathArg(0);

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

  if(request->method() == ASYNC_HTTP_OPTIONS){
    http_options(request);
    return;
  }

  if(!request->hasHeader(F("visual-token"))){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
    http_unauthorized(request);
    return;
  }

  if(request->method() != ASYNC_HTTP_PUT){
    http_methodNotAllowed(request);
    return;
  }

  if(request->pathArg(0).length() != 36){
    http_badRequest(request, F("UUID must be exactly 36 characters"));
    return;
  }

  File file = configFS.open(CONFIGFS_PATH_CONTROLLERS + (String)"/" + request->pathArg(0) , "w");

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

  if(request->method() == ASYNC_HTTP_OPTIONS){
    http_options(request);
    return;
  }

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

  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  DynamicJsonDocument doc(8192);  //Supports 128 UUID's
  JsonArray array = doc.to<JsonArray>();

  File root = configFS.open(CONFIGFS_PATH_CONTROLLERS + (String)"/");

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


/**
 * Generic handler for /api/clients
 */
void http_handleClients(AsyncWebServerRequest *request){
  switch(request->method()){

    case ASYNC_HTTP_OPTIONS:
      http_options(request);
      break;

    case ASYNC_HTTP_GET:

      http_handleClients_GET(request);
      break;

    case ASYNC_HTTP_DELETE:

      http_handleClients_DELETE(request);
      break;

    default:
      http_methodNotAllowed(request);
      break;
  }
}


/**
 * Handles Client GETs
*/
void http_handleClients_GET(AsyncWebServerRequest *request){

  if(!request->hasHeader(F("visual-token")) && !request->hasHeader(F("mac-address"))){
      http_unauthorized(request);
      return;
  }

  if(request->hasHeader(F("visual-token"))){
    if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
      http_unauthorized(request);
      return;
    }
  }

  if(request->hasHeader(F("mac-address"))){

    if(provisioningMode.getStatus() != true){
      http_badRequest(request, F("Provisioning mode inactive"));
      return;
    }

    if(!authClientWithMacAddress(request->pathArg(0).c_str(), request->header(F("mac-address")).c_str())){
      http_forbiddenRequest(request, F("Request will not be fulfilled"));
      return;
    }
  }
  
  String filename = CONFIGFS_PATH_CLIENTS + (String)"/" + request->pathArg(0);

  if(!configFS.exists(filename)){
    http_notFound(request);
    return;
  }

  AsyncWebServerResponse *response = request->beginResponse(configFS, filename, F("application/json"));
  request->send(response);

}


/// @brief Authorizes a call using just a MAC address
/// @param macAddress 
/// @return true if authorized, false if not authorized
boolean authClientWithMacAddress(const char* uuid, const char* macAddress){

    String filename = CONFIGFS_PATH_CLIENTS + (String)"/" + uuid;

    if(!configFS.exists(filename)){
      return false;
    }

    StaticJsonDocument<256> doc;
    StaticJsonDocument<16> filter;
    filter["mac"] = true;

    File file = configFS.open(filename);

    DeserializationError error = deserializeJson(doc, file, DeserializationOption::Filter(filter));

    if (error) {
      return false;
    }

    file.close();

    if(strcmp(doc["mac"].as<std::string>().c_str(), macAddress) != 0){
      log_i("Rogue client detected Header: %s != document: %s", macAddress, doc["mac"].as<std::string>().c_str());
      eventHandler_rogueClient(macAddress);
      provisioningMode.setInactive();
      return false;
    }

    return true;
}


/**
 * Handles Client DELETEs
*/
void http_handleClients_DELETE(AsyncWebServerRequest *request){

  if(!request->hasHeader(F("visual-token"))){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
    http_unauthorized(request);
    return;
  }

  String filename = CONFIGFS_PATH_CLIENTS + (String)"/" + request->pathArg(0);

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
 * Handles Clients PUTs
*/
void http_handleClients_PUT(AsyncWebServerRequest *request, JsonVariant doc){

  if(request->method() == ASYNC_HTTP_OPTIONS){
    http_options(request);
    return;
  }

  if(!request->hasHeader(F("visual-token"))){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
    http_unauthorized(request);
    return;
  }

  if(request->method() != ASYNC_HTTP_PUT){
    http_methodNotAllowed(request);
    return;
  }

  if(request->pathArg(0).length() != 36){
    http_badRequest(request, F("UUID must be exactly 36 characters"));
    return;
  }

  File file = configFS.open(CONFIGFS_PATH_CLIENTS + (String)"/" + request->pathArg(0) , "w");

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
 * Handles List Clients
*/
void http_handleListClients(AsyncWebServerRequest *request){

  if(request->method() == ASYNC_HTTP_OPTIONS){
    http_options(request);
    return;
  }

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

  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  DynamicJsonDocument doc(8192);  //Supports 128 UUID's
  JsonArray array = doc.to<JsonArray>();

  File root = configFS.open(CONFIGFS_PATH_CLIENTS + (String)"/");

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


/**
 * Generic handler for /provisioning
 */
void http_handleProvisioning(AsyncWebServerRequest *request){

  if(request->method() == ASYNC_HTTP_OPTIONS){
    http_options(request);
    return;
  }

  if(!request->hasHeader(F("visual-token"))){
    http_unauthorized(request);
    return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
    http_unauthorized(request);
    return;
  }
  
  switch(request->method()){

    case ASYNC_HTTP_PUT:
      http_handleProvisioning_PUT(request);
      break;

    case ASYNC_HTTP_GET:
      http_handleProvisioning_GET(request);
      break;

    case ASYNC_HTTP_DELETE:
      http_handleProvisioning_DELETE(request);
      break;

    default:
      http_methodNotAllowed(request);
      break;
  }
}


void http_handleProvisioning_GET(AsyncWebServerRequest *request){
  StaticJsonDocument<16> doc;
  doc["enabled"] = provisioningMode.getStatus();
  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  serializeJson(doc, *response);

  request->send(response);
}


void http_handleProvisioning_PUT(AsyncWebServerRequest *request){

  if(!configFS_isMounted){
    http_error(request, F("File system not mounted"));
    return;
  }

  request->send(202);

  StaticJsonDocument<256> doc;
  StaticJsonDocument<16> filter;
  filter["mac"] = true;

  File root = configFS.open(CONFIGFS_PATH_CLIENTS + (String)"/");
  File file = root.openNextFile();

  while(file){

      if(!file.isDirectory()){
        DeserializationError error = deserializeJson(doc, file, DeserializationOption::Filter(filter));

        if (error) {
          log_e("deserializeJson() failed: %s", error.c_str());
          continue;
        }
        provisioningMode.addAllowedMac(doc["mac"].as<std::string>());
      }

      file = root.openNextFile();
  }

  provisioningMode.setActive();
}

void http_handleProvisioning_DELETE(AsyncWebServerRequest *request){
  request->send(202);
  provisioningMode.setInactive();
}


void eventHandler_provisioningModeActive(){
  eventLog.createEvent(F("Provisioning active"), EventLog::LOG_LEVEL_NOTIFICATION);
}


void eventHandler_provisioningModeInactive(){
  eventLog.createEvent(F("Provisioning inactive"));
}


void eventHandler_rogueClient(const char* macAddress){
  char *oledText = new char[OLED_CHARACTERS_PER_LINE+1];
  snprintf(oledText, OLED_CHARACTERS_PER_LINE+1, "!RC %s", macAddress);
  eventLog.createEvent(oledText, EventLog::LOG_LEVEL_NOTIFICATION);
};


/**
 * Generic handler for /backup
 */
void http_handleBackup(AsyncWebServerRequest *request){
  switch(request->method()){

    case ASYNC_HTTP_OPTIONS:
      http_options(request);
      break;

    case ASYNC_HTTP_GET:
      http_handleBackup_GET(request);
      break;

    case ASYNC_HTTP_DELETE:
      http_handleBackup_DELETE(request);
      break;

    default:
      http_methodNotAllowed(request);
      break;
  }
}


/**
 * Handles Backup GETs
*/
void http_handleBackup_GET(AsyncWebServerRequest *request){

  if(!request->hasHeader(F("visual-token"))){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
    http_unauthorized(request);
    return;
  }

  if(!configFS.exists(F("/backup"))){
    http_notFound(request);
    return;
  }

  AsyncWebServerResponse *response = request->beginResponse(configFS, F("/backup"), F("application/json"));
  request->send(response);
}


/**
 * Handles Backup PUTs
*/
void http_handleBackup_PUT(AsyncWebServerRequest *request, JsonVariant doc){

  if(!request->hasHeader(F("visual-token"))){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
    http_unauthorized(request);
    return;
  }

  if(request->method() != ASYNC_HTTP_PUT){
    http_methodNotAllowed(request);
    return;
  }

  File file = configFS.open(F("/backup"), "w");

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
 * Handles Backup DELETEs
*/
void http_handleBackup_DELETE(AsyncWebServerRequest *request){

  if(!request->hasHeader(F("visual-token"))){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
    http_unauthorized(request);
    return;
  }

  if(!configFS.exists(F("/backup"))){
    http_notFound(request);
    return;
  }

  if(configFS.remove(F("/backup"))){
    request->send(204);
  }else{
    http_error(request, F("Failed when trying to delete file"));
  }
}


/**
 * Generic handler for /ui/version
 */
void http_handleUIVersion(AsyncWebServerRequest *request){
  switch(request->method()){

    case ASYNC_HTTP_OPTIONS:
      http_options(request);
      break;

    case ASYNC_HTTP_GET:
      http_handleUIVersion_GET(request);
      break;

    default:
      http_methodNotAllowed(request);
      break;
  }
}


/**
 * Handles UI Version GETs
*/
void http_handleUIVersion_GET(AsyncWebServerRequest *request){

  if(!request->hasHeader(F("visual-token"))){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
    http_unauthorized(request);
    return;
  }

  if(!wwwFS.exists(F("/version.json"))){
    http_notFound(request);
    return;
  }

  AsyncWebServerResponse *response = request->beginResponse(wwwFS, F("/version.json"), F("application/json"));
  request->send(response);
}


/**
 * Lists a given filesystem + path to an ArduinoJSON JsonArray
 */
void listDirToJsonArray(fs::FS &fs, const char *dirname, JsonArray &array) {

  File root = fs.open(dirname);
  if (!root) {
    return;
  }
  if (!root.isDirectory()) {
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      listDirToJsonArray(fs, file.path(), array);
    } else {
      JsonObject entry = array.createNestedObject();
      entry["path"] = (String)file.path();
      entry["size"] = file.size();
    }
    file = root.openNextFile();
  }
}


/**
 * Handles GET requests for all filesystems
 */
void http_handleFileList_GET(AsyncWebServerRequest *request){

  if(request->method() == ASYNC_HTTP_OPTIONS){
    http_options(request);
    return;
  }

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

  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  DynamicJsonDocument doc(49152);  //Supports approx 128 controllers, 128 clients, and 64 files in www
  JsonObject root = doc.to<JsonObject>();
  JsonObject configObject = root.createNestedObject("config");
  JsonObject wwwObject = root.createNestedObject("www");

  if(configFS_isMounted){
    configObject["total"] = configFS.totalBytes();
    configObject["used"] = configFS.usedBytes();
    JsonArray fileList = configObject.createNestedArray(F("files"));
    listDirToJsonArray(configFS, "/", fileList);
  }else{
    configObject["error"] = F("File system not mounted");
  }

  if(wwwFS_isMounted){
    wwwObject["total"] = wwwFS.totalBytes();
    wwwObject["used"] = wwwFS.usedBytes();
    JsonArray fileList = wwwObject.createNestedArray(F("files"));
    listDirToJsonArray(wwwFS, "/", fileList);
  }else{
    wwwObject["error"] = F("File system not mounted");
  }

  serializeJson(doc, *response);
  request->send(response);
}


/**
 * Handles /certs requests
*/
void http_handleCerts(AsyncWebServerRequest *request){

  switch(request->method()){

    case ASYNC_HTTP_OPTIONS:
      http_options(request);
      break;

    case ASYNC_HTTP_POST:

      break; //http_handleCerts_Upload handles all authorization and responses

    case ASYNC_HTTP_GET:
      http_handleCerts_GET(request);
      break;

    default:
      http_methodNotAllowed(request);
      break;
  }

}


/** 
 * Retrieves a list of certificates
*/
void http_handleCerts_GET(AsyncWebServerRequest *request){

  if(!request->hasHeader(F("visual-token"))){
    http_unauthorized(request);
    return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
    http_unauthorized(request);
    return;
  }

  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  StaticJsonDocument<768> doc;
  JsonArray array = doc.to<JsonArray>();

  File root = configFS.open(CONFIGFS_PATH_CERTS + (String)"/");

  File file = root.openNextFile();
  while(file){
      if(!file.isDirectory()){
          JsonObject fileInstance = array.createNestedObject();
          fileInstance[F("file")] = (String)file.name();
          fileInstance[F("size")] = file.size();
      }
      file = root.openNextFile();
    }

  serializeJson(doc, *response);
  request->send(response);

}


/**
 * Handles certificate uploads.  If the certificate already exists, a 403 is returned to the client
*/
void http_handleCerts_Upload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){

  if(request->method() == ASYNC_HTTP_OPTIONS){
    http_options(request);
    return;
  }

  if(!request->hasHeader(F("visual-token"))){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
    http_unauthorized(request);
    return;
  }

  if(request->method()!= ASYNC_HTTP_POST){
    http_methodNotAllowed(request);
    return;
  }

  MatchState ms;
  ms.Target((char*)filename.c_str());

  if(ms.MatchCount("[^a-z0-9_.]") > 0){
    http_badRequest(request, F("Invalid filename; Only [a-z0-9_.] permitted"));
    return;
  };

  if(!index){

    if(configFS.exists(CONFIGFS_PATH_CERTS + (String)"/" + filename)){
      http_forbiddenRequest(request, F("Certificate already exists"));
      return;
    }

    request->_tempFile = configFS.open(CONFIGFS_PATH_CERTS + (String)"/" + filename, "w");
  }

  request->_tempFile.write(data,len);
    
  if(final){
    request->_tempFile.close();
    request->send(201);
  }
}


/**
 * Handles /certs/{file} requests
*/
void http_handleCert(AsyncWebServerRequest *request){
  switch(request->method()){

    case ASYNC_HTTP_OPTIONS:
      http_options(request);
      break;

    case ASYNC_HTTP_GET:

        if(!request->hasHeader(F("visual-token"))){
        http_unauthorized(request);
        return;
      }

      if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
        http_unauthorized(request);
        return;
      }

      http_handleCert_GET(request);
      break;

    case ASYNC_HTTP_DELETE:

      if(!request->hasHeader(F("visual-token"))){
        http_unauthorized(request);
        return;
      }

      if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
        http_unauthorized(request);
        return;
      }

      http_handleCert_DELETE(request);
      break;

    default:
      http_methodNotAllowed(request);
      break;
  }

}


/** 
 * Handles retrieving a specific certificate
*/
void http_handleCert_GET(AsyncWebServerRequest *request){

if(!request->hasHeader(F("visual-token")) && !request->hasHeader(F("mac-address"))){
    http_unauthorized(request);
    return;
}

if(request->hasHeader(F("visual-token"))){
  if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
    http_unauthorized(request);
    return;
  }
}

if(request->hasHeader(F("mac-address"))){

  if(provisioningMode.getStatus() != true){
    http_badRequest(request, F("Provisioning mode inactive"));
    return;
  }

  if(!authClientWithMacAddress(request->pathArg(0).c_str(), request->header(F("mac-address")).c_str())){
    http_forbiddenRequest(request, F("Request will not be fulfilled"));
    return;
  }
}

  if(configFS.exists(CONFIGFS_PATH_CERTS + (String)"/" + request->pathArg(0))){

    AsyncWebServerResponse *response = request->beginResponse(configFS, CONFIGFS_PATH_CERTS + (String)"/" + request->pathArg(0), F("text/plain"));
    response->setCode(200);
    request->send(response);

  }else{
    request->send(404);
  }
}


/**
 * Handles deleting a specific certificate
*/
void http_handleCert_DELETE(AsyncWebServerRequest *request){

  if(!request->hasHeader(F("visual-token"))){
    http_unauthorized(request);
    return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str())){
    http_unauthorized(request);
    return;
  }

  if(configFS.exists(CONFIGFS_PATH_CERTS + (String)"/" + request->pathArg(0))){
    configFS.remove(CONFIGFS_PATH_CERTS + (String)"/" + request->pathArg(0));
    request->send(204);
  }else{
    request->send(404);
  }
}


/**
 * Handles force OTA force update requests using the payload provided
*/
void http_handleOTA_forced_POST(AsyncWebServerRequest *request, JsonVariant doc){

  if(!request->hasHeader(F("visual-token"))){
      http_unauthorized(request);
      return;
  }

  if(!authToken.authenticate(request->header(F("visual-token")).c_str(), true)){
    http_unauthorized(request);
    return;
  }

  if(request->method() != ASYNC_HTTP_POST){
    http_methodNotAllowed(request);
    return;
  }

  if(!doc.containsKey(F("url"))){
    http_badRequest(request, F("Field url is required"));
    return;
  }

  forcedOtaUpdateConfig newFirmwareRequest;
  newFirmwareRequest.url = doc["url"].as<String>();

  if(!newFirmwareRequest.url.endsWith(".bin")){
    http_badRequest(request, F("Bad url; File must be of type '.bin'"));
    return;
  }

  if(!newFirmwareRequest.url.startsWith("http:") && !newFirmwareRequest.url.startsWith("https:")){
    http_badRequest(request, F("Bad url; http or https required"));
    return;
  }

  if(newFirmwareRequest.url.startsWith("https:")){
    if(!doc.containsKey(F("certificate"))){
      http_badRequest(request, F("https requires certificate"));
      return;
    }

    newFirmwareRequest.certificate = doc["certificate"].as<String>();

    if(newFirmwareRequest.certificate == ""){
      http_badRequest(request, F("Certificate cannot be empty"));
      return;
    }

    if(!configFS.exists(CONFIGFS_PATH_CERTS + (String)"/" + newFirmwareRequest.certificate)){
      http_badRequest(request, F("Certificate does not exist"));
      return;
    }
  }

  if(request->url().endsWith("app")){
      newFirmwareRequest.type = OTA_UPDATE_APP;

  }else{
    newFirmwareRequest.type = OTA_UPDATE_SPIFFS;
  }

  otaFirmware.pending.add(newFirmwareRequest);

  request->send(202);
}


/** 
 * Configures the OTA firmware settings
*/
void setup_OtaFirmware(){

  if(strcmp(externalEEPROM.data.uuid, "") == 0){
    return;
  }

  if(configFS_isMounted == false){
    return;
  }

  String filename = CONFIGFS_PATH_CONTROLLERS + (String)"/" + externalEEPROM.data.uuid;

  if(!configFS.exists(filename)){
    return;
  }

  otaFirmware.setSPIFFsPartitionLabel("www");
  otaFirmware.setCertFileSystem(nullptr);

  if(strcmp(externalEEPROM.data.uuid, "") != 0){
    otaFirmware.setExtraHTTPHeader(F("uuid"), externalEEPROM.data.uuid);
    otaFirmware.setExtraHTTPHeader(F("product_id"), externalEEPROM.data.product_id);
  }

  StaticJsonDocument<16> filter;
  filter["ota"] = true;

  StaticJsonDocument<256> doc;

  File file = configFS.open(filename, "r");

  DeserializationError error = deserializeJson(doc, file, DeserializationOption::Filter(filter));

  if (error) {
    char *text = new char[OLED_CHARACTERS_PER_LINE+1];
    snprintf(text, OLED_CHARACTERS_PER_LINE+1, "OTA parse err %s", error.c_str());
    log_e("%s", text);
    eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
    return;
  }

  if(!doc.containsKey(F("ota"))){
    return;
  }

  if(!doc["ota"].containsKey(F("url"))){
    eventLog.createEvent("OTA cfg no url");
    return;
  }

  String url = doc["ota"]["url"];

  if(!url.startsWith("http:") && !url.startsWith("https:")){
    eventLog.createEvent("OTA cfg inv proto");
    return;
  }

  otaFirmware.setManifestURL(url.c_str());

  if(url.startsWith(F("https"))){
    if(!doc["ota"].containsKey(F("certificate"))){
      eventLog.createEvent("OTA cfg no cert");
      return;
    }

    const char* certificate = doc["ota"]["certificate"];
    if(!configFS.exists(CONFIGFS_PATH_CERTS + (String)"/" + certificate)){
      eventLog.createEvent(F("OTA cert missing"), EventLog::LOG_LEVEL_ERROR);
      return;
    }

    otaFirmware.setRootCA(new CryptoFileAsset((CONFIGFS_PATH_CERTS + (String)"/" + certificate).c_str(), &configFS));

  }

  otaFirmware.setProgressCb(eventHandler_otaFirmwareProgress);
  otaFirmware.setUpdateBeginFailCb(eventHandler_otaFirmwareFailed);
  otaFirmware.setUpdateFinishedCb(eventHandler_otaFirmwareFinished);
  otaFirmware.setUpdateAvailableCb(mqtt_publishUpdateAvailable);
  otaFirmware.setUpdateServiceAvailabilityCb(mqtt_publishUpdateServiceAvailability);

  otaFirmware.enabled = true;
  eventLog.createEvent(F("OTA update enabled"));
}


/**
 * Checks if there are any pending OTA firmware updates in the queue.  This is necessary to allow the async web server to raise requests to the main loop.
*/
void otaFirmware_checkPending(){

  if(otaFirmware.pending.size() == 0){
    return;
  }

  if(otaFirmware.updateInProcess == true){
    return;
  }

  for(int i=0; i < otaFirmware.pending.size(); i++){

    exEsp32FOTA::esp32FOTA forceFirmwareUpdate;
    forceFirmwareUpdate.setProgressCb(eventHandler_otaFirmwareProgress);
    forceFirmwareUpdate.setUpdateBeginFailCb(eventHandler_otaFirmwareFailed);
    forceFirmwareUpdate.setUpdateFinishedCb(eventHandler_otaFirmwareFinished);

    bool updateSuccess = false;

    if(otaFirmware.pending.get(i).url.startsWith("https:")){
      otaFirmware.pending.get(i).certificate = CONFIGFS_PATH_CERTS + (String)"/" + otaFirmware.pending.get(i).certificate;
      forceFirmwareUpdate.setRootCA(new CryptoFileAsset(otaFirmware.pending.get(i).certificate.c_str(), &configFS));
    }

    switch(otaFirmware.pending.get(i).type){

      case OTA_UPDATE_APP:
        eventLog.createEvent(F("OTA app forced"));
        updateSuccess = forceFirmwareUpdate.forceUpdate(otaFirmware.pending.get(i).url.c_str(), false);
        break;


      case OTA_UPDATE_SPIFFS:
        eventLog.createEvent(F("OTA SPIFFS forced"));
        updateSuccess = forceFirmwareUpdate.forceUpdateSPIFFS(otaFirmware.pending.get(i).url.c_str(), false);
        break;
    }

    if(!updateSuccess){
      eventLog.createEvent(F("OTA update failed"), EventLog::LOG_LEVEL_NOTIFICATION);
    }

    otaFirmware.pending.remove(i);
  }

}


/**
 * A callback function to report firmware upgrade progress, which draws a status on the OLED
 * @param progress The number of bytes that have been processed so far
 * @param size The total size of the update in bytes 
*/
void eventHandler_otaFirmwareProgress(size_t progress, size_t size){

  otaFirmware.updateInProcess = true;

  float percentage = ((float)progress/(float)size);

  if(progress == 0){
    eventLog.createEvent("OTA firmware started", EventLog::LOG_LEVEL_NOTIFICATION);
    oled.setPage(managerOled::PAGE_OTA_IN_PROGRESS);
  }

  oled.setProgressBar(percentage);

  char* topic = new char[MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH+1];
  snprintf(topic, MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH+1, MQTT_TOPIC_UPDATE_STATE_PATTERN, externalEEPROM.data.uuid);

  StaticJsonDocument<32> doc;
  doc["in_progress"] = true;
  doc["update_percentage"] = int(percentage*100);

  char buffer[256];
  serializeJson(doc, buffer);

  mqttClient.publish(topic, buffer);
}


/**
 * Callback function to report that the update has failed
 * @param partition inherited from esp32FOTA library
*/
void eventHandler_otaFirmwareFailed(int partition){
  log_e("Failed partition: [%i]", partition);
  eventLog.createEvent("OTA firmware failed", EventLog::LOG_LEVEL_NOTIFICATION);

  char* topic = new char[MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH+1];
  snprintf(topic, MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH+1, MQTT_TOPIC_UPDATE_STATE_PATTERN, externalEEPROM.data.uuid);

  StaticJsonDocument<16> doc;
  doc["in_progress"] = false;

  char buffer[256];
  serializeJson(doc, buffer);

  mqttClient.publish(topic, buffer);
}


/**
 * A callback function for when the firmware update has finished
*/
void eventHandler_otaFirmwareFinished(int partition, bool needs_restart){

  log_i("Finished Partition: [%i] needs restart: [%s]", partition, needs_restart ? "true":"false");

  eventLog.createEvent(F("OTA update finished"), EventLog::LOG_LEVEL_NOTIFICATION);

  if(needs_restart){
      eventLog.createEvent(F("Rebooting..."), EventLog::LOG_LEVEL_NOTIFICATION);
      delay(5000);
  }

  otaFirmware.updateInProcess = false;
}


/***
 * Configures the I/O
 */
void setupIO(){

  bool isOK = true;

  if(strcmp(externalEEPROM.data.uuid, "") == 0){
    eventLog.createEvent(F("No I/O setup (EEPROM)"), EventLog::LOG_LEVEL_ERROR);
    return;
  }

  if(configFS_isMounted == false){
    eventLog.createEvent(F("No I/O ConfigFS offline"), EventLog::LOG_LEVEL_ERROR);
    return;
  }

  String filename = CONFIGFS_PATH_CONTROLLERS + (String)"/" + externalEEPROM.data.uuid;

  if(!configFS.exists(filename)){
    eventLog.createEvent(F("No I/O file to read"));
    return;
  }

  if(!setup_outputs(filename)){
    isOK = false;
  }

  if(!setup_inputs(filename)){
    isOK = false;
  };

  if(isOK){
      eventLog.createEvent(F("I/O setup read OK"));
  }else{
    eventLog.createEvent(F("I/O setup read fail"));
  }
}


/***
 * Sets up the outputs from the JSON document stored on ConfigFS.
 * 
 * @param filename the filename of the config file to use, which should already have been checked to exist
 * @note if the JSON document describes ports which are greater in number to the maximum number of ports for this hardware, they are ignored.  The JSON document is filtered before being read
 */
bool setup_outputs(String filename){

  boolean isOK = true;
  StaticJsonDocument<112> filter;

  JsonObject filter_outputs__ = filter["outputs"].createNestedObject("*");
  filter_outputs__["id"] = true;
  filter_outputs__["type"] = true;
  filter_outputs__["enabled"] = true;

  DynamicJsonDocument doc(3072); //Supports up to 32 ports

  File file = configFS.open(filename, "r");

  DeserializationError error = deserializeJson(doc, file, DeserializationOption::Filter(filter));

  if (error) {
    char *text = new char[OLED_CHARACTERS_PER_LINE+1];
    snprintf(text, OLED_CHARACTERS_PER_LINE+1, "Out parse err %s", error.c_str());
    log_e("%s", text);
    eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
    return false;
  }

  for (JsonPair output : doc["outputs"].as<JsonObject>()) {

    int8_t outputPortNumber = atoi(output.key().c_str());

    if(outputPortNumber > (OUTPUT_CONTROLLER_COUNT * OUTPUT_CONTROLLER_COUNT_PINS)){
      char *text = new char[OLED_CHARACTERS_PER_LINE+1];
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "Out prt %s > max", output.key().c_str());
      log_e("%s", text);
      eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
      isOK = false;
      continue;
    }

    if(outputPortNumber < 1){
      char *text = new char[OLED_CHARACTERS_PER_LINE+1];
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "Out prt %s < 1", output.key().c_str());
      log_e("%s", text);
      eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
      isOK = false;
      continue;
    }

    if(!output.value().containsKey(F("id"))){
      char *text = new char[OLED_CHARACTERS_PER_LINE+1];
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "Out prt %s no id", output.key().c_str());
      log_e("%s", text);
      eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
      isOK = false;
      continue;
    }

    outputs.setPortId(outputPortNumber, output.value()["id"]);

    if(output.value().containsKey(F("type"))){
      if(strcmp(output.value()["type"], "VARIABLE") == 0){
        outputs.setPortType(outputPortNumber, nsOutputs::outputPin::VARIABLE);
      }
    }

    if(output.value().containsKey(F("enabled"))){
      outputs.enablePort(outputPortNumber, output.value()["enabled"].as<boolean>());
    }

  }

  return isOK;
}


/***
 * Sets up the inputs and actions from the JSON document stored on ConfigFS.
 * 
 * @param filename the filename of the config file to use, which should already have been checked to exist
 * @returns true on success, false on failure
 * @note if the JSON document describes ports which are greater in number to the maximum number of ports for this hardware, they are ignored.  The JSON document is filtered before being read
 */
bool setup_inputs(String filename){

  boolean isOK = true;
  StaticJsonDocument<160> filter;

  JsonObject filter_ports = filter["ports"].createNestedObject("*");
  filter_ports["id"] = true;

  JsonObject filter_ports_channels = filter_ports["channels"].createNestedObject("*");
  filter_ports_channels["type"] = true;
  filter_ports_channels["enabled"] = true;
  filter_ports_channels["offset"] = true;
  filter_ports_channels["actions"] = true;

  DynamicJsonDocument doc(32768); //Supports up to 32 ports

  File file = configFS.open(filename, "r");

  DeserializationError error = deserializeJson(doc, file, DeserializationOption::Filter(filter));

  file.close();

  if (error) {
    char *text = new char[OLED_CHARACTERS_PER_LINE+1];
    snprintf(text, OLED_CHARACTERS_PER_LINE+1, "In parse err %s", error.c_str());
    log_e("%s", text);
    eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
    return false;
  }

  for (JsonPair port : doc["ports"].as<JsonObject>()) {

    if(atoi(port.key().c_str()) > (IO_EXTENDER_COUNT_PINS / IO_EXTENDER_COUNT_CHANNELS_PER_PORT) * IO_EXTENDER_COUNT){
      char *text = new char[OLED_CHARACTERS_PER_LINE+1];
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "In prt %s > max", port.key().c_str());
      log_e("%s", text);
      eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
      isOK = false;
      continue;
    }

    if(atoi(port.key().c_str()) < 1){
      char *text = new char[OLED_CHARACTERS_PER_LINE+1];
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "In prt %s < 1", port.key().c_str());
      log_e("%s", text);
      eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
      isOK = false;
      continue;
    }

    strlcpy(inputPorts[atoi(port.key().c_str())-1].id, port.value()["id"], sizeof(inputPorts[atoi(port.key().c_str())-1].id));

    uint8_t i = 0;

    for (JsonPair port_value_channel : port.value()["channels"].as<JsonObject>()){

      if(i > IO_EXTENDER_COUNT_CHANNELS_PER_PORT){
        char *text = new char[OLED_CHARACTERS_PER_LINE+1];
        snprintf(text, OLED_CHARACTERS_PER_LINE+1, "In prt %s ch > max", port.key().c_str());
        log_e("%s", text);
        eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
        isOK = false;
        continue;
      }

      managerInputs::portChannel portChannel;

      portChannel.port = atoi(port.key().c_str());
      portChannel.channel = atoi(port_value_channel.key().c_str());
      inputPorts[atoi(port.key().c_str())-1].channels[i].channel = portChannel.channel;

      if(port_value_channel.value()["type"]){
        if(port_value_channel.value()["type"] != F("NORMALLY_OPEN")){
          inputs.setPortChannelInputType(portChannel, managerInputs::NORMALLY_CLOSED);
        }
      }

      if(port_value_channel.value()["enabled"]){
        inputs.enablePortChannel(portChannel, port_value_channel.value()["enabled"].as<boolean>());
      }

      if(port_value_channel.value()["offset"]){
        inputs.setOffset(portChannel, port_value_channel.value()["offset"].as<uint8_t>());
      }

      for (JsonObject port_value_channel_value_action : port_value_channel.value()["actions"].as<JsonArray>()) {

        bool actionIsOK = false;

        inputAction newInputAction;

        if(port_value_channel_value_action.containsKey(F("action"))){
        
          if(strcmp(port_value_channel_value_action["action"], "INCREASE") == 0){
            newInputAction.action = INCREASE;
            actionIsOK = true;
          }

          if(strcmp(port_value_channel_value_action["action"], "INCREASE_MAXIMUM") == 0){
            newInputAction.action = INCREASE_MAXIMUM;
            actionIsOK = true;
          }

          if(strcmp(port_value_channel_value_action["action"], "DECREASE") == 0){
            newInputAction.action = DECREASE;
            actionIsOK = true;
          }

          if(strcmp(port_value_channel_value_action["action"], "DECREASE_MAXIMUM") == 0){
            newInputAction.action = DECREASE_MAXIMUM;
            actionIsOK = true;
          }

          if(strcmp(port_value_channel_value_action["action"], "TOGGLE") == 0){
            newInputAction.action = TOGGLE;
            actionIsOK = true;
          }
        }

        if(!port_value_channel_value_action.containsKey(F("change_state"))){
          newInputAction.changeState = managerInputs::changeState::CHANGE_STATE_SHORT_DURATION;
        }else{
          if(strcmp(port_value_channel_value_action["change_state"], "LONG") == 0){
            newInputAction.changeState = managerInputs::changeState::CHANGE_STATE_LONG_DURATION;
          }
        }

        newInputAction.output = port_value_channel_value_action["output"].as<uint8_t>();

        if(newInputAction.output == 0){
          actionIsOK = false;
        }

        if(actionIsOK){
          inputPorts[atoi(port.key().c_str())-1].channels[i].actions.add(newInputAction);
        }else{
          char *text = new char[OLED_CHARACTERS_PER_LINE+1];
          snprintf(text, OLED_CHARACTERS_PER_LINE+1, "In prt %s ch %s inv act", port.key().c_str(), port_value_channel.key().c_str());
          log_e("%s", text);
          eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
          isOK = false;
        }
      }

      i++;
    }
  }
  return isOK;
}


/***
 * Attempts to reconnect to MQTT.  If the reconnect is not successful, the process will sleep until the timer elapses.
 * 
 */
void mqtt_reconnect(){

  if(!mqttClient.enabled){
    return;
  }

  if((esp_timer_get_time() - mqttClient.lastReconnectAttemptTime) / 1000  > MQTT_RECONNECT_WAIT_MILLISECONDS || mqttClient.lastReconnectAttemptTime == 0){

    if(!mqttClient.connected()){

      if(mqttClient.connect(externalEEPROM.data.uuid, mqttClient.username, mqttClient.password, mqttClient.topic_availability, 2, true, "offline")){
        mqttClient.lastReconnectAttemptTime = 0;
        return;
      }

      mqttClient.lastReconnectAttemptTime = esp_timer_get_time();
    }
  }
}


/***
 * Sets up the MQTT client for use
 */
void setupMQTT(){

  char *topic_availability = new char[MQTT_TOPIC_CONTROLLER_AVAILABILITY_LENGTH+1];
  snprintf(topic_availability, MQTT_TOPIC_CONTROLLER_AVAILABILITY_LENGTH+1, MQTT_TOPIC_CONTROLLER_AVAILABILITY_PATTERN, externalEEPROM.data.uuid);
  mqttClient.topic_availability = topic_availability;
  mqttClient.setCallback(eventHandler_mqttMessageReceived);
  mqttClient.setCallback_Connect(eventHandler_mqttConnect);
  mqttClient.setCallback_Disconnect(eventHandler_mqttDisconnect);

  String filename = CONFIGFS_PATH_CONTROLLERS + (String)"/" + externalEEPROM.data.uuid;

  if(!configFS.exists(filename)){
    return;
  }

  File file = configFS.open(filename, "r");

  StaticJsonDocument<48> filter;
  filter["name"] = true;
  filter["area"] = true;
  filter["mqtt"] = true;

  DynamicJsonDocument doc(512);

  DeserializationError error = deserializeJson(doc, file, DeserializationOption::Filter(filter));

  file.close();

  if (error) {
    char *text = new char[OLED_CHARACTERS_PER_LINE+1];
    snprintf(text, OLED_CHARACTERS_PER_LINE+1, "MQTT parse err %s", error.c_str());
    log_e("%s", text);
    eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
    return;
  }

  if(doc.containsKey(F("name"))){
    mqttClient.autoDiscovery.setDeviceName(doc["name"].as<String>().c_str());
  }

  if(doc.containsKey(F("area"))){
    mqttClient.autoDiscovery.setSuggestedArea(doc["area"].as<String>().c_str());
  }

  if(!doc.containsKey(F("mqtt"))){
    char *text = new char[OLED_CHARACTERS_PER_LINE+1];
    snprintf(text, OLED_CHARACTERS_PER_LINE+1, "MQTT obj missing");
    log_e("%s", text);
    eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
    return;
  }

  JsonObject mqtt = doc["mqtt"];
  uint16_t port = 1883;

  if(!mqtt.containsKey(F("host"))){
    char *text = new char[OLED_CHARACTERS_PER_LINE+1];
    snprintf(text, OLED_CHARACTERS_PER_LINE+1, "MQTT host missing");
    log_e("%s", text);
    eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
    return;
  }

  if(!mqtt.containsKey(F("username"))){
    char *text = new char[OLED_CHARACTERS_PER_LINE+1];
    snprintf(text, OLED_CHARACTERS_PER_LINE+1, "MQTT user missing");
    log_e("%s", text);
    eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
    return;
  }

  if(!mqtt.containsKey(F("password"))){
    char *text = new char[OLED_CHARACTERS_PER_LINE+1];
    snprintf(text, OLED_CHARACTERS_PER_LINE+1, "MQTT pass missing");
    log_e("%s", text);
    eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
    return;
  }

  if(mqtt.containsKey(F("port"))){
    port = mqtt["port"];
  }

  mqttClient.setServer(mqtt["host"].as<const char*>(), port);

  uint8_t baseMac[6];
  esp_read_mac(baseMac, ESP_MAC_ETH);

  char macOnly[13] = {0};
  sprintf(macOnly, "%02X%02X%02X%02X%02X%02X", baseMac[0],baseMac[1],baseMac[2],baseMac[3], baseMac[4], baseMac[5]);
  
  char macDashes[18] = {0};
  sprintf(macDashes, "%02X-%02X-%02X-%02X-%02X-%02X", baseMac[0],baseMac[1],baseMac[2],baseMac[3], baseMac[4], baseMac[5]);

  char macColons[18] = {0};
  sprintf(macColons, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0],baseMac[1],baseMac[2],baseMac[3], baseMac[4], baseMac[5]);

  String username = mqtt["username"].as<String>();
  username.replace("$$mac$$", macOnly);
  username.replace("$$mac_dashes$$", macDashes);
  username.replace("$$mac_colons$$", macColons);
  username.replace("$$uuid$$", externalEEPROM.data.uuid);
  mqttClient.setUsername(username.c_str());

  String password = mqtt["password"].as<String>();
  password.replace("$$mac$$", macOnly);
  password.replace("$$mac_dashes$$", macDashes);
  password.replace("$$mac_colons$$", macColons);
  password.replace("$$uuid$$", externalEEPROM.data.uuid);
  mqttClient.setPassword(password.c_str());

  mqttClient.enabled = true;
}


/***
 * Handles MQTT message received events for subscribed topics
 */
void eventHandler_mqttMessageReceived(char* topic, byte* pl, unsigned int length)
{

  if(length > 10){
    return; //Protect from abusive messages
  }

  String payload = "";
 
  for (unsigned int i = 0; i < length; i++) {
    payload = String(payload + (char)pl[i]);
  }

  payload.trim();

  MatchState ms;
  ms.Target(topic);

  if(ms.Match(MQTT_TOPIC_OUTPUT_SET_REGEX)){ //This is an output command request

    payload.toUpperCase();

    if(payload == "ON"){ //Non-numeric values will be changed to "0"
      payload = "100";
    }

    outputs.setPortValue(ms.GetCapture(topic, 0), payload.toInt());
    return;
  }

  if(ms.Match(MQTT_TOPIC_UPDATE_SET_REGEX)){ //This is an update command request

    char* topic = new char[MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH+1];
    snprintf(topic, MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH+1, MQTT_TOPIC_UPDATE_STATE_PATTERN, externalEEPROM.data.uuid);

    StaticJsonDocument<16> doc;
    doc["in_progress"] = true;
  
    char buffer[256];
    serializeJson(doc, buffer);
  
    mqttClient.publish(topic, buffer);

    otaFirmware.execOTA();
    return;
  } 
}


/**
 * Handles MQTT connection events
 */
void eventHandler_mqttConnect(){
  eventLog.createEvent("MQTT connected");
  eventLog.resolveError("MQTT disconnected");
  mqttClient.publish(mqttClient.topic_availability, "online", true);
  mqttClient.resubscribe();

  if(!mqttClient.autoDiscovery.sent){
    mqtt_autoDiscovery_update();
    mqtt_publishUpdateServiceAvailability(exEsp32FOTA::lastHTTPCheckStatus::NEVER_ATTEMPTED);
    mqtt_autoDiscovery_temperature();
    mqtt_autoDiscovery_outputs();
    mqtt_autoDiscovery_start_time();
    mqtt_autoDiscovery_ip_address();
    mqtt_autoDiscovery_mac_address();
    mqtt_autoDiscovery_count_errors();
    mqttClient.autoDiscovery.sent = true;
  }

  mqtt_publishTemperatures();
  mqtt_publishStartTime();
  mqtt_publishIPAddress();
  mqtt_publishMACAddress();
  mqtt_publishCountErrors();
}


/**
 * Handles MQTT disconnect events
 */
void eventHandler_mqttDisconnect(int8_t errorNumber){
  char *text = new char[OLED_CHARACTERS_PER_LINE+1];

  switch(errorNumber){

    case -4:
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "MQTT conn timeout");
      break;

    case -3:
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "MQTT conn lost");
      break;

    case -2:
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "MQTT conn fail");
      break;

    case -1:
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "MQTT conn disconnect");
      break;

    case 0:
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "MQTT connected");
      break;

    case 1:
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "MQTT bad protocol");
      break;

    case 2:
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "MQTT bad client ID");
      break;

    case 3:
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "MQTT unavailable");
      break;

    case 4:
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "MQTT bad creds");
      break;

    case 5:
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "MQTT unauthorized");
      break;

    default:
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "Unknown MQTT state");
      break;
  }

  log_v("%s", text);
  eventLog.createEvent(text);
  eventLog.createEvent("MQTT disconnected", EventLog::LOG_LEVEL_ERROR);
}


/**
 * Handles temperature sensor auto discovery broadcasts
 */
void mqtt_autoDiscovery_temperature(){

  for(int i = 0; i < TEMPERATURE_SENSOR_COUNT; i++){

    const char* sensorLocation = temperatureSensors.getSensorLocation(i);

    DynamicJsonDocument doc(1024);

    char* topic = new char[MQTT_TOPIC_TEMPERATURE_AUTO_DISCOVERY_LENGTH+1];
    snprintf(topic, MQTT_TOPIC_TEMPERATURE_AUTO_DISCOVERY_LENGTH+1, MQTT_TOPIC_TEMPERATURE_AUTO_DISCOVERY_PATTERN, mqttClient.autoDiscovery.homeAssistantRoot, externalEEPROM.data.uuid, sensorLocation);

    char* unique_id = new char[MQTT_TEMPERATURE_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1];
    snprintf(unique_id, MQTT_TEMPERATURE_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1, MQTT_TEMPERATURE_AUTO_DISCOVERY_UNIQUE_ID_PATTERN, externalEEPROM.data.uuid, sensorLocation);

    char* state_topic = new char[MQTT_TOPIC_TEMPERATURE_STATE_PATTERN_LENGTH+1];
    snprintf(state_topic, MQTT_TOPIC_TEMPERATURE_STATE_PATTERN_LENGTH+1, MQTT_TOPIC_TEMPERATURE_STATE_PATTERN, externalEEPROM.data.uuid, sensorLocation);

    doc["name"] = sensorLocation;
    doc["unique_id"] = unique_id;
    doc["object_id"] = unique_id;
    doc["icon"] = "mdi:thermometer";
    doc["device_class"] = "temperature";
    doc["unit_of_measurement"] = "°C";

    JsonObject device = doc.createNestedObject("device");
    JsonArray identifiers = device.createNestedArray(F("identifiers"));
    identifiers.add(externalEEPROM.data.uuid);

    if(strlen(mqttClient.autoDiscovery.deviceName) > 0){
      device["name"] =  mqttClient.autoDiscovery.deviceName;
    }

    device["manufacturer"] = HARDWARE_MANUFACTURER_NAME;
    device["model"] = APPLICATION_NAME;
    device["model_id"] = externalEEPROM.data.product_id;
    device["serial_number"] = externalEEPROM.data.uuid;
    device["sw_version"] = VERSION;

    if(strlen(mqttClient.autoDiscovery.suggestedArea) > 0){
      device["suggested_area"] =  mqttClient.autoDiscovery.suggestedArea;
    }

    doc["state_topic"] = state_topic;
    doc["availability_topic"] = mqttClient.topic_availability;

    mqttClient.beginPublish(topic, measureJson(doc), true);
    BufferingPrint bufferedClient(mqttClient, 32);
    serializeJson(doc, bufferedClient);
    bufferedClient.flush();
    mqttClient.endPublish();
  }
}


/**
 * Publishes the currently observed temperatures for all sensor locations to MQTT
 */
void mqtt_publishTemperatures(){

  for(int i = 0; i < TEMPERATURE_SENSOR_COUNT; i++){

    char* temperature = new char[6];
    snprintf(temperature, 6, "%.2f", temperatureSensors.getCurrentTemp(i));

    char* topic = new char[MQTT_TOPIC_TEMPERATURE_STATE_PATTERN_LENGTH+1];
    snprintf(topic, MQTT_TOPIC_TEMPERATURE_STATE_PATTERN_LENGTH+1, MQTT_TOPIC_TEMPERATURE_STATE_PATTERN, externalEEPROM.data.uuid, temperatureSensors.getSensorLocation(i));

    mqttClient.publish(topic, temperature, true);
  }
}


/**
 * Handles output auto discovery broadcasts
 */
void mqtt_autoDiscovery_outputs(){

  if(!configFS.exists(CONFIGFS_PATH_CONTROLLERS + (String)"/" + externalEEPROM.data.uuid)){
    log_v("Controller config file does not exist");
    return;
  };

  boolean isOK = true;
  StaticJsonDocument<128> controllerFilterDoc;

  JsonObject filter_outputs__ = controllerFilterDoc["outputs"].createNestedObject("*");
  filter_outputs__["id"] = true;
  filter_outputs__["name"] = true;
  filter_outputs__["area"] = true;
  filter_outputs__["icon"] = true;
  filter_outputs__["type"] = true;

  DynamicJsonDocument controllerDoc(12288); //Supports up to 32 ports

  File controllerFile = configFS.open(CONFIGFS_PATH_CONTROLLERS + (String)"/" + externalEEPROM.data.uuid, "r");
  DeserializationError errorControllerFileDeserialization = deserializeJson(controllerDoc, controllerFile, DeserializationOption::Filter(controllerFilterDoc));
  controllerFile.close();

  if (errorControllerFileDeserialization) {
    return;
  }

  
  for (JsonPair output : controllerDoc["outputs"].as<JsonObject>()) {

    int8_t outputPortNumber = atoi(output.key().c_str());

    if(outputPortNumber > (OUTPUT_CONTROLLER_COUNT * OUTPUT_CONTROLLER_COUNT_PINS)){
      continue;
    }

    if(outputPortNumber < 1){
      continue;
    }

    if(!output.value().containsKey(F("id"))){
      continue;
    }

    char* devicePlatform = new char[WORD_LENGTH_INTEGRATION+1];
    strcpy(devicePlatform,"switch"); //Default

    if(output.value().containsKey(F("icon"))){

      if(output.value()["icon"].as<String>().indexOf("light") != -1){
        strcpy(devicePlatform,"light");
      }

      if(output.value()["icon"].as<String>().indexOf("sconce") != -1){
        strcpy(devicePlatform,"light");
      }

      if(output.value()["icon"].as<String>().indexOf("lamp") != -1){
        strcpy(devicePlatform,"light");
      }

      if(output.value()["icon"].as<String>().indexOf("chandelier") != -1){
        strcpy(devicePlatform,"light");
      }

      if(output.value()["icon"].as<String>().indexOf("fan") != -1){
        strcpy(devicePlatform,"fan");
      }
    }

    bool isVariableOutput = false;

    if(output.value().containsKey(F("type"))){
      if(strcmp(output.value()["type"].as<const char*>(), "VARIABLE") == 0){
        isVariableOutput = true;
      }
    }

    DynamicJsonDocument mqttDoc(1024);

    char* autodiscovery_topic = new char[MQTT_TOPIC_OUTPUT_AUTO_DISCOVERY_LENGTH+1];
    snprintf(autodiscovery_topic, MQTT_TOPIC_OUTPUT_AUTO_DISCOVERY_LENGTH+1, MQTT_TOPIC_OUTPUT_AUTO_DISCOVERY_PATTERN, mqttClient.autoDiscovery.homeAssistantRoot, devicePlatform, output.value()["id"].as<const char*>());

    char* unique_id = new char[MQTT_OUTPUT_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1];
    snprintf(unique_id, MQTT_OUTPUT_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1, MQTT_OUTPUT_AUTO_DISCOVERY_UNIQUE_ID_PATTERN, output.value()["id"].as<const char*>());

    char* state_topic = new char[MQTT_TOPIC_OUTPUT_STATE_LENGTH+1];
    snprintf(state_topic, MQTT_TOPIC_OUTPUT_STATE_LENGTH+1, MQTT_TOPIC_OUTPUT_STATE_PATTERN, output.value()["id"].as<const char*>());

    char* command_topic = new char[MQTT_TOPIC_OUTPUT_SET_LENGTH+1];
    snprintf(command_topic, MQTT_TOPIC_OUTPUT_SET_LENGTH+1, MQTT_TOPIC_OUTPUT_SET_PATTERN, output.value()["id"].as<const char*>());

    mqttDoc["name"] = (char*)NULL;
    mqttDoc["unique_id"] = unique_id;
    mqttDoc["object_id"] = unique_id;

    if(output.value().containsKey(F("icon"))){
        mqttDoc["icon"] = output.value()["icon"].as<const char*>();
    }

    if(isVariableOutput){
      mqttDoc["on_command_type"] = "brightness";
      mqttDoc["brightness_scale"] = 100;
      mqttDoc["brightness_command_topic"] = command_topic;
      mqttDoc["brightness_state_topic"] = state_topic;
    }

    mqttDoc["state_value_template"] = "{% if value|int > 0 %}ON{% else %}OFF{% endif %}";

    JsonObject device = mqttDoc.createNestedObject("device");
    JsonArray identifiers = device.createNestedArray(F("identifiers"));
    identifiers.add(unique_id);

    if(output.value().containsKey(F("name"))){
      device["name"] = output.value()["name"].as<const char*>();
    }else{
      device["name"] = output.value()["id"].as<const char*>();
    }

    device["via_device"] = externalEEPROM.data.uuid;

    if(output.value().containsKey(F("area"))){
      device["suggested_area"] =  output.value()["area"].as<const char*>();
    }
    mqttDoc["state_topic"] = state_topic;
    mqttDoc["command_topic"] = command_topic;
    mqttDoc["availability_topic"] = mqttClient.topic_availability;

    mqttClient.beginPublish(autodiscovery_topic, measureJson(mqttDoc), true);
    BufferingPrint bufferedClient(mqttClient, 32);
    serializeJson(mqttDoc, bufferedClient);
    bufferedClient.flush();
    mqttClient.endPublish();

    mqttClient.addSubscription(command_topic);

    mqttClient.publish(state_topic, "0", true); //By default the controller will have set the output to off
  }
}


/**
 * Handles start time sensor auto discovery broadcasts
 */
void mqtt_autoDiscovery_start_time(){

  DynamicJsonDocument doc(1024);

  char* topic = new char[MQTT_TOPIC_TIME_START_AUTO_DISCOVERY_LENGTH+1];
  snprintf(topic, MQTT_TOPIC_TIME_START_AUTO_DISCOVERY_LENGTH+1, MQTT_TOPIC_TIME_START_AUTO_DISCOVERY_PATTERN, mqttClient.autoDiscovery.homeAssistantRoot, externalEEPROM.data.uuid);

  char* unique_id = new char[MQTT_TIME_START_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1];
  snprintf(unique_id, MQTT_TIME_START_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1, MQTT_TIME_START_AUTO_DISCOVERY_UNIQUE_ID_PATTERN, externalEEPROM.data.uuid);

  char* state_topic = new char[MQTT_TOPIC_TIME_START_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, MQTT_TOPIC_TIME_START_STATE_PATTERN_LENGTH+1, MQTT_TOPIC_TIME_START_STATE_PATTERN, externalEEPROM.data.uuid);

  doc["name"] = "Start Time";
  doc["unique_id"] = unique_id;
  doc["object_id"] = unique_id;
  doc["icon"] = "mdi:calendar-clock";
  doc["entity_category"] = "diagnostic";

  JsonObject device = doc.createNestedObject("device");
  JsonArray identifiers = device.createNestedArray(F("identifiers"));
  identifiers.add(externalEEPROM.data.uuid);

  if(strlen(mqttClient.autoDiscovery.deviceName) > 0){
    device["name"] =  mqttClient.autoDiscovery.deviceName;
  }

  device["manufacturer"] = HARDWARE_MANUFACTURER_NAME;
  device["model"] = APPLICATION_NAME;
  device["model_id"] = externalEEPROM.data.product_id;
  device["serial_number"] = externalEEPROM.data.uuid;
  device["sw_version"] = VERSION;

  if(strlen(mqttClient.autoDiscovery.suggestedArea) > 0){
        device["suggested_area"] =  mqttClient.autoDiscovery.suggestedArea;
  }

  doc["state_topic"] = state_topic;
  doc["value_template"] = "{{ ( value | int ) | timestamp_local }}";
  doc["availability_topic"] = mqttClient.topic_availability;

  mqttClient.beginPublish(topic, measureJson(doc), true);
  BufferingPrint bufferedClient(mqttClient, 32);
  serializeJson(doc, bufferedClient);
  bufferedClient.flush();
  mqttClient.endPublish();
}


/**
 * Publishes the start time to MQTT
 */
void mqtt_publishStartTime(){

  char* state_topic = new char[MQTT_TOPIC_TIME_START_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, MQTT_TOPIC_TIME_START_STATE_PATTERN_LENGTH+1, MQTT_TOPIC_TIME_START_STATE_PATTERN, externalEEPROM.data.uuid);

  char* start_time = new char[21];
  snprintf(start_time, 21, "%i", bootTime);

  mqttClient.publish(state_topic, start_time, true);
}


/**
 * Handles MAC address sensor auto discovery broadcasts
 */
void mqtt_autoDiscovery_mac_address(){

  DynamicJsonDocument doc(1024);

  char* topic = new char[MQTT_TOPIC_MAC_ADDRESS_AUTO_DISCOVERY_LENGTH+1];
  snprintf(topic, MQTT_TOPIC_MAC_ADDRESS_AUTO_DISCOVERY_LENGTH+1, MQTT_TOPIC_MAC_ADDRESS_AUTO_DISCOVERY_PATTERN, mqttClient.autoDiscovery.homeAssistantRoot, externalEEPROM.data.uuid);

  char* unique_id = new char[MQTT_MAC_ADDRESS_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1];
  snprintf(unique_id, MQTT_MAC_ADDRESS_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1, MQTT_MAC_ADDRESS_AUTO_DISCOVERY_UNIQUE_ID_PATTERN, externalEEPROM.data.uuid);

  char* state_topic = new char[MQTT_TOPIC_MAC_ADDRESS_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, MQTT_TOPIC_MAC_ADDRESS_STATE_PATTERN_LENGTH+1, MQTT_TOPIC_MAC_ADDRESS_STATE_PATTERN, externalEEPROM.data.uuid);

  doc["name"] = "MAC Address";
  doc["unique_id"] = unique_id;
  doc["object_id"] = unique_id;
  doc["icon"] = "mdi:ethernet";
  doc["entity_category"] = "diagnostic";

  JsonObject device = doc.createNestedObject("device");
  JsonArray identifiers = device.createNestedArray(F("identifiers"));
  identifiers.add(externalEEPROM.data.uuid);

  if(strlen(mqttClient.autoDiscovery.deviceName) > 0){
    device["name"] =  mqttClient.autoDiscovery.deviceName;
  }

  device["manufacturer"] = HARDWARE_MANUFACTURER_NAME;
  device["model"] = APPLICATION_NAME;
  device["model_id"] = externalEEPROM.data.product_id;
  device["serial_number"] = externalEEPROM.data.uuid;
  device["sw_version"] = VERSION;

  if(strlen(mqttClient.autoDiscovery.suggestedArea) > 0){
        device["suggested_area"] =  mqttClient.autoDiscovery.suggestedArea;
  }

  doc["state_topic"] = state_topic;
  doc["availability_topic"] = mqttClient.topic_availability;

  mqttClient.beginPublish(topic, measureJson(doc), true);
  BufferingPrint bufferedClient(mqttClient, 32);
  serializeJson(doc, bufferedClient);
  bufferedClient.flush();
  mqttClient.endPublish();
}


/**
 * Publishes the MAC Address to MQTT
 */
void mqtt_publishMACAddress(){

  char* state_topic = new char[MQTT_TOPIC_MAC_ADDRESS_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, MQTT_TOPIC_MAC_ADDRESS_STATE_PATTERN_LENGTH+1, MQTT_TOPIC_MAC_ADDRESS_STATE_PATTERN, externalEEPROM.data.uuid);

  uint8_t ethMac[6];
  esp_read_mac(ethMac, ESP_MAC_ETH);
  char macAddress[18] = {0};
  sprintf(macAddress, "%02X:%02X:%02X:%02X:%02X:%02X", ethMac[0], ethMac[1], ethMac[2], ethMac[3], ethMac[4], ethMac[5]);

  mqttClient.publish(state_topic, macAddress, true);
}


/**
 * Handles IP address sensor auto discovery broadcasts
 */
void mqtt_autoDiscovery_ip_address(){

  DynamicJsonDocument doc(1024);

  char* topic = new char[MQTT_TOPIC_IP_ADDRESS_AUTO_DISCOVERY_LENGTH+1];
  snprintf(topic, MQTT_TOPIC_IP_ADDRESS_AUTO_DISCOVERY_LENGTH+1, MQTT_TOPIC_IP_ADDRESS_AUTO_DISCOVERY_PATTERN, mqttClient.autoDiscovery.homeAssistantRoot, externalEEPROM.data.uuid);

  char* unique_id = new char[MQTT_IP_ADDRESS_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1];
  snprintf(unique_id, MQTT_IP_ADDRESS_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1, MQTT_IP_ADDRESS_AUTO_DISCOVERY_UNIQUE_ID_PATTERN, externalEEPROM.data.uuid);

  char* state_topic = new char[MQTT_TOPIC_IP_ADDRESS_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, MQTT_TOPIC_IP_ADDRESS_STATE_PATTERN_LENGTH+1, MQTT_TOPIC_IP_ADDRESS_STATE_PATTERN, externalEEPROM.data.uuid);

  doc["name"] = "IP Address";
  doc["unique_id"] = unique_id;
  doc["object_id"] = unique_id;
  doc["icon"] = "mdi:ip";
  doc["entity_category"] = "diagnostic";

  JsonObject device = doc.createNestedObject("device");
  JsonArray identifiers = device.createNestedArray(F("identifiers"));
  identifiers.add(externalEEPROM.data.uuid);

  if(strlen(mqttClient.autoDiscovery.deviceName) > 0){
    device["name"] =  mqttClient.autoDiscovery.deviceName;
  }

  device["manufacturer"] = HARDWARE_MANUFACTURER_NAME;
  device["model"] = APPLICATION_NAME;
  device["model_id"] = externalEEPROM.data.product_id;
  device["serial_number"] = externalEEPROM.data.uuid;
  device["sw_version"] = VERSION;

  if(strlen(mqttClient.autoDiscovery.suggestedArea) > 0){
        device["suggested_area"] =  mqttClient.autoDiscovery.suggestedArea;
  }

  doc["state_topic"] = state_topic;
  doc["availability_topic"] = mqttClient.topic_availability;

  mqttClient.beginPublish(topic, measureJson(doc), true);
  BufferingPrint bufferedClient(mqttClient, 32);
  serializeJson(doc, bufferedClient);
  bufferedClient.flush();
  mqttClient.endPublish();
}


/**
 * Publishes the IP Address to MQTT
 */
void mqtt_publishIPAddress(){

  char* state_topic = new char[MQTT_TOPIC_IP_ADDRESS_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, MQTT_TOPIC_IP_ADDRESS_STATE_PATTERN_LENGTH+1, MQTT_TOPIC_IP_ADDRESS_STATE_PATTERN, externalEEPROM.data.uuid);

  mqttClient.publish(state_topic, ETH.localIP().toString().c_str(), true);
}


/**
 * Handles count errors sensor auto discovery broadcasts
 */
void mqtt_autoDiscovery_count_errors(){

  DynamicJsonDocument doc(1024);

  char* topic = new char[MQTT_TOPIC_COUNT_ERRORS_AUTO_DISCOVERY_LENGTH+1];
  snprintf(topic, MQTT_TOPIC_COUNT_ERRORS_AUTO_DISCOVERY_LENGTH+1, MQTT_TOPIC_COUNT_ERRORS_AUTO_DISCOVERY_PATTERN, mqttClient.autoDiscovery.homeAssistantRoot, externalEEPROM.data.uuid);

  char* unique_id = new char[MQTT_COUNT_ERRORS_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1];
  snprintf(unique_id, MQTT_COUNT_ERRORS_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1, MQTT_COUNT_ERRORS_AUTO_DISCOVERY_UNIQUE_ID_PATTERN, externalEEPROM.data.uuid);

  char* state_topic = new char[MQTT_TOPIC_COUNT_ERRORS_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, MQTT_TOPIC_COUNT_ERRORS_STATE_PATTERN_LENGTH+1, MQTT_TOPIC_COUNT_ERRORS_STATE_PATTERN, externalEEPROM.data.uuid);

  doc["name"] = F("Error Count");
  doc["unique_id"] = unique_id;
  doc["object_id"] = unique_id;
  doc["icon"] = F("mdi:alert");
  doc["entity_category"] = F("diagnostic");

  JsonObject device = doc.createNestedObject("device");
  JsonArray identifiers = device.createNestedArray(F("identifiers"));
  identifiers.add(externalEEPROM.data.uuid);

  if(strlen(mqttClient.autoDiscovery.deviceName) > 0){
    device["name"] =  mqttClient.autoDiscovery.deviceName;
  }

  device["manufacturer"] = HARDWARE_MANUFACTURER_NAME;
  device["model"] = APPLICATION_NAME;
  device["model_id"] = externalEEPROM.data.product_id;
  device["serial_number"] = externalEEPROM.data.uuid;
  device["sw_version"] = VERSION;

  if(strlen(mqttClient.autoDiscovery.suggestedArea) > 0){
        device["suggested_area"] =  mqttClient.autoDiscovery.suggestedArea;
  }

  doc["state_topic"] = state_topic;
  doc["availability_topic"] = mqttClient.topic_availability;

  mqttClient.beginPublish(topic, measureJson(doc), true);
  BufferingPrint bufferedClient(mqttClient, 32);
  serializeJson(doc, bufferedClient);
  bufferedClient.flush();
  mqttClient.endPublish();
}


/**
 * Publishes the Error Count to MQTT
 */
void mqtt_publishCountErrors(){

  char* state_topic = new char[MQTT_TOPIC_COUNT_ERRORS_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, MQTT_TOPIC_COUNT_ERRORS_STATE_PATTERN_LENGTH+1, MQTT_TOPIC_COUNT_ERRORS_STATE_PATTERN, externalEEPROM.data.uuid);

  char* count = new char[3];
  snprintf(count, 3, "%i", eventLog.getErrors()->size());

  mqttClient.publish(state_topic, count, true);
}


/**
 * Handles update auto discovery broadcasts
 */
void mqtt_autoDiscovery_update(){
  
  char* topic = new char[MQTT_TOPIC_UPDATE_AUTO_DISCOVERY_PATTERN_LENGTH+1];
  snprintf(topic, MQTT_TOPIC_UPDATE_AUTO_DISCOVERY_PATTERN_LENGTH+1, MQTT_TOPIC_UPDATE_AUTO_DISCOVERY_PATTERN, mqttClient.autoDiscovery.homeAssistantRoot, externalEEPROM.data.uuid);

  char* unique_id = new char[MQTT_UPDATE_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1];
  snprintf(unique_id, MQTT_UPDATE_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1, MQTT_UPDATE_AUTO_DISCOVERY_UNIQUE_ID_PATTERN, externalEEPROM.data.uuid);

  char* state_topic = new char[MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH+1, MQTT_TOPIC_UPDATE_STATE_PATTERN, externalEEPROM.data.uuid);

  char* command_topic = new char[MQTT_TOPIC_UPDATE_SET_PATTERN_LENGTH+1];
  snprintf(command_topic, MQTT_TOPIC_UPDATE_SET_PATTERN_LENGTH+1, MQTT_TOPIC_UPDATE_SET_PATTERN, externalEEPROM.data.uuid);

  char* availability_topic = new char[MQTT_TOPIC_UPDATE_AVAILABILITY_LENGTH+1];
  snprintf(availability_topic, MQTT_TOPIC_UPDATE_AVAILABILITY_LENGTH+1, MQTT_TOPIC_UPDATE_AVAILABILITY_PATTERN, externalEEPROM.data.uuid);

  DynamicJsonDocument doc(1024);

  doc["name"] = F("Firmware");
  doc["unique_id"] = unique_id;
  doc["object_id"] = unique_id;
  doc["icon"] = F("mdi:update");

  JsonObject device = doc.createNestedObject("device");
  JsonArray identifiers = device.createNestedArray(F("identifiers"));
  identifiers.add(externalEEPROM.data.uuid);

  if(strlen(mqttClient.autoDiscovery.deviceName) > 0){
    device["name"] =  mqttClient.autoDiscovery.deviceName;
  }

  device["manufacturer"] = HARDWARE_MANUFACTURER_NAME;
  device["model"] = APPLICATION_NAME;
  device["model_id"] = externalEEPROM.data.product_id;
  device["serial_number"] = externalEEPROM.data.uuid;
  device["sw_version"] = VERSION;

  if(strlen(mqttClient.autoDiscovery.suggestedArea) > 0){
        device["suggested_area"] =  mqttClient.autoDiscovery.suggestedArea;
  }

  JsonArray availability = doc.createNestedArray(F("availability")); //Note, this is different from others
  JsonObject update_specific = availability.createNestedObject();
  JsonObject controller_level = availability.createNestedObject();
  update_specific["topic"] = availability_topic;
  controller_level["topic"] = mqttClient.topic_availability;
  doc["availability_mode"] = F("all");

  doc["state_topic"] = state_topic;
  doc["command_topic"] = command_topic;
  doc["payload_install"] = F("do-update");

  mqttClient.beginPublish(topic, measureJson(doc), true);
  BufferingPrint bufferedClient(mqttClient, 32);
  serializeJson(doc, bufferedClient);
  bufferedClient.flush();
  mqttClient.endPublish();

  mqttClient.addSubscription(command_topic);
}


/**
 * Handles broadcasting the update available to MQTT
 */
void mqtt_publishUpdateAvailable(JsonVariant &updateDoc){

  DynamicJsonDocument mqttDoc(256);

  mqttDoc["installed_version"] = VERSION;
  mqttDoc["latest_version"] = updateDoc["version"].as<const char*>();

  if(updateDoc.containsKey(F("title"))){
    mqttDoc["title"] = updateDoc["title"].as<const char*>();
  }

  if(updateDoc.containsKey(F("release_summary"))){
    mqttDoc["release_summary"] = updateDoc["release_summary"].as<const char*>();
  }

  if(updateDoc.containsKey(F("release_url"))){
    mqttDoc["release_url"] = updateDoc["release_url"].as<const char*>();
  }

  mqttDoc["in_progress"] = false;
  mqttDoc["update_percentage"] = NULL;

  char* topic = new char[MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH+1];
  snprintf(topic, MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH+1, MQTT_TOPIC_UPDATE_STATE_PATTERN, externalEEPROM.data.uuid);

  mqttClient.beginPublish(topic, measureJson(mqttDoc), false); //Don't retain
  BufferingPrint bufferedClient(mqttClient, 32);
  serializeJson(mqttDoc, bufferedClient);
  bufferedClient.flush();
  mqttClient.endPublish();
}


/**
 * Updates the update service availability.  If the service is available but no update is found, an MQTT event is published with the current version
 */
void mqtt_publishUpdateServiceAvailability(exEsp32FOTA::lastHTTPCheckStatus status){

  char* availability_topic = new char[MQTT_TOPIC_UPDATE_AVAILABILITY_LENGTH+1];
  snprintf(availability_topic, MQTT_TOPIC_UPDATE_AVAILABILITY_LENGTH+1, MQTT_TOPIC_UPDATE_AVAILABILITY_PATTERN, externalEEPROM.data.uuid);

  if(status == esp32FOTA::lastHTTPCheckStatus::SUCCESS || status == esp32FOTA::lastHTTPCheckStatus::SUCCESS_NO_UPDATE_AVAILABLE){
    mqttClient.publish(availability_topic, "online");
  }else{
    mqttClient.publish(availability_topic, "offline");
  }

  if(status == esp32FOTA::lastHTTPCheckStatus::SUCCESS_NO_UPDATE_AVAILABLE){
    DynamicJsonDocument mqttDoc(256);

    mqttDoc["installed_version"] = VERSION;
    mqttDoc["latest_version"] = VERSION;

    char* topic = new char[MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH+1];
    snprintf(topic, MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH+1, MQTT_TOPIC_UPDATE_STATE_PATTERN, externalEEPROM.data.uuid);

    mqttClient.beginPublish(topic, measureJson(mqttDoc), false); //Don't retain
    BufferingPrint bufferedClient(mqttClient, 32);
    serializeJson(mqttDoc, bufferedClient);
    bufferedClient.flush();
    mqttClient.endPublish();
  }
}


/**
 * Broadcasts an MQTT message that an output value has changed
 * @param id as the output's unique ID
 * @param value the new value that should be sent to any subscribers
 */
void mqtt_publishOutputValueChanged(char* id, uint8_t value){

    char* state_topic = new char[MQTT_TOPIC_OUTPUT_STATE_LENGTH+1];
    snprintf(state_topic, MQTT_TOPIC_OUTPUT_STATE_LENGTH+1, MQTT_TOPIC_OUTPUT_STATE_PATTERN, id);

    char* value_char = new char[4];
    snprintf(value_char, 4, "%i", value);

    mqttClient.publish(state_topic, value_char, true);
}