/**
 * Hardware Registration and Configuration.ino
 * 
 * Application allows for the reading of the device identity, as well as programming of the device identity data.  This is
 * separated from the main program to ensure the device identity can't be accidentally configured and lose the device information.
 * 
 * Exposes a set of API's that are described in the swagger.yaml file.
 * 
 * (C) 2024, P5 Software, LLC
*/

#if CORE_DEBUG_LEVEL == 0
  #ifndef VERSION
    #error "VERSION must be specified for a production build."
  #endif
  #ifndef COMMIT_HASH
    #error "COMMIT_HASH must be specified for a production build."
  #endif
#else
  #ifdef VERSION
    #error "VERSION may not be specified for a debug build."
  #else
    #define VERSION "9999.99.99"
  #endif
  #ifdef COMMIT_HASH
    #error "COMMIT_HASH may not be specified for a debug build."
  #else
    #define COMMIT_HASH "DEBUG"
  #endif
#endif
#define APPLICATION_NAME "HW Reg and Config"

#include "common/hardware.h"
#include "common/deviceIdentity.h"
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

unsigned long bootTime = 0; /* Approximate Epoch time the device booted */
AsyncWebServer httpServer(80);
managerDeviceIdentity deviceIdentity; /* Device identity instance */
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
void refreshCertBundle();

exEsp32FOTA otaFirmware(APPLICATION_NAME, VERSION, false); /* OTA firmware update class */

fs::LittleFSFS wwwFS;
fs::LittleFSFS configFS;
bool wwwFS_isMounted = false;
bool configFS_isMounted = false;

char* _certBundle = nullptr;            /* PSRAM-backed concatenated PEM bundle for OTA TLS */
size_t _certBundleSize = 0;             /* Byte length of _certBundle, excluding null terminator */
CryptoMemAsset* _certBundleAsset = nullptr; /* Asset pointer into _certBundle for esp32FOTA */

#define CONFIGFS_PATH_CERTS "/certs"


/**
 * One-time setup
*/
void setup() {


  eventLog.createEvent("Event log started");

  Wire.begin();

  /* Start the auth token service */
  authToken.begin();

  /* Startup the OLED display */
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
  frontPanel.setCallback_publisher(&frontPanelButtonPress);
  frontPanel.begin();
  frontPanel.setStatus(managerFrontPanel::status::NORMAL);

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
    ETH.begin(ETH_PHY_W5500, 1, ETHERNET_PIN, ETHERNET_PIN_INTERRUPT, ETHERNET_PIN_RESET,
              ETH_SPI_HOST, SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SPI_CLOCK_MHZ);

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


  /* Start device identity (NVS) */
  deviceIdentity.begin();

  if(deviceIdentity.enabled == true){

    oled.setProductID(deviceIdentity.data.product_id);
    oled.setUUID(deviceIdentity.data.uuid);

    log_i("NVS UUID: %s", deviceIdentity.data.uuid);
    log_i("NVS Product ID: %s", deviceIdentity.data.product_id);
    log_i("NVS Key: %s", deviceIdentity.data.key);
  }


  /* Start inputs */
  inputs.setCallback_failure(&failureHandler_inputs);
  inputs.begin();


  /* Start outputs */
  outputs.setCallback_failure(&failureHandler_outputs);
  outputs.begin();


  /* Start temperature sensors */
  temperatureSensors.setCallback_failure(&failureHandler_temperatureSensors);
  temperatureSensors.begin();


  /* Start LittleFS for www */
  if (wwwFS.begin(false, "/wwwFS", (uint8_t)10U, "www"))
  {
    wwwFS_isMounted = true;
    otaFirmware.setSPIFFsPartitionLabel("www");
    otaFirmware.setCertFileSystem(nullptr);
  }
  else{
    eventLog.createEvent("wwwFS mount fail", EventLog::LOG_LEVEL_ERROR);
    log_e("An Error has occurred while mounting www");
  }


    /* Start LittleFS for config */
  if (configFS.begin(false, "/configFS", (uint8_t)10U, "config"))
  {
    configFS_isMounted = true;
  }
  else{
    eventLog.createEvent("configFS mount fail", EventLog::LOG_LEVEL_ERROR);
    log_e("An Error has occurred while mounting configFS");
  }

  refreshCertBundle();

  /* Configure the web server.  
    IMPORTANT: *** Sequence below matters, they are sorted specific to generic *** 
  */
  httpServer.addHandler(new AsyncCallbackJsonWebHandler("/api/identity", http_handleIdentity_POST));
  httpServer.on("/api/identity", http_handleIdentity);
  httpServer.on("^/api/mcu$", http_handleMCU);
  httpServer.on("^/api/mcu/reboot$", http_handleReboot);
  httpServer.on("/api/partitions", http_handlePartitions);
  httpServer.on("/api/peripherals", http_handlePeripherals);
  httpServer.on("/api/version", http_handleVersion);
  httpServer.on("/api/events", http_handleEventLog);
  httpServer.on("/api/errors", http_handleErrorLog);
  httpServer.on("^/api/network/([a-z_]+)$", http_handleNetworkInterface);
  httpServer.on("/api/network", http_handleNetworkInterfaceAll);
  httpServer.on("/auth", http_handleAuth);

  if(configFS_isMounted){
    httpServer.on("^/certs/([a-z0-9_.]+)$", http_handleCert);
    httpServer.on("^/certs$", HTTP_ANY, http_handleCerts, http_handleCerts_Upload);
    httpServer.addHandler(new AsyncCallbackJsonWebHandler("/api/ota/app", http_handleOTA_forced));
    httpServer.addHandler(new AsyncCallbackJsonWebHandler("/api/ota/spiffs", http_handleOTA_forced));
  }else{
    log_e("configFS is not mounted");
    httpServer.on("^/certs/([a-z0-9_.]+)$", http_configFSNotMunted);
    httpServer.on("^/certs$", HTTP_ANY, http_configFSNotMunted);
    httpServer.on("^/api/ota/.+$", http_configFSNotMunted);
  }

  if(wwwFS_isMounted){
    httpServer.serveStatic("/", wwwFS, "/", "public, max-age=86400");
    httpServer.rewrite("/ui/version", "/version.json");
    httpServer.rewrite("/", "/index.html");
  }

  httpServer.onNotFound(http_notFound);

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*"); //Ignore CORS
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "visual-token, Content-Type"); //Ignore CORS
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS, DELETE"); //Ignore CORS

  #if WIFI_MODEL == ENUM_WIFI_MODEL_ESP32

    if(WiFi.getMode() == wifi_mode_t::WIFI_MODE_NULL){

      log_w("HTTP server will not be started because WiFi it has not been initialized (WIFI_MODE_NULL)");
    
    }else{
      httpServer.begin();
      eventLog.createEvent("Web server started");

      log_i("HTTP server ready");
    }
  #endif

  #if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_W5500

      httpServer.begin();
      eventLog.createEvent("Web server started");
      log_i("HTTP server ready");

  #endif

  oled.setPage(managerOled::PAGE_EVENT_LOG);

  otaFirmware.setProgressCb(eventHandler_otaFirmwareProgress);
  otaFirmware.setUpdateBeginFailCb(eventHandler_otaFirmwareFailed);
  otaFirmware.setUpdateFinishedCb(eventHandler_otaFirmwareFinished);
}


/**
 * Main loop
*/
void loop() {

  updateNTPTime();

  otaFirmware_checkPending();

  oled.loop();
  authToken.loop(); 
  frontPanel.loop();
}


/** 
 * Handles front panel button press events 
*/
void frontPanelButtonPress(){

  log_v("Front Panel button was pressed");

  oled.nextPage();
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

    bool updateSuccess = false;

    if(otaFirmware.pending[i].url.startsWith("https:")){
      if(_certBundleAsset != nullptr){
        otaFirmware.setRootCA(_certBundleAsset);
      } else {
        otaFirmware.useBundledCerts();
      }
    }

    switch(otaFirmware.pending[i].type){

      case OTA_UPDATE_APP:
        eventLog.createEvent("OTA app forced");
        updateSuccess = otaFirmware.forceUpdate(otaFirmware.pending[i].url.c_str(), false);
        break;


      case OTA_UPDATE_SPIFFS:
        eventLog.createEvent("OTA SPIFFS forced");
        updateSuccess = otaFirmware.forceUpdateSPIFFS(otaFirmware.pending[i].url.c_str(), false);
        break;
    }

    if(!updateSuccess){
      eventLog.createEvent("OTA update failed", EventLog::LOG_LEVEL_NOTIFICATION);
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

  if(progress == 0){
    eventLog.createEvent("OTA update started", EventLog::LOG_LEVEL_NOTIFICATION);
    oled.setPage(managerOled::PAGE_OTA_IN_PROGRESS);
  }

  oled.setProgressBar(((float)progress/(float)size));
}


/**
 * Callback function to report that the update has failed
 * @param partition inherited from esp32FOTA library
*/
void eventHandler_otaFirmwareFailed(int partition){
  log_i("Failed partition: [%i]", partition);
  eventLog.createEvent("OTA update failed", EventLog::LOG_LEVEL_NOTIFICATION);
}


/**
 * A callback function for when the firmware update has finished
*/
void eventHandler_otaFirmwareFinished(int partition, bool needs_restart){

  log_i("Finished Partition: [%i] needs restart: [%s]", partition, needs_restart ? "true":"false");

  eventLog.createEvent("OTA update finished", EventLog::LOG_LEVEL_NOTIFICATION);

  if(needs_restart){
      eventLog.createEvent("Rebooting...", EventLog::LOG_LEVEL_NOTIFICATION);
      delay(5000);
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

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc;
  doc["message"] = message;

  serializeJson(doc, *response);
  response->setCode(500);
  request->send(response);
}


/**
 * Sends a 400 response indicating an invalid request from the caller
*/
void http_badRequest(AsyncWebServerRequest *request, String message){

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc;
  doc["message"] = message;

  serializeJson(doc, *response);
  response->setCode(400);
  request->send(response);
}


/**
 * Sends a 403 response indicating an invalid request from the caller
*/
void http_forbiddenRequest(AsyncWebServerRequest *request, String message){

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc;
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

  if(!request->hasHeader("visual-token")){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header("visual-token").c_str())){
    http_unauthorized(request);
    return;
  }

  http_error(request, "file system not mounted");

}


/**
 * Handles /certs requests
*/
void http_handleCerts(AsyncWebServerRequest *request){

  switch(request->method()){

    case HTTP_OPTIONS:
      http_options(request);
      break;

    case HTTP_POST:

      break; //http_handleCerts_Upload handles all authorization and responses

    case HTTP_GET:

      if(!request->hasHeader("visual-token")){
        http_unauthorized(request);
        return;
      }

      if(!authToken.authenticate(request->header("visual-token").c_str())){
        http_unauthorized(request);
        return;
      }

      http_handleCerts_GET(request);
      break;

    default:
      http_methodNotAllowed(request);
      break;
  }

}


/**
 * Handles certificate uploads.  If the certificate already exists, a 403 is returned to the client
*/
void http_handleCerts_Upload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){

  if(!request->hasHeader("visual-token")){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header("visual-token").c_str())){
    http_unauthorized(request);
    return;
  }

  if(request->method()!= HTTP_POST){
    http_methodNotAllowed(request);
    return;
  }

  if(!index){

    if(!configFS.exists(CONFIGFS_PATH_CERTS)){
      configFS.mkdir(CONFIGFS_PATH_CERTS);
    }

    if(configFS.exists(CONFIGFS_PATH_CERTS + (String)"/" + filename)){
      http_forbiddenRequest(request, "Certificate already exists");
      return;
    }

    request->_tempFile = configFS.open(CONFIGFS_PATH_CERTS + (String)"/" + filename, "w");
  }

  request->_tempFile.write(data,len);
    
  if(final){
    request->_tempFile.close();
    refreshCertBundle();
    request->send(201);
  }
}


/**
 * Handles /certs/{file} requests
*/
void http_handleCert(AsyncWebServerRequest *request){
  switch(request->method()){

    case HTTP_OPTIONS:
      http_options(request);
      break;

    case HTTP_GET:

        if(!request->hasHeader("visual-token")){
        http_unauthorized(request);
        return;
      }

      if(!authToken.authenticate(request->header("visual-token").c_str())){
        http_unauthorized(request);
        return;
      }

      http_handleCert_GET(request);
      break;

    case HTTP_DELETE:

      if(!request->hasHeader("visual-token")){
        http_unauthorized(request);
        return;
      }

      if(!authToken.authenticate(request->header("visual-token").c_str())){
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

  if(configFS.exists(CONFIGFS_PATH_CERTS + (String)"/" + request->pathArg(0))){

    AsyncWebServerResponse *response = request->beginResponse(configFS, CONFIGFS_PATH_CERTS + (String)"/" + request->pathArg(0), "text/plain");
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

  if(configFS.exists(CONFIGFS_PATH_CERTS + (String)"/" + request->pathArg(0))){
    configFS.remove(CONFIGFS_PATH_CERTS + (String)"/" + request->pathArg(0));
    refreshCertBundle();
    request->send(204);
  }else{
    request->send(404);
  }
}


/**
 * Handles force OTA force update requests using the payload provided
*/
void http_handleOTA_forced(AsyncWebServerRequest *request, JsonVariant doc){

  if(!request->hasHeader("visual-token")){
      http_unauthorized(request);
      return;
  }

  if(!authToken.authenticate(request->header("visual-token").c_str(), true)){
    http_unauthorized(request);
    return;
  }

   if(doc["url"].isNull()){
    http_badRequest(request, "Field url is required");
    return;
  }

  forcedOtaUpdateConfig newFirmwareRequest;
  newFirmwareRequest.url = doc["url"].as<String>();

  if(!newFirmwareRequest.url.startsWith("http")){
    http_badRequest(request, "Bad url; http or https required");
    return;
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
 * Handles partitions requests for the internal EEPROM
*/
void http_handlePartitions(AsyncWebServerRequest *request){

  if(request->method() != HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  esp_partition_iterator_t partitionIterator = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);

  if (partitionIterator == NULL) {
    http_error(request, "esp_partition_find returned NULL");
    return;
  }

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();

  do {
    const esp_partition_t* p = esp_partition_get(partitionIterator);
    JsonObject jsonPartition = array.add<JsonObject>();
    
    jsonPartition["type"] = p->type;
    jsonPartition["subtype"] = p->subtype;
    jsonPartition["address"] = p->address;
    jsonPartition["size"] = p->size;
    jsonPartition["label"] = p->label;

  } while ((partitionIterator = esp_partition_next(partitionIterator)));

  esp_partition_iterator_release(partitionIterator);

  serializeJson(doc, *response);
  request->send(response);
    
}


/**
 * Handles the version endpoint requests
*/
void http_handleVersion(AsyncWebServerRequest *request){

  if(request->method() != HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc;
  doc["application"] = VERSION;
  char product_hex[16] = {0};
  sprintf(product_hex, "0x%08X", PRODUCT_HEX);
  doc["product_hex"] = product_hex;

  serializeJson(doc, *response);
  request->send(response);
}


/**
 * Handles peripheral requests for peripherals defined in hardware.h
*/
void http_handlePeripherals(AsyncWebServerRequest *request){

  if(request->method() != HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  char address[5] = {0};
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();

  managerInputs::healthResult inputHealth = inputs.health();
  nsOutputs::managerOutputs::healthResult outputHealth = outputs.health();
  managerTemperatureSensors::healthResult temperatureHealth = temperatureSensors.health();
  structHealth oledHealth = oled.health();


  for(int i=0; i < inputHealth.count; i++){
    JsonObject inputObj = array.add<JsonObject>();
    sprintf(address, "0x%02X", inputHealth.inputControllers[i].address);
    inputObj["address"] = address;
    inputObj["type"] = "INPUT";
    inputObj["online"] = inputHealth.inputControllers[i].enabled;
  }

  for(int i=0; i < outputHealth.count; i++){
    JsonObject outputObj = array.add<JsonObject>();
    sprintf(address, "0x%02X", outputHealth.outputControllers[i].address);
    outputObj["address"] = address;
    outputObj["type"] = "OUTPUT";
    outputObj["online"] = outputHealth.outputControllers[i].enabled;
  }

  for(int i=0; i < temperatureHealth.count; i++){
    JsonObject tempObj = array.add<JsonObject>();
    sprintf(address, "0x%02X", temperatureHealth.sensor[i].address);
    tempObj["address"] = address;
    tempObj["type"] = "TEMPERATURE";
    tempObj["online"] = temperatureHealth.sensor[i].enabled;
  }

  JsonObject oledObj = array.add<JsonObject>();
  sprintf(address, "0x%02X", oledHealth.address);
  oledObj["address"] = address;
  oledObj["type"] = "OLED";
  oledObj["online"] = oledHealth.enabled;

  JsonObject nvsObj = array.add<JsonObject>();
  nvsObj["address"] = "0x00";
  nvsObj["type"] = "NVS";
  nvsObj["online"] = true;

  serializeJson(doc, *response);
  request->send(response);
}


/**
 * Handles requests for the main control unit
*/
void http_handleMCU(AsyncWebServerRequest *request){

  if(request->method() != HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc;
  doc["chip_model"] = ESP.getChipModel();
  doc["revision"] = (String)ESP.getChipRevision();
  doc["flash_chip_size"] = ESP.getFlashChipSize();

  if(bootTime !=0){
      doc["boot_time"] = bootTime;
  }else{
    #ifdef ESP32
      doc["boot_time"] = esp_timer_get_time()/1000000;
    #else
      doc["boot_time"] = millis()/1000;
    #endif
  }

  serializeJson(doc, *response);
  request->send(response);
}


/**
 * Handles requests for the MCU to be rebooted remotely
*/
void http_handleReboot(AsyncWebServerRequest *request){

  if(!request->hasHeader("visual-token")){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header("visual-token").c_str())){
    http_unauthorized(request);
    return;
  }

  if(request->method()!= HTTP_POST){
    http_methodNotAllowed(request);
    return;
  }

  request->send(204);

  delay(500); // Allow the HTTP response to be transmitted before rebooting

  ESP.restart();

}


/**
 * Handles network interface requests for the specified interface based on the request path
*/
void http_handleNetworkInterface(AsyncWebServerRequest *request){

  if(request->method() != HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  esp_mac_type_t interface = ESP_MAC_IEEE802154; //Used for defaulting only; not supported by chip or API

  if(request->pathArg(0) == "bluetooth"){
    interface = ESP_MAC_BT;    
  }

  if(request->pathArg(0) == "ethernet"){
    interface = ESP_MAC_ETH;
  }

  if(request->pathArg(0) == "wifi"){
    interface = ESP_MAC_WIFI_STA;
  }

  if(request->pathArg(0) == "wifi_ap"){
    interface = ESP_MAC_WIFI_SOFTAP;
  }

  if(interface == ESP_MAC_IEEE802154){
    http_notFound(request);
    return;
  }

  char macAddress[18] = {0};
  getMacAddress(interface, macAddress);

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc;

  doc["interface"] = request->pathArg(0);
  doc["mac_address"] = macAddress;
  
  serializeJson(doc, *response);
  request->send(response);
}


/**
 * Handles network interface requests for all network interfaces
*/
void http_handleNetworkInterfaceAll(AsyncWebServerRequest *request){

  if(request->method() != HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();
  JsonObject objWifi = array.add<JsonObject>();
  JsonObject objAP = array.add<JsonObject>();
  JsonObject objBT = array.add<JsonObject>();
  JsonObject objEth = array.add<JsonObject>();

  char macAddress[18] = {0};

  getMacAddress(ESP_MAC_WIFI_STA, macAddress);
  objWifi["mac_address"] = macAddress;
  objWifi["interface"] = "wifi";

  getMacAddress(ESP_MAC_WIFI_SOFTAP, macAddress);
  objAP["mac_address"] = macAddress;
  objAP["interface"] = "wifi_ap";

  getMacAddress(ESP_MAC_BT, macAddress);
  objBT["mac_address"] = macAddress;
  objBT["interface"] = "bluetooth";
  
  getMacAddress(ESP_MAC_ETH, macAddress);
  objEth["mac_address"] = macAddress;
  objEth["interface"] = "ethernet";
  
  serializeJson(doc, *response);
  request->send(response);
}


/**
 * Handles all requests for device identity
*/
void http_handleIdentity(AsyncWebServerRequest *request){

  switch(request->method()){

    case HTTP_OPTIONS:
      http_options(request);
      break;

    case HTTP_POST:

      if(!request->hasHeader("visual-token")){
        http_unauthorized(request);
        return;
      }

      if(!authToken.authenticate(request->header("visual-token").c_str())){
        http_unauthorized(request);
        return;
      }

      http_badRequest(request, "Request requires a body"); //If there is no body in the request, it lands here
      break;

    case HTTP_GET:

      if(!request->hasHeader("visual-token")){
        http_unauthorized(request);
        return;
      }

      if(!authToken.authenticate(request->header("visual-token").c_str())){
        http_unauthorized(request);
        return;
      }

      http_handleIdentity_GET(request);
      break;

    case HTTP_DELETE:

      if(!request->hasHeader("visual-token")){
        http_unauthorized(request);
        return;
      }

      if(!authToken.authenticate(request->header("visual-token").c_str())){
        http_unauthorized(request);
        return;
      }

      http_handleIdentity_DELETE(request);
      break;

    default:
      http_methodNotAllowed(request);
      break;
  }
}


/**
 * Handles GET requests for device identity
*/
void http_handleIdentity_GET(AsyncWebServerRequest *request){

  if(strcmp(deviceIdentity.data.uuid, "") == 0){
    http_notFound(request);
    return;
  }
 
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc;
  doc["uuid"] = deviceIdentity.data.uuid;
  doc["product_id"] = deviceIdentity.data.product_id;
  doc["key"] = deviceIdentity.data.key;

  serializeJson(doc, *response);
  request->send(response);
}


/**
 * Handles DELETE requests for device identity, which destroys its data.
 * The response is synchronous to the operation completing
*/
void http_handleIdentity_DELETE(AsyncWebServerRequest *request){

  if(strcmp(deviceIdentity.data.uuid, "") == 0){
    http_notFound(request);
    return;
  }

  if(deviceIdentity.destroy() == false){
    http_error(request, "Error during NVS delete");
    return;
  }

  oled.setProductID(NULL);
  oled.setUUID(NULL);

  request->send(204);

  eventLog.createEvent("Deleted device identity", EventLog::LOG_LEVEL_NOTIFICATION);
}


/**
 * Handles POST requests for device identity, which writes the identity data to NVS.
 * The response is synchronous to the operation completing
*/
void http_handleIdentity_POST(AsyncWebServerRequest *request, JsonVariant doc){

    if(!request->hasHeader("visual-token")){
      http_unauthorized(request);
      return;
    }

    if(!authToken.authenticate(request->header("visual-token").c_str(), true)){
      http_unauthorized(request);
      return;
    }

  if(strcmp(deviceIdentity.data.uuid, "") != 0){
    http_badRequest(request, "Device already provisioned");
    return;
  }

  MatchState ms;

  if(doc["uuid"].isNull()){
    http_badRequest(request, "Field uuid is required");
    return;
  }

  if(strlen(doc["uuid"])!=(sizeof(deviceIdentity.data.uuid)-1)){
    http_badRequest(request, "Field uuid is not exactly 36 characters");
    return;
  }

  managerDeviceIdentity::deviceType postedData;

  strlcpy(postedData.uuid, doc["uuid"], sizeof(postedData.uuid));

  ms.Target(postedData.uuid);

  if(ms.MatchCount("^[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+$")!=1){ //Library does not support lengths of each section, so there is some opportunity for error
    http_badRequest(request, "Invalid uuid, see docs");
    return;
  }

  if(doc["product_id"].isNull()){
    http_badRequest(request, "Field product_id is required");
    return;
  }

  if(strlen(doc["product_id"])>(sizeof(postedData.product_id)-1)){
    http_badRequest(request, "Field product_id is greater than 32 characters, see docs");
    return;
  }

  strlcpy(postedData.product_id, doc["product_id"], sizeof(postedData.product_id));

  if(doc["key"].isNull()){
    http_badRequest(request, "Field key is required");
    return;
  }

  if(strlen(doc["key"])!=(sizeof(postedData.key)-1)){
    http_badRequest(request, "Field key is not exactly 64 characters");
    return;
  }

  strlcpy(postedData.key, doc["key"], sizeof(postedData.key));
  postedData.product_hex = PRODUCT_HEX;

  ms.Target(postedData.key);

  if(ms.MatchCount("^[0-9A-Za-z]+$")!=1){
    http_badRequest(request, "Invalid key, see docs");
    return;
  }

  deviceIdentity.data = postedData;

  if(deviceIdentity.write() == false){
    http_error(request, "Error during NVS write");
    return;
  }

  oled.setProductID(deviceIdentity.data.product_id);
  oled.setUUID(deviceIdentity.data.uuid);

  request->send(201);

  eventLog.createEvent("Wrote device identity", EventLog::LOG_LEVEL_NOTIFICATION);
}


/** 
 * Handle http requests for the event log
*/
void http_handleEventLog(AsyncWebServerRequest *request){

  if(request->method() != HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  if(eventLog.getEventCount() == 0){
    request->send(200, "application/json","[]");
    return;
  }

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc;

  for(uint16_t i=0; i < eventLog.getEventCount(); i++){
    JsonObject entry = doc.add<JsonObject>();
    entry["time"] = eventLog.getEvent(i).timestamp;

    switch(eventLog.getEvent(i).level){
      case EventLog::LOG_LEVEL_ERROR:
        entry["level"] = "error";
        break;

      case EventLog::LOG_LEVEL_NOTIFICATION:
        entry["level"] = "notify";
        break;

      case EventLog::LOG_LEVEL_INFO:
        entry["level"] = "info";
        break;

      default:
        entry["level"] = "unknown";
        break;
    }

    entry["text"] = eventLog.getEvent(i).text;
  }
 
  serializeJson(doc, *response);
  request->send(response);
}


/** 
 * Handle http requests for the error log
*/
void http_handleErrorLog(AsyncWebServerRequest *request){

  if(request->method() != HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  if(eventLog.getErrors()->size() == 0){
    request->send(200, "application/json","[]");
    return;
  }

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc;

  for(int i=0; i < eventLog.getErrors()->size(); i++){
    JsonObject entry = doc.add<JsonObject>();

    entry["text"] = eventLog.getErrors()->get(i);
  }

  serializeJson(doc, *response);
  request->send(response);
}


/** 
 * Retrieves a list of certificates
*/
void http_handleCerts_GET(AsyncWebServerRequest *request){

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();

  File root = configFS.open(CONFIGFS_PATH_CERTS);

  File file = root.openNextFile();
  while(file){
      if(!file.isDirectory()){
          JsonObject fileInstance = array.add<JsonObject>();
          fileInstance["file"] = (String)file.name();
          fileInstance["size"] = file.size();
      }
      file = root.openNextFile();
    }

  serializeJson(doc, *response);
  request->send(response);

}


/**
 * Creates a long-term authorization with the given visual token
*/
void http_handleAuth(AsyncWebServerRequest *request){
  if(request->method() != HTTP_POST){
    http_methodNotAllowed(request);
    return;
  }

  if(!request->hasHeader("visual-token")){
    http_unauthorized(request);
    return;
  }

  if(!authToken.authenticate(request->header("visual-token").c_str(), true)){
    http_unauthorized(request);
    return;
  }

  request->send(204);
}


/**
 * Retrieves the requested interface MAC address
*/
void getMacAddress(esp_mac_type_t type, char *buff) {
    uint8_t baseMac[6];
    esp_read_mac(baseMac, type);
    sprintf(buff, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
}


/** 
 * Callback function which handles failures of the OLED 
*/
void failureHandler_oled(uint8_t address, managerOled::failureReason failureReason){

  char text[OLED_CHARACTERS_PER_LINE+1];

  if(failureReason == managerOled::failureReason::ADDRESS_OFFLINE){
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "OLED 0x%02X offline", address);
  }else{
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "OLED 0x%02X fail %i", address, failureReason);
  }
  
  eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
  frontPanel.setStatus(managerFrontPanel::status::FAILURE);
}


/**
 * Callback function which handles failures of any input
*/
void failureHandler_inputs(uint8_t address, managerInputs::failureReason failureReason){

	char text[OLED_CHARACTERS_PER_LINE+1];

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

  char text[OLED_CHARACTERS_PER_LINE+1];

  if(failureReason == nsOutputs::failureReason::ADDRESS_OFFLINE){
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "Out ctl 0x%02X offline", address);
  }else{
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "Out ctl 0x%02X fail %i", address, failureReason);
  }
  
  eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
  frontPanel.setStatus(managerFrontPanel::status::FAILURE);
}


/** 
 * Callback function which handles failures of any temperature sensor 
*/
void failureHandler_temperatureSensors(uint8_t address, managerTemperatureSensors::failureReason failureReason){

  char text[OLED_CHARACTERS_PER_LINE+1];

  if(failureReason == managerTemperatureSensors::failureReason::ADDRESS_OFFLINE){
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "Temp sen 0x%02X offline", address);
  }else{
      snprintf(text, OLED_CHARACTERS_PER_LINE+1, "Temp sen 0x%02X fail %i", address, failureReason);
  }
  
  eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
  frontPanel.setStatus(managerFrontPanel::status::FAILURE);
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
 * Replace the auth token page with the event log so that the OLED will sleep when the visual token changes
*/
void eventHandler_visualAuthChanged(){
  if(oled.getPage() == managerOled::PAGE_AUTH_TOKEN){
    oled.setPage(managerOled::PAGE_EVENT_LOG);
  }
}


/**
 * Concatenates all user-uploaded certificates from configFS into a PSRAM-backed PEM bundle.
 * If no certificates are uploaded, _certBundleAsset remains nullptr, which signals callers
 * to use the ESP-IDF built-in Mozilla root CA bundle instead.
*/
void refreshCertBundle(){

  if(_certBundle != nullptr){ free(_certBundle); _certBundle = nullptr; }
  if(_certBundleAsset != nullptr){ delete _certBundleAsset; _certBundleAsset = nullptr; }
  _certBundleSize = 0;

  String bundle = "";

  if(configFS_isMounted){
    File certsDir = configFS.open(CONFIGFS_PATH_CERTS);
    if(certsDir && certsDir.isDirectory()){
      File certFile = certsDir.openNextFile();
      while(certFile){
        if(!certFile.isDirectory()){
          while(certFile.available()){ bundle += (char)certFile.read(); }
        }
        certFile = certsDir.openNextFile();
      }
    }
  }

  _certBundleSize = bundle.length();

  if(_certBundleSize == 0){
    log_i("No user certs found; will use bundled Mozilla root CAs");
    return;
  }

  _certBundle = (char*)ps_malloc(_certBundleSize + 1);
  if(_certBundle == nullptr){ _certBundle = (char*)malloc(_certBundleSize + 1); }
  if(_certBundle == nullptr){
    log_e("Failed to allocate cert bundle");
    _certBundleSize = 0;
    return;
  }

  memcpy(_certBundle, bundle.c_str(), _certBundleSize + 1);
  _certBundleAsset = new CryptoMemAsset("bundle", _certBundle, _certBundleSize);
  log_i("Cert bundle built: %u bytes", (unsigned int)_certBundleSize);
}


#if ETHERNET_MODEL != ENUM_ETHERNET_MODEL_NONE

  /** 
   * Handle Ethernet being connected
  */
  void eventHandler_ethernetConnect(){
    updateNTPTime(true);
    eventLog.createEvent("Ethernet connected");
    eventLog.resolveError("Ethernet disconnected");
  }


  /** 
   * Handle Ethernet being disconnected
  */
  void eventHandler_ethernetDisconnect(){
    eventLog.createEvent("Ethernet disconnected", EventLog::LOG_LEVEL_ERROR);
  }

#endif