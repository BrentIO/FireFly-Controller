/**
 * Hardware Registration and Configuration.ino
 * 
 * Application allows for the reading of the external EEPROM, as well as programming of the external EEPROM data.  This is
 * separated from the main program to ensure the EEPROM can't be accidentally configured and lose the device information.
 * 
 * Application creates a soft AP to connect to, and exposes a set of API's that are described in the swagger.yaml file.
 * 
 * (C) 2023, P5 Software, LLC
*/

#define DEBUG 2500
#define VERSION "2023.07.0001"


#include "common/hardware.h"
#include "common/externalEEPROM.h"
#include "common/oled.h"
#include "common/frontPanel.h"
#include "common/inputs.h"
#include "common/temperature.h"
#include "common/outputs.h"
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include <esp_chip_info.h> // https://github.com/espressif/arduino-esp32
#include <WiFi.h>
#include <AsyncTCP.h> // https://github.com/me-no-dev/AsyncTCP
#include <ESPAsyncWebServer.h> // https://github.com/me-no-dev/ESPAsyncWebServer
#include "AsyncJson.h"
#include <LittleFS.h>
#include <Regexp.h> // https://github.com/nickgammon/Regexp


AsyncWebServer httpServer(80);
managerExternalEEPROM externalEEPROM;
managerOled oled;
managerFrontPanel frontPanel;
managerInputs inputs;
managerOutputs outputs;
managerTemperatureSensors temperatureSensors;


void setup() {

  #ifdef DEBUG
    Serial.begin(115200);
  #endif

  Wire.begin();

  oled.setCallback_failure(&failureHandler_oled);
  oled.begin();

  frontPanel.setCallback_publisher(&frontPanelButtonPress);
  frontPanel.begin();
  frontPanel.setStatus(managerFrontPanel::status::NORMAL);

  //Setup a soft AP with the SSID FireFly-######, where the last 6 characters are the last 6 of the Soft AP MAC address
  uint8_t baseMac[6];
  esp_read_mac(baseMac, ESP_MAC_WIFI_SOFTAP);
  char apName[18] = {0};
  sprintf(apName, "FireFly-%02X%02X%02X", baseMac[3], baseMac[4], baseMac[5]);

  #if DEBUG > 1000
    Serial.print(F("[main] (setup) Starting SoftAP..."));
  #endif

  WiFi.softAP(apName);

  #if DEBUG > 1000
    Serial.println(F("Done"));
    Serial.println("[main] (setup) Started SoftAP " + WiFi.softAPSSID());
  #endif
 
  oled.setWiFiInfo(&WiFi);

  externalEEPROM.setCallback_failure(&failureHandler_eeprom);
  externalEEPROM.begin();

  oled.setProductID(externalEEPROM.data.product_id);
  oled.setUUID(externalEEPROM.data.uuid);

  #if DEBUG > 500

   if(externalEEPROM.enabled == true){
      Serial.println("uuid: " + String(externalEEPROM.data.uuid));
      Serial.println("product_id: " + String(externalEEPROM.data.product_id));
      Serial.println("key: " + String(externalEEPROM.data.key));
    }

  #endif

  inputs.setCallback_failure(&failureHandler_inputs);
  inputs.begin();

  outputs.setCallback_failure(&failureHandler_outputs);
  outputs.begin();

  temperatureSensors.setCallback_failure(&failureHandler_temperatureSensors);
  temperatureSensors.begin();

  // *** Sequence below matters -- specific to generic ***
  AsyncCallbackJsonWebHandler* eepromTest = new AsyncCallbackJsonWebHandler("/api/eeprom", http_handleEEPROM_POST);
  httpServer.addHandler(eepromTest);
  httpServer.on("/api/eeprom", http_handleEEPROM);
  httpServer.on("/api/mcu", http_handleMCU);
  httpServer.on("/api/partitions", http_handlePartitions);
  httpServer.on("/api/peripherals", http_handlePeripherals);
  httpServer.on("/api/version", http_handleVersion);
  httpServer.on("^\/api\/network\/([a-z_]+)$", http_handleNetworkInterface);
  httpServer.on("/api/network", http_handleAllNetworkInterfaces);

  if (!LittleFS.begin())
  {
    oled.logEvent("LittleFS Mount Fail", managerOled::LOG_LEVEL_ERROR);

    #ifdef DEBUG
      Serial.println(F("[main] (setup) An Error has occurred while mounting LittleFS"));
    #endif
  }
  else{
    httpServer.serveStatic("/", LittleFS, "/");
  }

  httpServer.rewrite("/", "/index.html");
  httpServer.onNotFound(http_notFound);

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  if(WiFi.getMode() == wifi_mode_t::WIFI_MODE_NULL){

    #ifdef DEBUG
     Serial.println(F("[main] (setup) HTTP server will not be started because WiFi it has not been initialized (WIFI_MODE_NULL)"));
    #endif
    
  }else{
    httpServer.begin();

    #if DEBUG > 1000
      Serial.println(F("[main] (setup) HTTP server ready"));
    #endif
  }
  
  oled.logEvent("Ready to Configure", managerOled::LOG_LEVEL_INFO);
  oled.showPage(managerOled::PAGE_EVENT_LOG);

}


void loop() {
  frontPanel.loop();
  oled.loop();
}


/** Handles front panel button press events */
void frontPanelButtonPress(){

  #if DEBUG > 2000
    Serial.println(F("[main] (frontPanelButtonPress) Front Panel button was pressed"));
  #endif

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
 * Sends a 200 response for any OPTIONS requests
*/
void http_options(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse(200);
  request->send(response);
};


/**
 * Handles partitions requests for the internal EEPROM
*/
void http_handlePartitions(AsyncWebServerRequest *request){

  if(request->method() != HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  esp_partition_iterator_t pi = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);

  if (pi != NULL) {
    http_error(request, F("esp_partition_find returned NULL"));
    return;
  }

  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  StaticJsonDocument<768> doc;
  JsonArray array = doc.to<JsonArray>();

  do {
    const esp_partition_t* p = esp_partition_get(pi);
    JsonObject jsonPartition = array.createNestedObject();
    
    jsonPartition["type"] = p->type;
    jsonPartition["subtype"] = p->subtype;
    jsonPartition["address"] = p->address;
    jsonPartition["size"] = p->size;
    jsonPartition["label"] = p->label;

  } while (pi = (esp_partition_next(pi)));

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

  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  StaticJsonDocument<96> doc;
  doc["application"] = VERSION;
  char product_hex[16] = {0};
  sprintf(product_hex, "0x%02X", PRODUCT_HEX);
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
  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  StaticJsonDocument<1536> doc;
  JsonArray array = doc.to<JsonArray>();

  managerInputs::healthResult inputHealth = inputs.health();
  managerOutputs::healthResult outputHealth = outputs.health();
  managerTemperatureSensors::healthResult temperatureHealth = temperatureSensors.health();
  structHealth oledHealth = oled.health();
  structHealth externalEepromHealth = externalEEPROM.health();


  for(int i=0; i < inputHealth.count; i++){
    JsonObject inputObj = array.createNestedObject();
    sprintf(address, "0x%02X", inputHealth.inputControllers[i].address);
    inputObj["address"] = address;
    inputObj["type"] = "Input " + String(i);
    inputObj["online"] = inputHealth.inputControllers[i].enabled;
  }

  for(int i=0; i < outputHealth.count; i++){
    JsonObject outputObj = array.createNestedObject();
    sprintf(address, "0x%02X", outputHealth.outputControllers[i].address);
    outputObj["address"] = address;
    outputObj["type"] = "Output " + String(i);
    outputObj["online"] = outputHealth.outputControllers[i].enabled;
  }

  for(int i=0; i < temperatureHealth.count; i++){
    JsonObject tempObj = array.createNestedObject();
    sprintf(address, "0x%02X", temperatureHealth.sensor[i].address);
    tempObj["address"] = address;
    tempObj["type"] = "Temperature " + String(i);
    tempObj["online"] = temperatureHealth.sensor[i].enabled;
  }

  JsonObject oledObj = array.createNestedObject();
  sprintf(address, "0x%02X", oledHealth.address);
  oledObj["address"] = address;
  oledObj["type"] = "OLED";
  oledObj["online"] = oledHealth.enabled;

  JsonObject externalEepromObj = array.createNestedObject();
  sprintf(address, "0x%02X", externalEepromHealth.address);
  externalEepromObj["address"] = address;
  externalEepromObj["type"] = "EEPROM";
  externalEepromObj["online"] = externalEepromHealth.enabled;

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

  AsyncResponseStream *response = request->beginResponseStream(F("application/json"));
  StaticJsonDocument<128> doc;
  doc["chip_model"] = ESP.getChipModel();
  doc["revision"] = (String)ESP.getChipRevision();
  doc["flash_chip_size"] = ESP.getFlashChipSize();

  serializeJson(doc, *response);
  request->send(response);
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
void http_handleAllNetworkInterfaces(AsyncWebServerRequest *request){

  if(request->method() != HTTP_GET){
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
  objBT["mac_address"] = macAddress;
  objBT["interface"] = F("bluetooth");
  
  getMacAddress(ESP_MAC_ETH, macAddress);
  objEth["mac_address"] = macAddress;
  objEth["interface"] = F("ethernet");
  
  getMacAddress(ESP_MAC_WIFI_STA, macAddress);
  objWifi["mac_address"] = macAddress;
  objWifi["interface"] = F("wifi");

  getMacAddress(ESP_MAC_WIFI_SOFTAP, macAddress);
  objAP["mac_address"] = macAddress;
  objAP["interface"] = F("wifi_ap");
  
  serializeJson(doc, *response);
  request->send(response);
}


/**
 * Handles all requests for the external EEPROM
*/
void http_handleEEPROM(AsyncWebServerRequest *request){

  switch(request->method()){

    case HTTP_OPTIONS:
      http_options(request);
      break;

    case HTTP_GET:
      http_handleEEPROM_GET(request);
      break;

    case HTTP_DELETE:
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
    http_badRequest(request, F("EEPROM already deleted"));
    return;
  }

  if(externalEEPROM.destroy() == false){
    http_error(request, F("Error during EEPROM delete"));
    return;
  }

  oled.setProductID(NULL);
  oled.setUUID(NULL);

  request->send(204);

  oled.logEvent("Deleted EEPROM", managerOled::LOG_LEVEL_NOTIFICATION);
}


/**
 * Handles POST requests for the external EEPROM, which writes data to the EEPROM.
 * The response is synchronous to the operation completing
*/
void http_handleEEPROM_POST(AsyncWebServerRequest *request, JsonVariant doc){

  if (externalEEPROM.enabled == false){
    http_error(request, F("Cannot connect to external EEPROM"));
    return;
  }

  if(strcmp(externalEEPROM.data.uuid, "") != 0){
    http_badRequest(request, F("EEPROM already configured"));
    return;
  }

  MatchState ms;

  if(!doc.containsKey("uuid")){
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

  if(!doc.containsKey("key")){
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

  request->send(204);

  oled.logEvent("Wrote EEPROM", managerOled::LOG_LEVEL_NOTIFICATION);
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
void failureHandler_oled(managerOled::failureCode failureCode){

  switch(failureCode){
    case managerOled::failureCode::NOT_ON_BUS:
      #ifdef DEBUG
        Serial.println(F("[main] (failureHandler_oled) Error: OLED not found on bus"));
      #endif
      break;

    case managerOled::failureCode::UNABLE_TO_START:
      #ifdef DEBUG
        Serial.println(F("[main] (failureHandler_oled) Error: Unable to start OLED"));
      #endif
      break;

    default:
      #ifdef DEBUG
        Serial.println(F("[main] (failureHandler_oled) Error: Unknown OLED failure"));
      #endif
      break;
  }

  frontPanel.setStatus(managerFrontPanel::status::FAILURE);

}


/** 
 * Callback function which handles failures of the external EERPOM 
*/
void failureHandler_eeprom(){

  oled.logEvent("EEPROM Failure", managerOled::LOG_LEVEL_ERROR);

}


/** 
 * Callback function which handles failures of any input 
*/
void failureHandler_inputs(){

  oled.logEvent("Input Failure", managerOled::LOG_LEVEL_ERROR);
 
}


/** 
 * Callback function which handles failures of any output 
*/
void failureHandler_outputs(){

  oled.logEvent("Output Failure", managerOled::LOG_LEVEL_ERROR);
  
}


/** 
 * Callback function which handles failures of any temperature sensor 
*/
void failureHandler_temperatureSensors(String message){

  oled.logEvent("Temperature Failure", managerOled::LOG_LEVEL_ERROR);
  
}