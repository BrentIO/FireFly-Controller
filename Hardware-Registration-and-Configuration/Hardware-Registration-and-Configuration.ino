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

#include "customTypes.h" //P5 Software
#include "hardwareDefinitions.h" //P5 Software
#include "I2C_eeprom.h" //1.7.0, https://github.com/RobTillaart/I2C_EEPROM/releases/tag/1.7.0
#include <ArduinoJson.h> //6.2.0, https://github.com/bblanchon/ArduinoJson/releases/tag/v6.20.0
#include "esp_chip_info.h" //2.0.6, https://github.com/espressif/arduino-esp32/releases/tag/2.0.6
#include <WiFi.h>
#include <WiFiAP.h>
#include <WebServer.h>

WebServer server(80);
I2C_eeprom externalEeprom(ADDRESS_EEPROM, SIZE_EEPROM);
deviceType deviceInfo;


void setup() {

  //Setup a soft AP with the SSID FireFly-######, where the last 6 characters are the last 6 of the Soft AP MAC address
  uint8_t baseMac[6];
  esp_read_mac(baseMac, ESP_MAC_WIFI_SOFTAP);
  char baseMacChr[18] = {0};
  sprintf(baseMacChr, "FireFly-%02X%02X%02X", baseMac[3], baseMac[4], baseMac[5]);
  
  WiFi.softAP(baseMacChr);
  IPAddress myIP = WiFi.softAPIP();

  //Start the external EEPROM
  externalEeprom.begin();
  pinMode(PIN_EEPROM_WP, OUTPUT);

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

  //Ensure we can talk to the EEPROM, otherwise throw an error
  if (!externalEeprom.isConnected())
  {
    handle500(F("Cannot connect to external EEPROM"));
    return;
  }

  //Read the expected data size abck from the EEPROM and put it into deviceInfo
  externalEeprom.readBlock(0, (uint8_t *) &deviceInfo, sizeof(deviceInfo));

  //Do a sanity check to see if the data is printable
  if(!isPrintable(deviceInfo.uuid[0]) || !isPrintable(deviceInfo.uuid[18]) || !isPrintable(deviceInfo.uuid[35])){
    handle404();
    return;
  }
 
  StaticJsonDocument<128> doc;
  doc["uuid"] = deviceInfo.uuid;
  doc["product_id"] = deviceInfo.product_id;

  String output;
  serializeJson(doc, output);
  server.send(200, F("application/json"), output);
}


/**
 * Store the EEPROM configuration with the payload specified.
 * The response is synchronous to the operation completing
*/
void handlePostEEPROM(){

  //Ensure we received a body
  if(server.hasArg("plain") == false) {
    handle400(F("No body was sent with request"));
    return;
  }
  
  //Ensure we can talk to the EEPROM, otherwise throw an error
  if (!externalEeprom.isConnected())
  {
    handle500(F("Cannot connect to external EEPROM"));
    return;
  }

  StaticJsonDocument<192> doc;
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

  if(!doc.containsKey("product_id")){
    handle400(F("Field product_id is required"));
    return;
  }

  //Disable write protection
  digitalWrite(PIN_EEPROM_WP, LOW);
  delay(10);

  //Copy the data to the deviceInfo object
  strcpy(deviceInfo.uuid, doc["uuid"]);
  strcpy(deviceInfo.product_id, doc["product_id"]);

  //Write the deviceInfo object to the external EEPROM
  int writeResponse = externalEeprom.writeBlock(0, (uint8_t *) &deviceInfo, sizeof(deviceInfo));

  //I2C bus will return a non-zero on failure
  if(writeResponse != 0){
    handle500("Error during EEPROM write (" + (String)writeResponse + ")");
  }

  //Enable write protection
  digitalWrite(PIN_EEPROM_WP, HIGH);

  server.send(204);
}


/**
 * Format the EEPROM so it has no data.
 * The response is synchronous to the operation completing
*/
void handleDeleteEEPROM(){

  //Ensure we can talk to the EEPROM, otherwise throw an error
  if (!externalEeprom.isConnected())
  {
    handle500(F("Cannot connect to external EEPROM"));
    return;
  }

  //Disable write protection
  digitalWrite(PIN_EEPROM_WP, LOW);
  delay(10);

  for (uint32_t address = 0; address < SIZE_EEPROM; address += 128)
  {
    externalEeprom.setBlock(address, 0xff, 128);
  }

  //Enable write protection
  digitalWrite(PIN_EEPROM_WP, HIGH);

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