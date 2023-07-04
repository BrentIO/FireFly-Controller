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


void setup() {

  #ifdef DEBUG
    Serial.begin(115200);
  #endif

  Wire.begin();

  oled.setCallback_failure(&oledFailure);
  oled.begin();

  frontPanel.setCallback_publisher(&frontPanelButtonPress);
  frontPanel.begin();

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
    Serial.println(F("Done."));
    Serial.println("[main] (setup) Started SoftAP " + WiFi.softAPSSID());
  #endif
 
  oled.setWiFiInfo(&WiFi);

  externalEEPROM.setCallback_failure(&eepromFailure);
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

  /* Note, sequence below matters. */
  AsyncCallbackJsonWebHandler* eepromTest = new AsyncCallbackJsonWebHandler("/api/eeprom", http_handleEEPROM_POST);

  httpServer.addHandler(eepromTest);
  httpServer.on("/api/eeprom", http_handleEEPROM);
  httpServer.on("/api/mcu", http_handleMCU);
  httpServer.on("/api/partitions", http_handlePartitions);
  httpServer.on("^\/api\/network\/([a-z_]+)$", http_handleNetworkInterface);
  httpServer.on("/api/network", http_handleAllNetworkInterfaces);

  if (!LittleFS.begin())
  {
    oled.logEvent("Error loading LittleFS", managerOled::LOG_LEVEL_ERROR);

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
      Serial.println(F("[main] (setup) HTTP server ready."));
    #endif
  }
  
  oled.logEvent("Ready to Configure", managerOled::LOG_LEVEL_INFO);

  frontPanel.setStatus(managerFrontPanel::status::NORMAL);
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


/** Handles failures of the OLED display */
void oledFailure(managerOled::failureCode failureCode){

  switch(failureCode){
    case managerOled::failureCode::NOT_ON_BUS:
      #ifdef DEBUG
        Serial.println(F("[main] (oledFailure) Error: OLED not found on bus"));
      #endif
      break;

    case managerOled::failureCode::UNABLE_TO_START:
      #ifdef DEBUG
        Serial.println(F("[main] (oledFailure) Error: Unable to start OLED"));
      #endif
      break;

    default:
      #ifdef DEBUG
        Serial.println(F("[main] (oledFailure) Error: Unknown OLED failure"));
      #endif
      break;
  }

  frontPanel.setStatus(managerFrontPanel::status::FAILURE);

}


void http_notFound(AsyncWebServerRequest *request) {
    request->send(404);
}


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


void http_options(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse(200);
  request->send(response);
};


void http_handlePartitions(AsyncWebServerRequest *request){

  if(request->method() != HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  esp_partition_iterator_t pi = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);

  if (pi != NULL) {

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
    return;

  }
    http_error(request, F("esp_partition_find returned NULL"));
}



/**
 * Retrieve the main control unit hardware information
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
 * Retrieve the specified network interface information based on the requested path
*/
void http_handleNetworkInterface(AsyncWebServerRequest *request){

  #if DEBUG > 2000
    Serial.println("http_handleNetworkInterface()");
  #endif

  if(request->method() != HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  esp_mac_type_t interface = ESP_MAC_IEEE802154; //Not used

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
 * Retrieve all of the network interfaces
*/
void http_handleAllNetworkInterfaces(AsyncWebServerRequest *request){

  #if DEBUG > 2000
    Serial.println("http_handleAllNetworkInterfaces()");
  #endif

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
 * Retrieve the current EEPROM configuration
*/
void http_handleEEPROM_GET(AsyncWebServerRequest *request){

  #if DEBUG > 2000
    Serial.println("http_handleEEPROM_GET()");
  #endif

  //Ensure we can talk to the EEPROM, otherwise throw an error
  if (!externalEEPROM.enabled){
    http_error(request, F("Cannot connect to external EEPROM"));
    return;
  }

  //Do a sanity check to see if the data is printable
  if(!isPrintable(externalEEPROM.data.uuid[0]) || !isPrintable(externalEEPROM.data.uuid[18]) || !isPrintable(externalEEPROM.data.uuid[35])){
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
 * Format the EEPROM so it has no data.
 * The response is synchronous to the operation completing
*/
void http_handleEEPROM_DELETE(AsyncWebServerRequest *request){

  #if DEBUG > 2000
    Serial.println("handleDeleteEEPROM()");
  #endif

  //Ensure we can talk to the EEPROM, otherwise throw an error
  if (!externalEEPROM.enabled)
  {
    http_error(request, F("Cannot connect to external EEPROM"));
    return;
  }

  if(externalEEPROM.destroy() == false){
    http_error(request, F("Error during EEPROM delete."));
    return;
  }

  oled.setProductID(NULL);
  oled.setUUID(NULL);

  request->send(204);
}


/**
 * Store the EEPROM configuration with the payload specified.
 * The response is synchronous to the operation completing
*/
void http_handleEEPROM_POST(AsyncWebServerRequest *request, JsonVariant doc){

  if(request->method()!= HTTP_POST){
    http_methodNotAllowed(request);
    return;
  }

  MatchState ms;

  #if DEBUG > 2000
    Serial.println("http_handleEEPROM_POST()");
  #endif

  //Ensure required fields are passed in the JSON body
  if(!doc.containsKey("uuid")){
    http_badRequest(request, F("Field uuid is required"));
    return;
  }

  if(strlen(doc["uuid"])!=(sizeof(externalEEPROM.data.uuid)-1)){
    http_badRequest(request, F("Field uuid is not exactly 36 characters"));
    return;
  }

  strcpy(externalEEPROM.data.uuid, doc["uuid"]);

  ms.Target(externalEEPROM.data.uuid);

  if(ms.MatchCount("^[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+$")!=1){ //Library does not support lengths of each section, so there is some opportunity for error
    http_badRequest(request, F("Invalid uuid, see Swagger"));
    return;
  }

  if(!doc.containsKey("product_id")){
    http_badRequest(request, F("Field product_id is required"));
    return;
  }

  if(strlen(doc["product_id"])>(sizeof(externalEEPROM.data.product_id)-1)){
    http_badRequest(request, F("Field product_id is greater than 32 characters, see Swagger"));
    return;
  }

  strcpy(externalEEPROM.data.product_id, doc["product_id"]);

  if(!doc.containsKey("key")){
    http_badRequest(request, F("Field key is required"));
    return;
  }

  if(strlen(doc["key"])!=(sizeof(externalEEPROM.data.key)-1)){
    http_badRequest(request, F("Field key is not exactly 64 characters"));
    return;
  }

  strcpy(externalEEPROM.data.key, doc["key"]);

  ms.Target(externalEEPROM.data.key);

  if(ms.MatchCount("^[0-9A-Za-z]+$")!=1){
    http_badRequest(request, F("Invalid key, see Swagger"));
    return;
  }

  //Write the deviceInfo object to the external EEPROM
  if(externalEEPROM.write() == false){
    http_error(request, "Error during EEPROM write.");
    return;
  }

  oled.setProductID(externalEEPROM.data.product_id);
  oled.setUUID(externalEEPROM.data.uuid);

  request->send(204);

}


/**
 * Formats the MAC address into a XX:XX:XX:XX:XX format
*/
void getMacAddress(esp_mac_type_t type, char *buff) {
    uint8_t baseMac[6];
    esp_read_mac(baseMac, type);
    sprintf(buff, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
}


/** Handles failures of external EEPROM */
void eepromFailure(){

  oled.logEvent("EEPROM Failure", managerOled::LOG_LEVEL_ERROR);

  #if DEBUG > 1000
    Serial.println("EEPROM failure was called");
  #endif

}