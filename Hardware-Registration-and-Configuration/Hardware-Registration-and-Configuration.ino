/**
 * Hardware Registration and Configuration.ino
 * 
 * Application allows for the reading of the external EEPROM, as well as programming of the external EEPROM data.  This is
 * separated from the main program to ensure the EEPROM can't be accidentally configured and lose the device information.
 * 
 * Exposes a set of API's that are described in the swagger.yaml file.
 * 
 * (C) 2024, P5 Software, LLC
*/

#define VERSION "2024.4.1"
#define APPLICATION_NAME "HW Reg and Config"

#include "common/hardware.h"
#include "common/externalEEPROM.h"
#include "common/oled.h"
#include "common/frontPanel.h"
#include "common/inputs.h"
#include "common/temperature.h"
#include "common/outputs.h"
#include "common/eventLog.h"
#include "common/authorizationToken.h"
#include <ArduinoJson.h>
#include "AsyncJson.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

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

  /* Startup the OLED display */
  oled.setCallback_failure(&failureHandler_oled);
  oled.begin();
  oled.setEventLog(&eventLog);
  oled.setAuthorizationToken(&authToken);
  authToken.setCallback_visualTokenChanged(&eventHandler_visualAuthChanged);

  /* Set event log callbacks to the OLED */
  eventLog.setCallback_notification(eventHandler_eventLogNotificationEvent);
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
  httpServer.addHandler(new AsyncCallbackJsonWebHandler("/api/eeprom", http_handleEEPROM_POST));
  httpServer.on("/api/eeprom", http_handleEEPROM);
  httpServer.on("/api/mcu", http_handleMCU);
  httpServer.on("/api/partitions", http_handlePartitions);
  httpServer.on("/api/peripherals", http_handlePeripherals);
  httpServer.on("/api/version", http_handleVersion);
  httpServer.on("/api/events", http_handleEventLog);
  httpServer.on("/api/errors", http_handleErrorLog);
  httpServer.on("^\/api\/network\/([a-z_]+)$", http_handleNetworkInterface);
  httpServer.on("/api/network", http_handleNetworkInterfaceAll);
  httpServer.on("/auth", http_handleAuth);

  if(wwwFS_isMounted){
    httpServer.serveStatic("/", wwwFS, "/");
    httpServer.rewrite("/ui/version", "/version.json");
    httpServer.rewrite("/", "/index.html");
  }

  if(configFS_isMounted){
    httpServer.on("^\/certs\/([a-z0-9_.]+)$", http_handleCert);
    httpServer.on("^/certs$", ASYNC_HTTP_ANY, http_handleCerts, http_handleCerts_Upload);
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


  oled.setPage(managerOled::PAGE_EVENT_LOG);

  otaFirmware.setManifestURL("http://192.168.10.101:8080/firmware.json");
  otaFirmware.setCertFileSystem(nullptr);
  otaFirmware.setSPIFFsPartitionLabel("www");

  if(strcmp(externalEEPROM.data.uuid, "") != 0){
    otaFirmware.setExtraHTTPHeader(F("uuid"), externalEEPROM.data.uuid);
    otaFirmware.setExtraHTTPHeader(F("product_id"), externalEEPROM.data.product_id);
  }

}


/**
 * Main loop
*/
void loop() {

  #if WIFI_MODEL != ENUM_WIFI_MODEL_ESP32 //Ignore when in SoftAP mode
    updateNTPTime();
  #endif

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
  doc["error"] = message;

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
 * Handles certificate uploads.  If the certificate already exists, a 403 is returned to the client
*/
void http_handleCerts_Upload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){

  if(!request->hasHeader("x-visual-token")){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header("x-visual-token").c_str())){
    http_unauthorized(request);
    return;
  }

  if(request->method()!= ASYNC_HTTP_POST){
    http_methodNotAllowed(request);
    return;
  }

  if(!index){

    if(configFS.exists(CONFIGFS_PATH_CERTS + filename)){
      http_forbiddenRequest(request, F("Certificate already exists"));
      return;
    }

    request->_tempFile = configFS.open(CONFIGFS_PATH_CERTS + filename, "w");
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

        if(!request->hasHeader("x-visual-token")){
        http_unauthorized(request);
        return;
      }

      if(!authToken.authenticate(request->header("x-visual-token").c_str())){
        http_unauthorized(request);
        return;
      }

      http_handleCert_GET(request);
      break;

    case ASYNC_HTTP_DELETE:

      if(!request->hasHeader("x-visual-token")){
        http_unauthorized(request);
        return;
      }

      if(!authToken.authenticate(request->header("x-visual-token").c_str())){
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

  if(configFS.exists(CONFIGFS_PATH_CERTS + request->pathArg(0))){

    AsyncWebServerResponse *response = request->beginResponse(configFS, CONFIGFS_PATH_CERTS + request->pathArg(0), "text/plain");
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

  if(configFS.exists(CONFIGFS_PATH_CERTS + request->pathArg(0))){
    configFS.remove(CONFIGFS_PATH_CERTS + request->pathArg(0));
    request->send(204);
  }else{
    request->send(404);
  }
}


/**
 * Handles partitions requests for the internal EEPROM
*/
void http_handlePartitions(AsyncWebServerRequest *request){

  if(request->method() != ASYNC_HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  esp_partition_iterator_t pi = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);

  if (pi == NULL) {
    http_error(request, F("esp_partition_find returned NULL"));
    return;
  }

  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  StaticJsonDocument<768> doc;
  JsonArray array = doc.to<JsonArray>();

  do {
    const esp_partition_t* p = esp_partition_get(pi);
    JsonObject jsonPartition = array.createNestedObject();
    
    jsonPartition[F("type")] = p->type;
    jsonPartition[F("subtype")] = p->subtype;
    jsonPartition[F("address")] = p->address;
    jsonPartition[F("size")] = p->size;
    jsonPartition[F("label")] = p->label;

  } while (pi = (esp_partition_next(pi)));

  esp_partition_iterator_release(pi);

  serializeJson(doc, *response);
  request->send(response);
    
}


/**
 * Handles the version endpoint requests
*/
void http_handleVersion(AsyncWebServerRequest *request){

  if(request->method() != ASYNC_HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  StaticJsonDocument<96> doc;
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

  if(request->method() != ASYNC_HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  char address[5] = {0};
  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  StaticJsonDocument<1536> doc;
  JsonArray array = doc.to<JsonArray>();

  managerInputs::healthResult inputHealth = inputs.health();
  nsOutputs::managerOutputs::healthResult outputHealth = outputs.health();
  managerTemperatureSensors::healthResult temperatureHealth = temperatureSensors.health();
  structHealth oledHealth = oled.health();
  structHealth externalEepromHealth = externalEEPROM.health();


  for(int i=0; i < inputHealth.count; i++){
    JsonObject inputObj = array.createNestedObject();
    sprintf(address, "0x%02X", inputHealth.inputControllers[i].address);
    inputObj[F("address")] = address;
    inputObj[F("type")] = F("INPUT");
    inputObj[F("online")] = inputHealth.inputControllers[i].enabled;
  }

  for(int i=0; i < outputHealth.count; i++){
    JsonObject outputObj = array.createNestedObject();
    sprintf(address, "0x%02X", outputHealth.outputControllers[i].address);
    outputObj[F("address")] = address;
    outputObj[F("type")] = F("OUTPUT");
    outputObj[F("online")] = outputHealth.outputControllers[i].enabled;
  }

  for(int i=0; i < temperatureHealth.count; i++){
    JsonObject tempObj = array.createNestedObject();
    sprintf(address, "0x%02X", temperatureHealth.sensor[i].address);
    tempObj[F("address")] = address;
    tempObj[F("type")] = F("TEMPERATURE");
    tempObj[F("online")] = temperatureHealth.sensor[i].enabled;
  }

  JsonObject oledObj = array.createNestedObject();
  sprintf(address, "0x%02X", oledHealth.address);
  oledObj[F("address")] = address;
  oledObj[F("type")] = F("OLED");
  oledObj[F("online")] = oledHealth.enabled;

  JsonObject externalEepromObj = array.createNestedObject();
  sprintf(address, "0x%02X", externalEepromHealth.address);
  externalEepromObj[F("address")] = address;
  externalEepromObj[F("type")] = F("EEPROM");
  externalEepromObj[F("online")] = externalEepromHealth.enabled;

  serializeJson(doc, *response);
  request->send(response);
}


/**
 * Handles requests for the main control unit
*/
void http_handleMCU(AsyncWebServerRequest *request){

  if(request->method() != ASYNC_HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  StaticJsonDocument<128> doc;
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
 * Handles network interface requests for the specified interface based on the request path
*/
void http_handleNetworkInterface(AsyncWebServerRequest *request){

  if(request->method() != ASYNC_HTTP_GET){
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

  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  StaticJsonDocument<96> doc;

  doc["interface"] = request->pathArg(0);
  doc["mac_address"] = macAddress;
  
  serializeJson(doc, *response);
  request->send(response);
}


/**
 * Handles network interface requests for all network interfaces
*/
void http_handleNetworkInterfaceAll(AsyncWebServerRequest *request){

  if(request->method() != ASYNC_HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  StaticJsonDocument<384> doc;
  JsonArray array = doc.to<JsonArray>();
  JsonObject objBT = array.createNestedObject();
  JsonObject objEth = array.createNestedObject();
  JsonObject objWifi = array.createNestedObject();
  JsonObject objAP = array.createNestedObject();

  char macAddress[18] = {0};

  getMacAddress(ESP_MAC_BT, macAddress);
  objBT[F("mac_address")] = macAddress;
  objBT[F("interface")] = F("bluetooth");
  
  getMacAddress(ESP_MAC_ETH, macAddress);
  objEth[F("mac_address")] = macAddress;
  objEth[F("interface")] = F("ethernet");
  
  getMacAddress(ESP_MAC_WIFI_STA, macAddress);
  objWifi[F("mac_address")] = macAddress;
  objWifi[F("interface")] = F("wifi");

  getMacAddress(ESP_MAC_WIFI_SOFTAP, macAddress);
  objAP[F("mac_address")] = macAddress;
  objAP[F("interface")] = F("wifi_ap");
  
  serializeJson(doc, *response);
  request->send(response);
}


/**
 * Handles all requests for the external EEPROM
*/
void http_handleEEPROM(AsyncWebServerRequest *request){

  switch(request->method()){

    case ASYNC_HTTP_OPTIONS:
      http_options(request);
      break;

    case ASYNC_HTTP_POST:

      if(!request->hasHeader("x-visual-token")){
        http_unauthorized(request);
        return;
      }

      if(!authToken.authenticate(request->header("x-visual-token").c_str())){
        http_unauthorized(request);
        return;
      }

      http_badRequest(request, F("Request requires a body")); //If there is no body in the request, it lands here
      break;

    case ASYNC_HTTP_GET:
      http_handleEEPROM_GET(request);
      break;

    case ASYNC_HTTP_DELETE:

      if(!request->hasHeader("x-visual-token")){
        http_unauthorized(request);
        return;
      }

      if(!authToken.authenticate(request->header("x-visual-token").c_str())){
        http_unauthorized(request);
        return;
      }

      http_handleEEPROM_DELETE(request);
      break;

    default:
      http_methodNotAllowed(request);
      break;
  }
}


/**
 * Handles GET requests for the external EEPROM
*/
void http_handleEEPROM_GET(AsyncWebServerRequest *request){

  if(externalEEPROM.enabled == false){
    http_error(request, F("Cannot connect to external EEPROM"));
    return;
  }

  if(strcmp(externalEEPROM.data.uuid, "") == 0){
    http_notFound(request);
    return;
  }
 
  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  StaticJsonDocument<256> doc;
  doc["uuid"] = externalEEPROM.data.uuid;
  doc["product_id"] = externalEEPROM.data.product_id;
  doc["key"] = externalEEPROM.data.key;

  serializeJson(doc, *response);
  request->send(response);
}


/**
 * Handles DELETE requests for the external EEPROM, which destroys its data.
 * The response is synchronous to the operation completing
*/
void http_handleEEPROM_DELETE(AsyncWebServerRequest *request){

  if (externalEEPROM.enabled == false){
    http_error(request, F("Cannot connect to external EEPROM"));
    return;
  }

  if(strcmp(externalEEPROM.data.uuid, "") == 0){
    http_notFound(request);
    return;
  }

  if(externalEEPROM.destroy() == false){
    http_error(request, F("Error during EEPROM delete"));
    return;
  }

  oled.setProductID(NULL);
  oled.setUUID(NULL);

  request->send(204);

  eventLog.createEvent(F("Deleted EEPROM"), EventLog::LOG_LEVEL_NOTIFICATION);
}


/**
 * Handles POST requests for the external EEPROM, which writes data to the EEPROM.
 * The response is synchronous to the operation completing
*/
void http_handleEEPROM_POST(AsyncWebServerRequest *request, JsonVariant doc){

    if(!request->hasHeader("x-visual-token")){
      http_unauthorized(request);
      return;
    }

    if(!authToken.authenticate(request->header("x-visual-token").c_str(), true)){
      http_unauthorized(request);
      return;
    }

  if (externalEEPROM.enabled == false){
    http_error(request, F("Cannot connect to external EEPROM"));
    return;
  }

  if(strcmp(externalEEPROM.data.uuid, "") != 0){
    http_badRequest(request, F("EEPROM already configured"));
    return;
  }

  MatchState ms;

  if(!doc.containsKey(F("uuid"))){
    http_badRequest(request, F("Field uuid is required"));
    return;
  }

  if(strlen(doc["uuid"])!=(sizeof(externalEEPROM.data.uuid)-1)){
    http_badRequest(request, F("Field uuid is not exactly 36 characters"));
    return;
  }

  managerExternalEEPROM::deviceType postedData;

  strcpy(postedData.uuid, doc["uuid"]);

  ms.Target(postedData.uuid);

  if(ms.MatchCount("^[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+$")!=1){ //Library does not support lengths of each section, so there is some opportunity for error
    http_badRequest(request, F("Invalid uuid, see docs"));
    return;
  }

  if(!doc.containsKey("product_id")){
    http_badRequest(request, F("Field product_id is required"));
    return;
  }

  if(strlen(doc["product_id"])>(sizeof(postedData.product_id)-1)){
    http_badRequest(request, F("Field product_id is greater than 32 characters, see docs"));
    return;
  }

  strcpy(postedData.product_id, doc["product_id"]);

  if(!doc.containsKey(F("key"))){
    http_badRequest(request, F("Field key is required"));
    return;
  }

  if(strlen(doc["key"])!=(sizeof(postedData.key)-1)){
    http_badRequest(request, F("Field key is not exactly 64 characters"));
    return;
  }

  strcpy(postedData.key, doc["key"]);

  ms.Target(postedData.key);

  if(ms.MatchCount("^[0-9A-Za-z]+$")!=1){
    http_badRequest(request, F("Invalid key, see docs"));
    return;
  }

  externalEEPROM.data = postedData;

  if(externalEEPROM.write() == false){
    http_error(request, "Error during EEPROM write");
    return;
  }

  oled.setProductID(externalEEPROM.data.product_id);
  oled.setUUID(externalEEPROM.data.uuid);

  request->send(201);

  eventLog.createEvent(F("Wrote EEPROM"), EventLog::LOG_LEVEL_NOTIFICATION);
}


/** 
 * Handle http requests for the event log
*/
void http_handleEventLog(AsyncWebServerRequest *request){

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
 * Retrieves a list of certificates
*/
void http_handleCerts_GET(AsyncWebServerRequest *request){

  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  StaticJsonDocument<768> doc;
  JsonArray array = doc.to<JsonArray>();

  File root = configFS.open("/certs");

  File file = root.openNextFile();
  while(file){
      if(!file.isDirectory()){
          JsonObject fileInstance = array.createNestedObject();
          fileInstance[F("file")] = file.name();
          fileInstance[F("size")] = file.size();
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
  if(request->method() != ASYNC_HTTP_POST){
    http_methodNotAllowed(request);
    return;
  }

  if(!request->hasHeader("x-visual-token")){
    http_unauthorized(request);
    return;
  }

  if(!authToken.authenticate(request->header("x-visual-token").c_str(), true)){
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