#define VERSION "2024.01.0001"

#define DEBUG 501



#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include "common/hardware.h"
#include "common/outputs.h"
#include "common/inputs.h"
#include "common/temperature.h"
#include "common/frontPanel.h"
#include "common/externalEEPROM.h"
#include "common/oled.h"
#include "time.h"

#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "0.north-america.pool.ntp.org"
#define NTP_SERVER_3 "0.europe.pool.ntp.org"

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
    connectEthernet();

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


void connectEthernet(){

  #if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_W5500

    SPI.begin(SPI_SCK_PIN,SPI_MISO_PIN,SPI_MOSI_PIN,ETHERNET_PIN);

    uint8_t macAddress[6];

    #ifdef ESP32
      esp_read_mac(macAddress, ESP_MAC_ETH);
    #endif

    #ifndef ESP32
      #pragma message "MAC Address using DE:AD:BE:EF because we can't source the MAC Address from an ESP32"
      macAddress[0] = 0xDE;
      macAddress[1] = 0xAD;
      macAddress[2] = 0xBE;
      macAddress[3] = 0xEF;
      macAddress[4] = 0xFE;
      macAddress[5] = 0xED;
    #endif

    //Set Ethernet pins for output
    pinMode(ETHERNET_PIN, OUTPUT);

    delay(500);

    //Disable Ethernet
    digitalWrite(ETHERNET_PIN, LOW);

    delay(500);

    Ethernet.init(ETHERNET_PIN);

    if(Ethernet.begin(macAddress) == 1){

      #ifdef DEBUG
        Serial.println("[main] (connectEthernet) Ethernet Connected");
      #endif

      oled.logEvent("Ethernet Connected",managerOled::LOG_LEVEL_INFO);
    }else{
      oled.logEvent("Ethernet Failure",managerOled::LOG_LEVEL_INFO);
    }

    oled.setEthernetInfo(&Ethernet);

  #endif
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

  outputs.configurePort(2, nsOutputs::outputPin::VARIABLE); //FOR DEBUG ONLY

  #if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_W5500

    switch(Ethernet.maintain()){

      case 1: //Renew Failed
        #ifdef DEBUG
          Serial.println("[main] (loop) DHCP Lease Fail");
        #endif

        oled.logEvent("DHCP Lease Fail", managerOled::logLevel::LOG_LEVEL_INFO);
        break;

      case 2: //Renew Success
        #ifdef DEBUG
          Serial.println("[main] (loop) DHCP Lease OK");
        #endif

        oled.logEvent("DHCP Lease OK", managerOled::logLevel::LOG_LEVEL_INFO);
        break;

      case 3: //Rebind Fail
        #ifdef DEBUG
          Serial.println("[main] (loop) DHCP Rebind Fail");
        #endif

        oled.logEvent("DHCP Rebind Fail", managerOled::logLevel::LOG_LEVEL_INFO);
        break;

      case 4: //Rebind Success
        #ifdef DEBUG
          Serial.println("[main] (loop) DHCP Rebind OK");
        #endif
        
        oled.logEvent("DHCP Rebind OK", managerOled::logLevel::LOG_LEVEL_INFO);
        break;

      default:
        break;

    };

  #endif

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