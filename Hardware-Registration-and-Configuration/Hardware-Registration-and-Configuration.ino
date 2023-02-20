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

#define DEBUG 1000

#include "common/hardware.h"
#include "common/externalEEPROM.h"
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include <esp_chip_info.h> // https://github.com/espressif/arduino-esp32
#include <WiFi.h>
#include <WebServer.h>
#include <Regexp.h> // https://github.com/nickgammon/Regexp


WebServer server(80);
managerExternalEEPROM externalEEPROM;


void setup() {

  #ifdef DEBUG
    Serial.begin(115200);
  #endif

  //Setup a soft AP with the SSID FireFly-######, where the last 6 characters are the last 6 of the Soft AP MAC address
  uint8_t baseMac[6];
  esp_read_mac(baseMac, ESP_MAC_WIFI_SOFTAP);
  char baseMacChr[18] = {0};
  sprintf(baseMacChr, "FireFly-%02X%02X%02X", baseMac[3], baseMac[4], baseMac[5]);
 
  WiFi.softAP(baseMacChr);
  IPAddress myIP = WiFi.softAPIP();

  #ifdef DEBUG
    Serial.println("Started SoftAP " + String(baseMacChr));
  #endif

  externalEEPROM.begin();

  #ifdef DEBUG

   if(externalEEPROM.enabled == true){
      Serial.println("uuid: " + String(externalEEPROM.data.uuid));
      Serial.println("product_id: " + String(externalEEPROM.data.product_id));
      Serial.println("key: " + String(externalEEPROM.data.key));
    }

  #endif
 
  //Configure the HTTP server and enable CORS and cross-origin
  server.enableCORS(true);
  server.enableCrossOrigin(true);

  server.on("/api/eeprom", HTTP_GET, handleGetEEPROM);
  server.on("/api/eeprom", HTTP_POST, handlePostEEPROM);
  server.on("/api/eeprom", HTTP_DELETE, handleDeleteEEPROM);
  server.on("/api/eeprom", HTTP_OPTIONS, handleOptions);
  server.on("/api/network", HTTP_GET, handleGetNetworkInterface_all);
  server.on("/api/network/bluetooth", HTTP_GET, handleGetNetworkInterface);
  server.on("/api/network/ethernet", HTTP_GET, handleGetNetworkInterface);
  server.on("/api/network/wifi", HTTP_GET, handleGetNetworkInterface);
  server.on("/api/network/wifi_ap", HTTP_GET, handleGetNetworkInterface);
  server.on("/api/mcu", HTTP_GET, handleGetMCU);
  server.onNotFound(handle404);

  server.begin();

  #ifdef DEBUG
    Serial.println("HTTP server ready.");
  #endif
}


void loop() {
  server.handleClient();
}


/**
 * Sends back requests for OPTIONS method requests, necessary for AJAX calls
*/
void handleOptions(){
  server.send(200, F("application/json"), "");
}


/**
 * Retrieve the main control unit hardware information
*/
void handleGetMCU(){

  StaticJsonDocument<128> doc;
  doc["chip_model"] = ESP.getChipModel();
  doc["revision"] = (String)ESP.getChipRevision();
  doc["flash_chip_size"] = ESP.getFlashChipSize(); 

  String output;
  serializeJson(doc, output);
  server.send(200, F("application/json"), output);
}


/**
 * Retrieve the specified network interface information based on the requested server.uri
*/
void handleGetNetworkInterface(){

  #ifdef DEBUG
    Serial.println("handleGetNetworkInterface()");
  #endif

  StaticJsonDocument<96> doc;

  if(server.uri() == "/api/network/bluetooth"){
    doc["mac_address"] = getMacAddress(ESP_MAC_BT);
    doc["interface"] = F("bluetooth");
  }

  if(server.uri() == "/api/network/ethernet"){
    doc["mac_address"] = getMacAddress(ESP_MAC_ETH);
    doc["interface"] = F("ethernet");
  }

  if(server.uri() == "/api/network/wifi"){
    doc["mac_address"] = getMacAddress(ESP_MAC_WIFI_STA);
    doc["interface"] = F("wifi");
  }

  if(server.uri() == "/api/network/wifi_ap"){
    doc["mac_address"] = getMacAddress(ESP_MAC_WIFI_SOFTAP);
    doc["interface"] = F("wifi_ap");
  }

  String output;
  serializeJson(doc, output);
  server.send(200, F("application/json"), output);
}


/**
 * Retrieve all of the network interfaces
*/
void handleGetNetworkInterface_all(){

  StaticJsonDocument<384> doc;

  JsonArray array = doc.to<JsonArray>();

  JsonObject objBT = array.createNestedObject();
  JsonObject objEth = array.createNestedObject();
  JsonObject objWifi = array.createNestedObject();
  JsonObject objAP = array.createNestedObject();

  objBT["mac_address"] = getMacAddress(ESP_MAC_BT);
  objBT["interface"] = F("bluetooth");
  
  objEth["mac_address"] = getMacAddress(ESP_MAC_ETH);
  objEth["interface"] = F("ethernet");
  
  objWifi["mac_address"] = getMacAddress(ESP_MAC_WIFI_STA);
  objWifi["interface"] = F("wifi");

  objAP["mac_address"] = getMacAddress(ESP_MAC_WIFI_SOFTAP);
  objAP["interface"] = F("wifi_ap");
  
  String output;
  serializeJson(doc, output);
  server.send(200, F("application/json"), output);
}


/**
 * Retrieve the current EEPROM configuration
*/
void handleGetEEPROM(){

  #ifdef DEBUG
    Serial.println("handleGetEEPROM()");
  #endif

  //Ensure we can talk to the EEPROM, otherwise throw an error
  if (!externalEEPROM.enabled){
    handle500(F("Cannot connect to external EEPROM"));
    return;
  }

  //Do a sanity check to see if the data is printable
  if(!isPrintable(externalEEPROM.data.uuid[0]) || !isPrintable(externalEEPROM.data.uuid[18]) || !isPrintable(externalEEPROM.data.uuid[35])){
    handle404();
    return;
  }
 
  StaticJsonDocument<256> doc;
  doc["uuid"] = externalEEPROM.data.uuid;
  doc["product_id"] = externalEEPROM.data.product_id;
  doc["key"] = externalEEPROM.data.key;

  String output;
  serializeJson(doc, output);
  server.send(200, F("application/json"), output);
}


/**
 * Store the EEPROM configuration with the payload specified.
 * The response is synchronous to the operation completing
*/
void handlePostEEPROM(){

  MatchState ms;

  #ifdef DEBUG
    Serial.println("handlePostEEPROM()");
  #endif

  //Ensure we received a body
  if(server.hasArg("plain") == false) {
    handle400(F("No body was sent with request"));
    return;
  }
  
  //Ensure we can talk to the EEPROM, otherwise throw an error
  if (!externalEEPROM.enabled)
  {
    handle500(F("Cannot connect to external EEPROM"));
    return;
  }

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    handle400(error.c_str());
    return;
  }

  //Ensure required fields are passed in the JSON body
  if(!doc.containsKey("uuid")){
    handle400(F("Field uuid is required"));
    return;
  }

  if(strlen(doc["uuid"])!=36){
    handle400(F("Field uuid is not exactly 36 characters"));
    return;
  }

  strcpy(externalEEPROM.data.uuid, doc["uuid"]);

  ms.Target(externalEEPROM.data.uuid);

  if(ms.MatchCount("^[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+$")!=1){ //Library does not support lengths of each section, so there is some opportunity for error
    handle400(F("Invalid uuid, see Swagger"));
    return;
  }

  if(!doc.containsKey("product_id")){
    handle400(F("Field product_id is required"));
    return;
  }

  if(strlen(doc["product_id"])>32){
    handle400(F("Field product_id is greater than 32 characters, see Swagger"));
    return;
  }

  strcpy(externalEEPROM.data.product_id, doc["product_id"]);

  if(!doc.containsKey("key")){
    handle400(F("Field key is required"));
    return;
  }

  if(strlen(doc["key"])!=64){
    handle400(F("Field key is not exactly 64 characters"));
    return;
  }

  strcpy(externalEEPROM.data.key, doc["key"]);

  ms.Target(externalEEPROM.data.key);

  if(ms.MatchCount("^[0-9A-Za-z]+$")!=1){
    handle400(F("Invalid key, see Swagger"));
    return;
  }

  //Write the deviceInfo object to the external EEPROM
  if(externalEEPROM.write() == false){
    handle500("Error during EEPROM write.");
    return;
  }

  server.send(204);
}


/**
 * Format the EEPROM so it has no data.
 * The response is synchronous to the operation completing
*/
void handleDeleteEEPROM(){

  #ifdef DEBUG
    Serial.println("handleDeleteEEPROM()");
  #endif

  //Ensure we can talk to the EEPROM, otherwise throw an error
  if (!externalEEPROM.enabled)
  {
    handle500(F("Cannot connect to external EEPROM"));
    return;
  }

  if(externalEEPROM.destroy() == false){
    handle500("Error during EEPROM delete.");
    return;
  }

  server.send(204);
}


/**
 * Sends a 404 response indicating a requested resource is not available
*/
void handle404(){
  server.send(404);
}


/**
 * Sends a 400 response indicating the request was invalid
*/
void handle400(String message){
  StaticJsonDocument<96> doc;
  doc["message"] = message;

  String output;
  serializeJson(doc, output);
  server.send(400, F("application/json"), output);
}


/**
 * Sends a 500 response indicating an internal server or hardware failure
*/
void handle500(String error){
  StaticJsonDocument<96> doc;
  doc["error"] = error;

  String output;
  serializeJson(doc, output);
  server.send(500, F("application/json"), output);
}


/**
 * Formats the MAC address into a XX:XX:XX:XX:XX format
*/
String getMacAddress(esp_mac_type_t type) {
    uint8_t baseMac[6];
    esp_read_mac(baseMac, type);
    char baseMacChr[18] = {0};
    sprintf(baseMacChr, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
    return String(baseMacChr);
}
