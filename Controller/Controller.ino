/*
* Controller.ino
*
* Create software-defined lighting.
*
* (C) 2019-2025, P5 Software, LLC
*
*/

#ifndef VERSION
  #error "VERSION must be specified for a build."
#endif
#ifndef COMMIT_HASH
  #error "COMMIT_HASH must be specified for a build."
#endif

#define APPLICATION      "Controller"
#define APPLICATION_NAME "FireFly Controller"

#if CORE_DEBUG_LEVEL > 0
  #include "common/telnetLog.h"   // class definition + #define log_printf — must precede all project headers
#endif

#include "esp_efuse.h"
#include "esp_efuse_table.h"
#include "common/hardware.h"
#include "common/cloudConfig.h"
#include "common/deviceIdentity.h"
#include "common/secretEncryption.h"
#include "common/oled.h"
#include "common/frontPanel.h"
#include "common/inputs.h"
#include "common/temperature.h"
#include "common/outputs.h"
#include "common/eventLog.h"
#include "common/authorizationToken.h"
#include "common/otaConfig.h"
#include "common/cloudDeviceAuth.h"
#include <ArduinoJson.h>
#include "common/psramAllocator.h"
#include "AsyncJson.h"
#include <StreamUtils.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "common/extendedPubSubClient.h"
#include "common/provisioningMode.h"
#include <HTTPClient.h>
#include <mbedtls/hkdf.h>
#include <mbedtls/md.h>
#include <mbedtls/sha256.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/base64.h>
#include <esp_http_client.h>
#include <WiFiClientSecure.h>
#include <esp_crt_bundle.h>

// The _LENGTH macros in extendedPubSubClient.h correctly bound every snprintf destination buffer; these warnings are false positives.
#pragma GCC diagnostic ignored "-Wformat-truncation"

uint64_t bootTime = 0; /* Approximate Epoch time the device booted */
uint64_t lastTimeMemoryBroadcast = 0; /* The last time memory usage was broadcast */
uint64_t lastTimeCloudBackup = 0; /* The last time an automatic cloud backup upload was attempted */
uint32_t lastPublishedHeapFree = UINT32_MAX;         /* Last heap-free value published to MQTT; UINT32_MAX forces publish on first read */
uint32_t lastPublishedLargestFreeBlock = UINT32_MAX; /* Last largest-free-block value published to MQTT */
volatile uint32_t lastTimeHttpServerUsed = 0;  /* Lower 32 bits of esp_timer_get_time() at last authorized HTTP request */
bool httpServerIsActive = false; /* If the HTTP server has been started */
bool _mqttWasConnected = false; /* Tracks prior MQTT connected state to detect disconnect transitions */
AsyncWebServer httpServer(80);
managerDeviceIdentity deviceIdentity; /* Device identity instance */
SecretEncryption secretEncryption; /* File encryption instance */
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
void refreshCertBundle();
void mqtt_publishClientCertState();
void mqtt_publishControllerCertState();
bool cloudBackup_performUpload(int &httpCode, String &errorMsg);
void cloudBackup_scheduleHandler();
void writeBackupEtag();
void http_handleCloudBackup(AsyncWebServerRequest *request);
void http_handleCloudBackup_POST(AsyncWebServerRequest *request);
void http_handleCloudBackup_GET(AsyncWebServerRequest *request);
void http_handleCloudBackup_DELETE(AsyncWebServerRequest *request);

esp32OTA otaFirmware;
WiFiClientSecure _otaHttpsClient;       /* TLS client used when the manifest URL is https:// */

#if CORE_DEBUG_LEVEL > 0
  TelnetLog telnetLog;   // global instance — kept here at original position to avoid early-init UART regression
#endif

String _otaManifestUrl;                 /* Set when setup_OtaFirmware() succeeds; empty otherwise */
bool _otaUpdateInProcess = false;
bool _otaPendingRequest = false;
bool _otaCheckFailed = false;           /* Set by onError during checkForUpdate(); reset before each check */
int _otaCheckErrorCode = 0;             /* Error code captured by onError during checkForUpdate() */
JsonDocument _otaPendingDoc;
static char _otaCurrentPartition[8] = "";
static char _otaLatestVersion[32] = "";
static char _otaReleaseUrl[256] = "";
static int _otaLastPublishedPercentage = -1;
uint64_t _otaLastCheckedTime = 0;

fs::LittleFSFS uiFS;
fs::LittleFSFS configFS;
bool uiFS_isMounted = false;
bool configFS_isMounted = false;
char _uiApplication[64] = {0};
char _uiVersion[16] = {0};
char _uiCommit[16] = {0};

char* _certBundle = nullptr;            /* PSRAM-backed concatenated PEM bundle for OTA TLS */
size_t _certBundleSize = 0;             /* Byte length of _certBundle, excluding null terminator */

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

  /// @brief UUID of the paired FireFly-Client; empty string when unpaired
  char clientUUID[UUID_LENGTH + 1];

  /// @brief Reference to the channels on this input port
  inputChannel channels[IO_EXTENDER_COUNT_CHANNELS_PER_PORT];
};

inputPort inputPorts[(IO_EXTENDER_COUNT_PINS / IO_EXTENDER_COUNT_CHANNELS_PER_PORT) * IO_EXTENDER_COUNT];


void reportMemoryUsage(const char* tag) {
  log_d("[%lu] ***Memory Usage Report*** [%s]\tHeap Free: %lu, Largest Free Heap Block: %lu",
          timeClient.getEpochTime(),
          tag,
          (unsigned long)ESP.getFreeHeap(),
          (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
}


/**
 * One-time setup
*/
void setup() {

  #if CORE_DEBUG_LEVEL > 0
    Serial.begin(115200);
  #endif

  #if CORE_DEBUG_LEVEL >= 4
    reportMemoryUsage("Setup begin.");
  #endif /* CORE_DEBUG_LEVEL >= 4 */

  if(!psramFound()){
    eventLog.createEvent("No PSRAM found", EventLog::LOG_LEVEL_INFO);
  }

  log_i("VDD_SDIO eFuse: %s",
    (esp_efuse_read_field_bit(ESP_EFUSE_XPD_SDIO_REG) &&
     esp_efuse_read_field_bit(ESP_EFUSE_XPD_SDIO_TIEH)) ? "set" : "not set");

  eventLog.createEvent("Event log started");
  
  Wire.begin();

  #if CORE_DEBUG_LEVEL >= 4
    reportMemoryUsage("Wire started.");
  #endif /* CORE_DEBUG_LEVEL >= 4 */

  /* Start the auth token service */
  authToken.begin();


  //Configure the peripherals
  oled.setCallback_failure(&failureHandler_oled);
  oled.begin();
  oled.setApplicationName(APPLICATION_NAME);
  oled.setApplicationVersion(VERSION);
  oled.setEventLog(&eventLog);
  oled.setAuthorizationToken(&authToken);
  authToken.setCallback_visualTokenChanged(&eventHandler_visualAuthChanged);


  /* Set event log callbacks to the OLED */
  eventLog.setCallback_info(&eventHandler_eventLogInfoEvent);
  eventLog.setCallback_notification(&eventHandler_eventLogNotificationEvent);
  eventLog.setCallback_error(&eventHandler_eventLogErrorEvent);
  eventLog.setCallback_resolveError(&eventHandler_eventLogResolvedErrorEvent);

  /* Start device identity */
  deviceIdentity.begin();

  if(deviceIdentity.enabled == true){

    if(!secretEncryption.begin(deviceIdentity.data.key, sizeof(deviceIdentity.data.key))){
      eventLog.createEvent("Enc init fail", EventLog::LOG_LEVEL_ERROR);
      log_e("SecretEncryption init failed");
    } else {
      memset(deviceIdentity.data.key, 0, sizeof(deviceIdentity.data.key));
    }

    oled.setProductID(deviceIdentity.data.product_id);
    oled.setUUID(deviceIdentity.data.uuid);

    log_d("eFuse UUID: %s", deviceIdentity.data.uuid);
    log_d("eFuse Product Hex: 0x%08lX", (unsigned long)deviceIdentity.data.product_hex);
    log_d("Product ID: %s", deviceIdentity.data.product_id);
    log_d("eFuse master key loaded");

    if(deviceIdentity.data.product_hex != 0 && deviceIdentity.data.product_hex != PRODUCT_HEX){
      oled.setMismatchHex(deviceIdentity.data.product_hex, (uint32_t)PRODUCT_HEX);
      oled.setPage(managerOled::PAGE_HW_FW_MISMATCH);
      log_e("HW/FW product_hex mismatch (HW: 0x%08lX, FW: 0x%08lX).", deviceIdentity.data.product_hex, (uint32_t)PRODUCT_HEX);
      esp_ota_img_states_t ota_state;
      if(esp_ota_get_state_partition(esp_ota_get_running_partition(), &ota_state) == ESP_OK
         && ota_state == ESP_OTA_IMG_PENDING_VERIFY){
        otaFirmware.markAppInvalid(); /* reboots into previous OTA slot; never returns */
      }
      esp_deep_sleep_start();
    }
  }

  /* Startup the front panel */
  frontPanel.setCallback_publisher(&eventHandler_frontPanelButtonPress);
  frontPanel.setCallback_state_closed_at_begin(&eventHandler_frontPanelButtonClosedAtBegin);
  frontPanel.begin();


  /* Set callbacks for provisioning mode */
  provisioningMode.setCallback_active(&eventHandler_provisioningModeActive);
  provisioningMode.setCallback_inactive(&eventHandler_provisioningModeInactive);
  provisioningMode.setCallback_rogueClient(&eventHandler_rogueClient);


  #if WIFI_MODEL == ENUM_WIFI_MODEL_ESP32
    oled.setWiFiInfo(&WiFi);
    provisioningMode.setWiFI(&WiFi);
  #endif

  #if CORE_DEBUG_LEVEL >= 4
    reportMemoryUsage("Starting Ethernet.");
  #endif /* CORE_DEBUG_LEVEL >= 4 */

  #if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_W5500 && defined(ESP32)

    ESP32_W5500_onEvent();
    ESP32_W5500_setCallback_connected(&eventHandler_ethernetConnect);
    ESP32_W5500_setCallback_disconnected(&eventHandler_ethernetDisconnect);

    log_d("Setting up Ethernet on W5500");
    uint64_t ethernet_start_time = esp_timer_get_time();

    uint8_t ethMac[6];
    esp_read_mac(ethMac, ESP_MAC_ETH);
    if(!ETH.begin(ETH_PHY_W5500, 1, ETHERNET_PIN, ETHERNET_PIN_INTERRUPT, ETHERNET_PIN_RESET,
                  ETH_SPI_HOST, SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SPI_CLOCK_MHZ)){
      eventLog.resolveError("Ethernet begin fail");
      return;
    }

    char hostname[18] = {0};
    sprintf(hostname, "FireFly-%02X%02X%02X", ethMac[3], ethMac[4], ethMac[5]);
    ETH.setHostname(hostname);

    while(!ESP32_W5500_isConnected()){

      delay(100);

      if(esp_timer_get_time() > ethernet_start_time + (ETHERNET_TIMEOUT * 1000ULL)){
          log_w("Ethernet connection timeout");
        break;
      }
    }

    if(ESP32_W5500_isConnected()){

      timeClient.begin();
      updateNTPTime(true);

      #if CORE_DEBUG_LEVEL > 0
        telnetLog.begin();
      #endif
    }

    log_d("Ethernet IP: %s", ETH.localIP().toString().c_str());
    oled.setEthernetInfo(&ETH);

  #endif

  #if CORE_DEBUG_LEVEL >= 4
    reportMemoryUsage("Ethernet started.");
  #endif /* CORE_DEBUG_LEVEL >= 4 */

  #if CORE_DEBUG_LEVEL >= 4
    reportMemoryUsage("Starting Inputs.");
  #endif /* CORE_DEBUG_LEVEL >= 4 */

  /* Start inputs */
  inputs.setCallback_failure(&failureHandler_inputs);
  inputs.setCallback_publisher(&eventHandler_inputs);
  inputs.begin();

  #if CORE_DEBUG_LEVEL >= 4
    reportMemoryUsage("Inputs started; starting outputs.");
  #endif /* CORE_DEBUG_LEVEL >= 4 */

  /* Start outputs */
  outputs.setCallback_failure(&failureHandler_outputs);
  outputs.setCallback_outputValueChanged(&mqtt_publishOutputValueChanged);
  outputs.begin();

  #if CORE_DEBUG_LEVEL >= 4
    reportMemoryUsage("Outputs started; starting temperature sensors.");
  #endif /* CORE_DEBUG_LEVEL >= 4 */

  /* Start temperature sensors */
  temperatureSensors.setCallback_publisher(&eventHandler_temperature);
  temperatureSensors.setCallback_failure(&failureHandler_temperatureSensors);
  temperatureSensors.begin();

  #if CORE_DEBUG_LEVEL >= 4
    reportMemoryUsage("Temperature sensors started; starting http server.");
  #endif /* CORE_DEBUG_LEVEL >= 4 */

  /* Start LittleFS for ui */
  if(uiFS.begin(false, "/uiFS", (uint8_t)10U, "ui"))
  {
    uiFS_isMounted = true;

    File vf = uiFS.open("/version.json", "r");
    if(vf && vf.size() > 0){
      JsonDocument uiDoc;
      if(!deserializeJson(uiDoc, vf)){
        strncpy(_uiApplication, uiDoc["application"] | "", sizeof(_uiApplication)-1);
        strncpy(_uiVersion,     uiDoc["version"]     | "", sizeof(_uiVersion)-1);
        strncpy(_uiCommit,      uiDoc["commit"]      | "", sizeof(_uiCommit)-1);
      }
    }
    if(vf) vf.close();

    if(_uiApplication[0] != '\0' && (
        strcmp(_uiApplication, APPLICATION)                        != 0 ||
        strcmp(_uiVersion,     VERSION) != 0 ||
        strcmp(_uiCommit,      COMMIT_HASH)                        != 0)){
      eventLog.createEvent("App/UI ver mismatch", EventLog::LOG_LEVEL_ERROR);
      log_i("App/UI mismatch — app: %s/%s/%s  ui: %s/%s/%s",
            APPLICATION, VERSION, COMMIT_HASH,
            _uiApplication, _uiVersion, _uiCommit);
    }
  }
  else{
    eventLog.createEvent("uiFS mount fail", EventLog::LOG_LEVEL_ERROR);
    log_e("An Error has occurred while mounting uiFS");
  }


  /* Start LittleFS for config */
  if(configFS.begin(false, "/configFS", (uint8_t)10U, "config"))
  {
    configFS_isMounted = true;

    if(!configFS.exists(CONFIGFS_PATH_CERTS + (String)"/")){
      if(!configFS.mkdir(CONFIGFS_PATH_CERTS)){
        eventLog.createEvent("Err mkdir certs", EventLog::LOG_LEVEL_ERROR);
        configFS_isMounted = false;
      };
    }

    if(!configFS.exists(CONFIGFS_PATH_CONTROLLERS + (String)"/")){
      if(!configFS.mkdir(CONFIGFS_PATH_CONTROLLERS)){
        eventLog.createEvent("Err mkdir ctlrs", EventLog::LOG_LEVEL_ERROR);
        configFS_isMounted = false;
      };
    }

    if(!configFS.exists(CONFIGFS_PATH_CLIENTS + (String)"/")){
      if(!configFS.mkdir(CONFIGFS_PATH_CLIENTS)){
        eventLog.createEvent("Err mkdir clients", EventLog::LOG_LEVEL_ERROR);
        configFS_isMounted = false;
      };
    }

  }
  else{
    eventLog.createEvent("configFS mount fail", EventLog::LOG_LEVEL_ERROR);
    log_e("An Error has occurred while mounting configFS");
  }

  if(configFS.exists("/backup.json.upload_in_progress")){
    configFS.remove("/backup.json.upload_in_progress");
  }

  refreshCertBundle();

  /* If configFS is mounted and this device has no controller config, scan for a provisioning AP
     and attempt to pull all controller and client records from the source controller.
  */
  #if WIFI_MODEL == ENUM_WIFI_MODEL_ESP32
  if(configFS_isMounted && deviceIdentity.enabled){

    String ownControllerFile = CONFIGFS_PATH_CONTROLLERS + (String)"/" + deviceIdentity.data.uuid;

    if(!configFS.exists(ownControllerFile)){

      eventLog.createEvent("No cfg, scanning AP");
      log_i("No controller config found; scanning for provisioning AP");

      WiFi.mode(WIFI_STA);
      int networkCount = WiFi.scanNetworks();

      int provisioningNetworkIndex = -1;

      for(int i = 0; i < networkCount; i++){
        if(WiFi.SSID(i) == "FireFly-Provisioning"){
          provisioningNetworkIndex = i;
          break;
        }
      }

      if(provisioningNetworkIndex >= 0){

        uint8_t bssid[6];
        memcpy(bssid, WiFi.BSSID(provisioningNetworkIndex), 6);

        char apPassword[13] = {0};
        const char apPasswordHex[] = "0123456789ABCDEF";
        for(int i = 0; i < 6; i++){
          apPassword[i * 2]     = apPasswordHex[bssid[i] >> 4];
          apPassword[i * 2 + 1] = apPasswordHex[bssid[5 - i] & 0xF];
        }
        apPassword[12] = '\0';

        uint8_t ethMac[6];
        esp_read_mac(ethMac, ESP_MAC_ETH);
        log_i("Ethernet MAC: %02x:%02x:%02x:%02x:%02x:%02x", ethMac[0], ethMac[1], ethMac[2], ethMac[3], ethMac[4], ethMac[5]);

        esp_err_t macSetResult = esp_wifi_set_mac(WIFI_IF_STA, ethMac);
        if(macSetResult != ESP_OK){
          log_e("Failed to set WiFi STA MAC to Ethernet MAC: %s", esp_err_to_name(macSetResult));
        } else {
          log_i("WiFi STA MAC overridden to Ethernet MAC");
        }

        log_i("Found FireFly-Provisioning AP, connecting...");
        WiFi.begin("FireFly-Provisioning", apPassword);

        uint64_t connectStart = esp_timer_get_time();
        while(WiFi.status() != WL_CONNECTED){
          delay(200);
          if(esp_timer_get_time() - connectStart > 10ULL * 1000000ULL){
            log_w("Timed out connecting to provisioning AP");
            break;
          }
        }

        if(WiFi.status() == WL_CONNECTED){

          log_i("Connected to provisioning AP; requesting provisioning token");

          WiFiClient wifiClientForProvisioning;
          HTTPClient httpProvisioning;

          String ownMac = WiFi.macAddress();
          ownMac.toLowerCase();

          JsonDocument tokenRequestDoc;
          tokenRequestDoc["uuid"] = deviceIdentity.data.uuid;
          tokenRequestDoc["mac_address"] = ownMac;
          String tokenRequestBody;
          serializeJson(tokenRequestDoc, tokenRequestBody);

          httpProvisioning.begin(wifiClientForProvisioning, "http://192.168.4.1/api/provisioning/token");
          httpProvisioning.addHeader("Content-Type", "application/json");
          int tokenHttpCode = httpProvisioning.POST(tokenRequestBody);

          if(tokenHttpCode == HTTP_CODE_OK){

            String tokenPayload = httpProvisioning.getString();
            httpProvisioning.end();

            JsonDocument tokenDoc;
            DeserializationError tokenErr = deserializeJson(tokenDoc, tokenPayload);

            if(!tokenErr && !tokenDoc["token"].isNull()){

              uint32_t provToken = tokenDoc["token"].as<uint32_t>();
              char provTokenStr[12];
              snprintf(provTokenStr, sizeof(provTokenStr), "%" PRIu32, provToken);
              log_i("Got provisioning token; cleaning up local config");
              log_d("Prov: token=%u", provToken);

              {
                File ctlrRoot = configFS.open(CONFIGFS_PATH_CONTROLLERS + (String)"/");
                if(ctlrRoot){
                  File f = ctlrRoot.openNextFile();
                  while(f){
                    String fname = f.name();
                    f.close();
                    configFS.remove(CONFIGFS_PATH_CONTROLLERS + (String)"/" + fname);
                    f = ctlrRoot.openNextFile();
                  }
                  ctlrRoot.close();
                }
              }
              {
                File clientRoot = configFS.open(CONFIGFS_PATH_CLIENTS + (String)"/");
                if(clientRoot){
                  File f = clientRoot.openNextFile();
                  while(f){
                    String fname = f.name();
                    f.close();
                    configFS.remove(CONFIGFS_PATH_CLIENTS + (String)"/" + fname);
                    f = clientRoot.openNextFile();
                  }
                  clientRoot.close();
                }
              }
              if(configFS.exists("/backup.json")){
                configFS.remove("/backup.json");
              }
              if(configFS.exists("/backup.etag")){
                configFS.remove("/backup.etag");
              }

              int expected_controllers = 0;
              int actual_controllers = 0;

              httpProvisioning.begin(wifiClientForProvisioning, "http://192.168.4.1/api/controllers");
              httpProvisioning.addHeader("provisioning-token", provTokenStr);
              int controllersListCode = httpProvisioning.GET();

              if(controllersListCode == HTTP_CODE_OK){
                String ctlrListPayload = httpProvisioning.getString();
                httpProvisioning.end();
                JsonDocument ctlrListDoc;
                DeserializationError ctlrListErr = deserializeJson(ctlrListDoc, ctlrListPayload);
                if(!ctlrListErr){
                  JsonArray ctlrUuids = ctlrListDoc.as<JsonArray>();
                  expected_controllers = ctlrUuids.size();
                  for(JsonVariant uuidVar : ctlrUuids){
                    String uuid = uuidVar.as<String>();
                    httpProvisioning.begin(wifiClientForProvisioning, "http://192.168.4.1/api/controllers/" + uuid);
                    httpProvisioning.addHeader("provisioning-token", provTokenStr);
                    log_d("Prov: GET /api/controllers/%s token=%u", uuid.c_str(), provToken);
                    int ctlrCode = httpProvisioning.GET();
                    if(ctlrCode == HTTP_CODE_OK){
                      String ctlrPayload = httpProvisioning.getString();
                      httpProvisioning.end();
                      if(secretEncryption.encryptToFile(configFS, CONFIGFS_PATH_CONTROLLERS + (String)"/" + uuid, ctlrPayload)){
                        actual_controllers++;
                      } else {
                        log_e("Prov: failed to write controller %s", uuid.c_str());
                      }
                    } else {
                      log_e("Prov: GET /api/controllers/%s returned %d", uuid.c_str(), ctlrCode);
                      httpProvisioning.end();
                    }
                  }
                } else {
                  log_e("Prov: failed to parse controller list: %s", ctlrListErr.c_str());
                }
              } else {
                log_e("Prov: GET /api/controllers returned %d", controllersListCode);
                httpProvisioning.end();
              }

              int expected_clients = 0;
              int actual_clients = 0;

              httpProvisioning.begin(wifiClientForProvisioning, "http://192.168.4.1/api/clients");
              httpProvisioning.addHeader("provisioning-token", provTokenStr);
              int clientsListCode = httpProvisioning.GET();

              if(clientsListCode == HTTP_CODE_OK){
                String clientListPayload = httpProvisioning.getString();
                httpProvisioning.end();
                JsonDocument clientListDoc;
                DeserializationError clientListErr = deserializeJson(clientListDoc, clientListPayload);
                if(!clientListErr){
                  JsonArray clientUuids = clientListDoc.as<JsonArray>();
                  expected_clients = clientUuids.size();
                  for(JsonVariant clientUuidVar : clientUuids){
                    String clientUuid = clientUuidVar.as<String>();
                    httpProvisioning.begin(wifiClientForProvisioning, "http://192.168.4.1/api/clients/" + clientUuid);
                    httpProvisioning.addHeader("provisioning-token", provTokenStr);
                    log_d("Prov: GET /api/clients/%s token=%u", clientUuid.c_str(), provToken);
                    int clientCode = httpProvisioning.GET();
                    if(clientCode == HTTP_CODE_OK){
                      String clientPayload = httpProvisioning.getString();
                      httpProvisioning.end();
                      if(secretEncryption.encryptToFile(configFS, CONFIGFS_PATH_CLIENTS + (String)"/" + clientUuid, clientPayload)){
                        actual_clients++;
                      } else {
                        log_e("Prov: failed to write client %s", clientUuid.c_str());
                      }
                    } else {
                      log_e("Prov: GET /api/clients/%s returned %d", clientUuid.c_str(), clientCode);
                      httpProvisioning.end();
                    }
                  }
                } else {
                  log_e("Prov: failed to parse client list: %s", clientListErr.c_str());
                }
              } else {
                log_e("Prov: GET /api/clients returned %d", clientsListCode);
                httpProvisioning.end();
              }

              int backupResult = 0; // 0=fail, 1=ok, 2=not_found

              httpProvisioning.begin(wifiClientForProvisioning, "http://192.168.4.1/backup");
              httpProvisioning.addHeader("provisioning-token", provTokenStr);
              int backupCode = httpProvisioning.GET();

              if(backupCode == HTTP_CODE_OK){
                String backupPayload = httpProvisioning.getString();
                httpProvisioning.end();
                JsonDocument backupValidate;
                DeserializationError backupErr = deserializeJson(backupValidate, backupPayload);
                if(!backupErr){
                  File backupFile = configFS.open("/backup.json", "w");
                  if(backupFile){
                    backupFile.print(backupPayload);
                    backupFile.close();

                    writeBackupEtag();
                    backupResult = 1;
                  } else {
                    log_e("Prov: failed to open backup.json for writing");
                  }
                } else {
                  log_e("Prov: backup JSON parse failed: %s", backupErr.c_str());
                }
              } else if(backupCode == HTTP_CODE_NOT_FOUND){
                log_i("Prov: donor has no backup");
                backupResult = 2;
                httpProvisioning.end();
              } else {
                log_e("Prov: GET /backup returned %d", backupCode);
                httpProvisioning.end();
              }

              WiFi.disconnect(true);

              oled.setPage(managerOled::PAGE_EVENT_LOG);

              bool countsOK = (actual_controllers == expected_controllers) && (actual_clients == expected_clients);

              char oledLine[OLED_CHARACTERS_PER_LINE+1];

              if(actual_controllers == expected_controllers && actual_clients == expected_clients){
                snprintf(oledLine, sizeof(oledLine), "Prov OK %dC %dL", actual_controllers, actual_clients);
                eventLog.createEvent(oledLine);
                log_i("Prov: %d/%d controllers, %d/%d clients OK", actual_controllers, expected_controllers, actual_clients, expected_clients);
              } else if(actual_controllers != expected_controllers){
                snprintf(oledLine, sizeof(oledLine), "Prov fail C %d/%d", actual_controllers, expected_controllers);
                eventLog.createEvent(oledLine, EventLog::LOG_LEVEL_ERROR);
                log_e("Prov: controller mismatch: got %d expected %d", actual_controllers, expected_controllers);
              } else {
                snprintf(oledLine, sizeof(oledLine), "Prov fail L %d/%d", actual_clients, expected_clients);
                eventLog.createEvent(oledLine, EventLog::LOG_LEVEL_ERROR);
                log_e("Prov: client mismatch: got %d expected %d", actual_clients, expected_clients);
              }

              char oledLine2[OLED_CHARACTERS_PER_LINE+1];

              if(backupResult == 1){
                snprintf(oledLine2, sizeof(oledLine2), "Prov OK got backup");
                eventLog.createEvent(oledLine2);
              } else if(backupResult == 2){
                snprintf(oledLine2, sizeof(oledLine2), "Prov OK no backup");
                eventLog.createEvent(oledLine2);
              } else {
                snprintf(oledLine2, sizeof(oledLine2), "Prov bad backup");
                eventLog.createEvent(oledLine2, EventLog::LOG_LEVEL_ERROR);
              }

              if(countsOK && backupResult != 0){
                log_i("Prov: complete; rebooting in 5 seconds");
                eventLog.createEvent("Rebooting...", EventLog::LOG_LEVEL_NOTIFICATION);
                oled.loop();
                delay(5000);
                ESP.restart();
              }

            } else {
              log_w("Prov: failed to parse token response; continuing unprovisioned");
            }

          } else {
            log_w("Prov: token request returned %d; continuing unprovisioned", tokenHttpCode);
            httpProvisioning.end();
          }

          WiFi.disconnect(true);

        }

      } else {
        log_i("No FireFly-Provisioning AP found; continuing unprovisioned");
      }

      WiFi.disconnect(true);

    }
  }
  #endif /* WIFI_MODEL == ENUM_WIFI_MODEL_ESP32 */

  /* Configure the http server.
    IMPORTANT: *** Sequence below matters, they are sorted specific to generic ***
  */

  httpServer.on("/api/version", http_handleVersion);
  httpServer.on("/api/reboot", http_handleReboot_POST);
  httpServer.on("/api/events", http_handleEventLog);
  httpServer.on("/api/errors", http_handleErrorLog);
  httpServer.on("/auth", http_handleAuth);
  httpServer.on("/files", http_handleFileList_GET);

  if(configFS_isMounted){
    AsyncCallbackJsonWebHandler* controllersHandler = new AsyncCallbackJsonWebHandler("^/api/controllers/([0-9a-f-]+)$", http_handleControllers_PUT);
    controllersHandler->setMaxContentLength(65535);
    controllersHandler->setMethod(HTTP_PUT);
    httpServer.addHandler(controllersHandler);
    httpServer.on("^/api/controllers$", HTTP_ANY, http_handleListControllers);
    httpServer.on("^/api/controllers/([0-9a-f-]+)$", http_handleControllers);
    AsyncCallbackJsonWebHandler* clientsHandler = new AsyncCallbackJsonWebHandler("^/api/clients/([0-9a-f-]+)$", http_handleClients_PUT);
    clientsHandler->setMaxContentLength(65535);
    clientsHandler->setMethod(HTTP_PUT);
    httpServer.addHandler(clientsHandler);
    httpServer.on("^/api/clients$", HTTP_ANY, http_handleListClients);
    httpServer.on("^/api/clients/([0-9a-f-]+)$", http_handleClients);
    httpServer.on("/backup", HTTP_PUT, http_handleBackup_PUT, nullptr, http_handleBackup_PUT_body);
    httpServer.on("/backup", http_handleBackup);
    httpServer.on("/api/provisioning/token", HTTP_OPTIONS, http_options);
    AsyncCallbackJsonWebHandler* provisioningTokenHandler = new AsyncCallbackJsonWebHandler("/api/provisioning/token", http_handleProvisioningToken);
    httpServer.addHandler(provisioningTokenHandler);
    httpServer.on("/api/provisioning/certs/client", http_handleProvisioningCerts_client);
    httpServer.on("/api/provisioning/certs/controller", http_handleProvisioningCerts_controller);
    httpServer.on("/api/provisioning", http_handleProvisioning);
    httpServer.on("^/certs/([a-z0-9_.]+)$", http_handleCert);
    httpServer.on("^/certs$", HTTP_ANY, http_handleCerts, http_handleCerts_Upload);
    httpServer.on("/api/ota", HTTP_OPTIONS, http_options);
    httpServer.addHandler(new AsyncCallbackJsonWebHandler("/api/ota", http_handleOTA_POST));
    httpServer.on("/api/cloud-backup", http_handleCloudBackup);
    setup_OtaFirmware();
  }else{
    log_e("configFS is not mounted");
    httpServer.on("^/certs/([a-z0-9_.]+)$", http_configFSNotMunted);
    httpServer.on("^/certs$", HTTP_ANY, http_configFSNotMunted);
    httpServer.on("/api/controllers", http_configFSNotMunted);
    httpServer.on("/api/clients", http_configFSNotMunted);
    httpServer.on("/backup", http_configFSNotMunted);
  }

  if(uiFS_isMounted){
    httpServer.serveStatic("/", uiFS, "/", "public, max-age=86400");
    httpServer.rewrite("/", "/index.html");
  }

  httpServer.onNotFound(http_notFound);

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*"); //Ignore CORS
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "visual-token, provisioning-token, Content-Type"); //Ignore CORS
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS, PUT, DELETE"); //Ignore CORS


  startHttpServer();

  #if CORE_DEBUG_LEVEL >= 4
    reportMemoryUsage("Setting up MQTT.");
  #endif /* CORE_DEBUG_LEVEL >= 4 */

  setupMQTT();

  #if CORE_DEBUG_LEVEL >= 4
    reportMemoryUsage("MQTT Started.  Setting up IO.");
  #endif /* CORE_DEBUG_LEVEL >= 4 */

  setupIO();

  setControllerNameOnOLED();

  oled.setPage(managerOled::PAGE_EVENT_LOG);

  otaFirmware.markAppValid();
  otaFirmware.setCurrentVersion(VERSION);
  otaFirmware.setBlockedPartitions({"config"});

  #if CORE_DEBUG_LEVEL >= 4
    reportMemoryUsage("Setup complete.");
  #endif /* CORE_DEBUG_LEVEL >= 4 */

}


/**
 * Main loop
*/
void loop() {

  updateNTPTime();

  if(configFS_isMounted && esp_timer_get_time() > 30ULL * 1000000ULL){ //Wait 30 seconds after booting before uploading cloud backup
    if((esp_timer_get_time() - lastTimeCloudBackup >= (uint64_t)CLOUD_BACKUP_INTERVAL_SECONDS * 1000000ULL) || (lastTimeCloudBackup == 0)){
      if(configFS.exists("/backup.json")){
        cloudBackup_scheduleHandler();
      }
      lastTimeCloudBackup = esp_timer_get_time();
    }
  }

  if(!_otaManifestUrl.isEmpty() && esp_timer_get_time() > 60ULL * 1000000ULL){ //Wait 60 seconds after booting before checking the firmware
    if((_otaLastCheckedTime == 0) || (esp_timer_get_time() - _otaLastCheckedTime >= (uint64_t)FIRMWARE_CHECK_SECONDS * 1000000ULL)){
      if(!_otaUpdateInProcess){
        #if CORE_DEBUG_LEVEL >= 4
          reportMemoryUsage("Starting firmware check.");
        #endif /* CORE_DEBUG_LEVEL >= 4 */

        _otaCheckFailed = false;
        _otaCheckErrorCode = 0;
        bool updateAvailable = otaFirmware.checkForUpdate();

        if(updateAvailable){
          otaFirmware.execOTA(); /* onAvailable already published MQTT update-available state */
        } else if(_otaCheckFailed){
          eventLog.createEvent("OTA check failed");
          /* onError already published "offline" to availability topic; do not overwrite with "online" */
          if(deviceIdentity.enabled && mqttClient.connected()){
            const char* notifyMessage;
            switch(_otaCheckErrorCode){
              case -1:
                notifyMessage = "Could not reach the OTA server. Please verify network connectivity.";
                break;
              case 404:
                notifyMessage = "No firmware was found for this device. The OTA URL may be incorrect.";
                break;
              case 409:
                notifyMessage = "This device is running a revoked firmware version and cannot update automatically. Manual intervention is required.";
                break;
              case 500:
                notifyMessage = "The OTA server returned an error. This is likely transient; the device will retry.";
                break;
              case ESP_ERR_INVALID_ARG:
                notifyMessage = "The firmware manifest could not be parsed. The server may have returned an unexpected response.";
                break;
              case ESP_ERR_INVALID_STATE:
                notifyMessage = "OTA is not properly configured on this device.";
                break;
              case ESP_ERR_NOT_FOUND:
                notifyMessage = "This device was not found in the firmware manifest. The OTA URL may be incorrect.";
                break;
              default:
                notifyMessage = "The firmware update check failed. Please verify network connectivity and OTA configuration.";
                break;
            }
            JsonDocument notifyDoc;
            notifyDoc["title"] = "⚠️ FireFly Controller OTA Check Failed";
            notifyDoc["message"] = notifyMessage;
            char notificationId[64];
            snprintf(notificationId, sizeof(notificationId), "firefly_ota_error_%s", deviceIdentity.data.uuid);
            notifyDoc["notification_id"] = notificationId;
            mqttClient.beginPublish("homeassistant/persistent_notification/create", measureJson(notifyDoc), false);
            BufferingPrint bufferedNotify(mqttClient, 32);
            serializeJson(notifyDoc, bufferedNotify);
            bufferedNotify.flush();
            mqttClient.endPublish();
          }
        } else {
          /* Service reachable, no update — publish online + current version */
          if(deviceIdentity.enabled && mqttClient.connected()){
            char availability_topic[MQTT_TOPIC_UPDATE_AVAILABILITY_LENGTH+1];
            snprintf(availability_topic, sizeof(availability_topic), MQTT_TOPIC_UPDATE_AVAILABILITY_PATTERN, deviceIdentity.data.uuid);
            mqttClient.publish(availability_topic, "online", true);

            JsonDocument mqttDoc;
            mqttDoc["installed_version"] = VERSION;
            mqttDoc["latest_version"] = VERSION;
            char topic[MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH+1];
            snprintf(topic, sizeof(topic), MQTT_TOPIC_UPDATE_STATE_PATTERN, deviceIdentity.data.uuid);
            mqttClient.beginPublish(topic, measureJson(mqttDoc), true);
            BufferingPrint bufferedClient(mqttClient, 32);
            serializeJson(mqttDoc, bufferedClient);
            bufferedClient.flush();
            mqttClient.endPublish();
          }
          eventLog.createEvent("OTA firmware checked");
        }
      }

      _otaLastCheckedTime = esp_timer_get_time();
      #if CORE_DEBUG_LEVEL >= 4
        reportMemoryUsage("Firmware check complete.");
      #endif /* CORE_DEBUG_LEVEL >= 4 */
    }
  }

  if(esp_timer_get_time() - lastTimeMemoryBroadcast >= (uint64_t)MEMORY_USAGE_REPORT_SECONDS * 1000000ULL){
    lastTimeMemoryBroadcast = esp_timer_get_time();
    uint32_t currentHeapFree         = (uint32_t)ESP.getFreeHeap();
    uint32_t currentLargestFreeBlock = (uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
    if(currentHeapFree != lastPublishedHeapFree || currentLargestFreeBlock != lastPublishedLargestFreeBlock){
      lastPublishedHeapFree         = currentHeapFree;
      lastPublishedLargestFreeBlock = currentLargestFreeBlock;
      #if CORE_DEBUG_LEVEL >= 4
        reportMemoryUsage("Main loop timer elapsed.");
      #endif /* CORE_DEBUG_LEVEL >= 4 */
      mqtt_publishMemoryUsage();
    }
  }

  if(httpServerIsActive){
    uint32_t _httpIdleElapsed = (uint32_t)esp_timer_get_time() - lastTimeHttpServerUsed;
    if(_httpIdleElapsed >= (uint32_t)HTTP_SERVER_MAX_IDLE_SECONDS * 1000000UL) {
      log_d("HTTP idle timeout: elapsed=%lu s threshold=%d s lastUsed=%lu",
            _httpIdleElapsed / 1000000UL, HTTP_SERVER_MAX_IDLE_SECONDS, lastTimeHttpServerUsed);
      stopHttpServer();
    }
  }

  otaFirmware_checkPending();

  oled.loop();
  authToken.loop();
  frontPanel.loop();
  inputs.loop();
  outputs.loop();
  temperatureSensors.loop();
  provisioningMode.loop();

  #if CORE_DEBUG_LEVEL > 0
    telnetLog.loop();
  #endif

  #if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_W5500
    if(ESP32_W5500_isConnected()){
      bool _mqttLoopResult = mqttClient.loop();
      if(!_mqttLoopResult && mqttClient.enabled == true){
        if(_mqttWasConnected){
          mqtt_onDisconnect();
        }
        mqtt_reconnect();
      } else {
        _mqttWasConnected = _mqttLoopResult && mqttClient.enabled;
      }
    }
  #endif

  #if ETHERNET_MODEL != ENUM_ETHERNET_MODEL_W5500
    #warning MQTT will not automatically reconnect with this ethernet model
  #endif

}

/** Handles changes in observed temperatures 
 * @param location the location where the change was observed
 * @param value the new temperature in degrees celsius
*/
void eventHandler_temperature(const char* location, float value){

  if(deviceIdentity.enabled == false){
    return;
  }

  char temperature[6];
  snprintf(temperature, sizeof(temperature), "%.2f", value);

  char topic[MQTT_TOPIC_TEMPERATURE_STATE_PATTERN_LENGTH+1];
  snprintf(topic, sizeof(topic), MQTT_TOPIC_TEMPERATURE_STATE_PATTERN, deviceIdentity.data.uuid, location);

  mqttClient.publish(topic, temperature, true);
};


/** 
 * Callback function which handles failures of any temperature sensor 
*/
void failureHandler_temperatureSensors(uint8_t address, managerTemperatureSensors::failureReason failureReason){

  char text[OLED_CHARACTERS_PER_LINE+1];

  if(failureReason == managerTemperatureSensors::failureReason::ADDRESS_OFFLINE){
      snprintf(text, sizeof(text), "Temp sen 0x%02X offline", address);
  }else{
      snprintf(text, sizeof(text), "Temp sen 0x%02X fail %i", address, failureReason);
  }

  eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
  frontPanel.setStatus(managerFrontPanel::status::FAILURE);

  const char* location = temperatureSensors.getSensorLocationByAddress(address);

  if(location != nullptr && mqttClient.connected()){
    char availability_topic[MQTT_TOPIC_TEMPERATURE_AVAILABILITY_LENGTH+1];
    snprintf(availability_topic, sizeof(availability_topic), MQTT_TOPIC_TEMPERATURE_AVAILABILITY_PATTERN, deviceIdentity.data.uuid, location);
    mqttClient.publish(availability_topic, "offline", true);
  }
}


/** Handles changes in observed inputs 
 * @param portChannel the port and channel where the change was observed
 * @param changeState the state of the change observed
*/
void eventHandler_inputs(managerInputs::portChannel portChannel, managerInputs::changeState changeState){

  log_d("Port %u channel %u %s", portChannel.port, portChannel.channel + portChannel.offset,
    changeState == managerInputs::changeState::CHANGE_STATE_NORMAL         ? "NORMAL" :
    changeState == managerInputs::changeState::CHANGE_STATE_SHORT_DURATION ? "SHORT"  :
    changeState == managerInputs::changeState::CHANGE_STATE_LONG_DURATION  ? "LONG"   : "UNKNOWN");

  if(inputPorts[portChannel.port-1].id[0] == '\0'){
    return;
  }

  char state_topic[MQTT_TOPIC_INPUT_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, sizeof(state_topic), MQTT_TOPIC_INPUT_STATE_PATTERN, inputPorts[portChannel.port-1].id, (portChannel.channel+portChannel.offset));

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
    default:
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
        default:
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

  nsOutputs::set_result returnValue = nsOutputs::set_result::SUCCESS;

  switch(action){

    case outputAction::INCREASE:
      if(currentPortValue == 0){
        returnValue = outputs.setPortValue(port, outputs.getPortStartBrightness(port));
      }else{
        returnValue = outputs.setPortValue(port, (currentPortValue + 10));
      }
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

  char text[OLED_CHARACTERS_PER_LINE+1];

  if(failureReason == managerOled::failureReason::ADDRESS_OFFLINE){
      snprintf(text, sizeof(text), "OLED 0x%02X offline", address);
  }else{
      snprintf(text, sizeof(text), "OLED 0x%02X fail %i", address, failureReason);
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


/** 
 * Callback function which handles failures of the external EERPOM 
*/

/** 
 * Callback function which handles failures of any input 
*/
void failureHandler_inputs(uint8_t address, managerInputs::failureReason failureReason){
	
	char text[OLED_CHARACTERS_PER_LINE+1];

  if(failureReason == managerInputs::failureReason::ADDRESS_OFFLINE){
      snprintf(text, sizeof(text), "Inpt ctl 0x%02X offline", address);
  }else{
      snprintf(text, sizeof(text), "Inpt ctl 0x%02X fail %i", address, failureReason);
  }
  
  eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
  frontPanel.setStatus(managerFrontPanel::status::FAILURE);

  if(mqttClient.connected()){
    char availability_topic[MQTT_TOPIC_INPUT_CONTROLLER_AVAILABILITY_LENGTH+1];
    snprintf(availability_topic, sizeof(availability_topic), MQTT_TOPIC_INPUT_CONTROLLER_AVAILABILITY_PATTERN, deviceIdentity.data.uuid, address);
    mqttClient.publish(availability_topic, "offline", true);
  }
}


/**
 * Callback function which handles failures of any output
*/
void failureHandler_outputs(uint8_t address, nsOutputs::failureReason failureReason){

  char text[OLED_CHARACTERS_PER_LINE+1];

  if(failureReason == nsOutputs::failureReason::ADDRESS_OFFLINE){
      snprintf(text, sizeof(text), "Out ctl 0x%02X offline", address);
  }else{
      snprintf(text, sizeof(text), "Out ctl 0x%02X fail %i", address, failureReason);
  }

  eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
  frontPanel.setStatus(managerFrontPanel::status::FAILURE);

  if(mqttClient.connected()){
    char availability_topic[MQTT_TOPIC_OUTPUT_CONTROLLER_AVAILABILITY_LENGTH+1];
    snprintf(availability_topic, sizeof(availability_topic), MQTT_TOPIC_OUTPUT_CONTROLLER_AVAILABILITY_PATTERN, deviceIdentity.data.uuid, address);
    mqttClient.publish(availability_topic, "offline", true);
  }
}


/**
 * Updates the NTP time from the server with special handling
*/
void updateNTPTime(bool force){

  if((esp_timer_get_time() > ntpSleepUntil) || force){
      /*
        Workaround until https://github.com/arduino-libraries/NTPClient/pull/163 is merged
        If time client update is unsuccessful, stop trying for 5 minutes
      */

      if(timeClient.update()){
        ntpSleepUntil = 0;
      }else{
        ntpSleepUntil = esp_timer_get_time() + 300ULL * 1000000ULL; // 5 minutes in µs
        return; // Stop early if update failed
      }
  }

  if(timeClient.isTimeSet() && bootTime == 0){
      unsigned long epoch = timeClient.getEpochTime();
      bootTime = (uint64_t)epoch - (esp_timer_get_time() / 1000000ULL);
      mqtt_publishStartTime();
  }
}


/** 
 * Sets the controller name on the OLED display
 */
void setControllerNameOnOLED(){

  if(deviceIdentity.enabled == false){
    return;
  }


  String filename = CONFIGFS_PATH_CONTROLLERS + (String)"/" + deviceIdentity.data.uuid;

  if(configFS.exists(filename)){
    JsonDocument filter;
    filter["name"] = true;

    String plaintext;
    if(secretEncryption.decryptFromFile(configFS, filename, plaintext)){
      JsonDocument doc;
      deserializeJson(doc, plaintext, DeserializationOption::Filter(filter));
      oled.setName(doc["name"]);
    } else {
      eventLog.createEvent("Config decrypt fail", EventLog::LOG_LEVEL_ERROR);
      log_e("Failed to decrypt %s", filename.c_str());
    }

  }

}


/**
 * Sends a 404 response indicating the resource is not found
*/
void http_notFound(AsyncWebServerRequest *request) {
    log_d("http_notFound: method=%d url=%s", request->method(), request->url().c_str());
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
 * Sends a 409 response indicating a resource conflict (e.g. provisioning mode inactive)
*/
void http_conflict(AsyncWebServerRequest *request, String message){

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc;
  doc["message"] = message;

  serializeJson(doc, *response);
  response->setCode(409);
  request->send(response);
}


/**
 * Sends a 503 response indicating the service is temporarily unavailable
*/
void http_serviceUnavailable(AsyncWebServerRequest *request, String message){

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc;
  doc["message"] = message;

  serializeJson(doc, *response);
  response->setCode(503);
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
 * Handles the version endpoint requests
*/
void http_handleVersion(AsyncWebServerRequest *request){

  if(request->method() == HTTP_OPTIONS){
    http_options(request);
    return;
  }

  if(request->method() != HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  resetHTPServerUsage();

  if(deviceIdentity.enabled == false){
    http_error(request, "Device identity unavailable");
    return;
  }

  if(strcmp(deviceIdentity.data.uuid, "") == 0){
    http_error(request, "Device identity invalid");
    return;
  }

  char product_hex[16] = {0};
  sprintf(product_hex, "0x%08X", PRODUCT_HEX);
 
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc;
  doc["uuid"] = deviceIdentity.data.uuid;
  doc["product_id"] = deviceIdentity.data.product_id;
  doc["product_hex"] = product_hex;
  doc["application"]["name"]    = APPLICATION;
  doc["application"]["version"] = VERSION;
  doc["application"]["commit"]  = COMMIT_HASH;
  if(_uiApplication[0] != '\0'){
    doc["ui"]["name"]    = _uiApplication;
    doc["ui"]["version"] = _uiVersion;
    doc["ui"]["commit"]  = _uiCommit;
  } else {
    doc["ui"] = (const char*)nullptr;
  }

  serializeJson(doc, *response);
  request->send(response);
}


/**
 * POST /api/reboot — reboots the device after a short delay.
*/
void http_handleReboot_POST(AsyncWebServerRequest *request){

  if(request->method() == HTTP_OPTIONS){
    http_options(request);
    return;
  }

  if(!request->hasHeader("visual-token")){
    http_unauthorized(request);
    return;
  }

  if(!authToken.authenticate(request->header("visual-token").c_str())){
    http_unauthorized(request);
    return;
  }

  if(request->method() != HTTP_POST){
    http_methodNotAllowed(request);
    return;
  }

  resetHTPServerUsage();
  eventLog.createEvent("Rebooting...", EventLog::LOG_LEVEL_NOTIFICATION);

  request->onDisconnect([]() {
    ESP.restart();
  });

  request->send(204);
}


/**
 * Handle http requests for the event log
*/
void http_handleEventLog(AsyncWebServerRequest *request){

  if(request->method() == HTTP_OPTIONS){
    http_options(request);
    return;
  }

  if(!request->hasHeader("visual-token")){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header("visual-token").c_str())){
    http_unauthorized(request);
    return;
  }

  if(request->method() != HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  resetHTPServerUsage();

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

  if(request->method() == HTTP_OPTIONS){
    http_options(request);
    return;
  }

  if(!request->hasHeader("visual-token")){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header("visual-token").c_str())){
    http_unauthorized(request);
    return;
  }

  if(request->method() != HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  resetHTPServerUsage();

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
 * Creates a long-term authorization with the given visual token
*/
void http_handleAuth(AsyncWebServerRequest *request){

  switch(request->method()){

    case HTTP_OPTIONS:
      http_options(request);
      break;

    case HTTP_POST:
      if(!request->hasHeader("visual-token")){
        http_unauthorized(request);
        return;
      }
    
      if(!authToken.authenticate(request->header("visual-token").c_str(), true)){
        http_unauthorized(request);
        return;
      }

      resetHTPServerUsage();
    
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
  log_d("http_handleControllers: method=%d url=%s", request->method(), request->url().c_str());
  switch(request->method()){

    case HTTP_OPTIONS:
      http_options(request);
      break;

    case HTTP_GET:
      http_handleControllers_GET(request);
      break;

    case HTTP_DELETE:
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

  log_d("http_handleControllers_GET: pathArg(0)=%s visual-token=%d provisioning-token=%d", request->pathArg(0).c_str(), request->hasHeader("visual-token"), request->hasHeader("provisioning-token"));

  if(request->hasHeader("visual-token")){
    if(!authToken.authenticate(request->header("visual-token").c_str())){
      http_unauthorized(request);
      return;
    }
  } else if(request->hasHeader("provisioning-token")){
    if(!authenticateWithProvisioningToken(request)){
      return;
    }
  } else {
    log_w("http_handleControllers_GET: no auth header from %s", request->client()->remoteIP().toString().c_str());
    http_unauthorized(request);
    return;
  }

  resetHTPServerUsage();

  String filename = CONFIGFS_PATH_CONTROLLERS + (String)"/" + request->pathArg(0);

  if(!configFS.exists(filename)){
    http_notFound(request);
    return;
  }

  String plaintext;
  if(!secretEncryption.decryptFromFile(configFS, filename, plaintext)){
    http_error(request, "Unable to decrypt file");
    return;
  }
  request->send(200, "application/json", plaintext);

}


/**
 * Handles Controller DELETEs
*/
void http_handleControllers_DELETE(AsyncWebServerRequest *request){

  if(!request->hasHeader("visual-token")){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header("visual-token").c_str())){
    http_unauthorized(request);
    return;
  }

  resetHTPServerUsage();

  String filename = CONFIGFS_PATH_CONTROLLERS + (String)"/" + request->pathArg(0);

  if(!configFS.exists(filename)){
    http_notFound(request);
    return;
  }

  if(configFS.remove(filename)){
    request->send(204);
  }else{
    http_error(request, "Failed when trying to delete file");
  }
}


/**
 * Handles Controller PUTs
*/
void http_handleControllers_PUT(AsyncWebServerRequest *request, JsonVariant doc){
  log_d("http_handleControllers_PUT: method=%d url=%s", request->method(), request->url().c_str());

  if(request->method() == HTTP_OPTIONS){
    http_options(request);
    return;
  }

  if(!request->hasHeader("visual-token")){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header("visual-token").c_str())){
    http_unauthorized(request);
    return;
  }

  if(request->method() != HTTP_PUT){
    http_methodNotAllowed(request);
    return;
  }

  resetHTPServerUsage();

  if(request->pathArg(0).length() != 36){
    http_badRequest(request, "UUID must be exactly 36 characters");
    return;
  }

  if(!deviceIdentity.enabled){
    http_conflict(request, "Device not provisioned");
    return;
  }

  String jsonStr;
  serializeJson(doc, jsonStr);
  if(!secretEncryption.encryptToFile(configFS, CONFIGFS_PATH_CONTROLLERS + (String)"/" + request->pathArg(0), jsonStr)){
    http_error(request, "Unable to open the file for writing");
    return;
  }

  request->send(204);
}


/**
 * Handles List Controllers
*/
void http_handleListControllers(AsyncWebServerRequest *request){

  log_d("http_handleListControllers: method=%d url=%s", request->method(), request->url().c_str());

  if(request->method() == HTTP_OPTIONS){
    http_options(request);
    return;
  }

  if(request->hasHeader("visual-token")){
    if(!authToken.authenticate(request->header("visual-token").c_str())){
      http_unauthorized(request);
      return;
    }
  } else if(request->hasHeader("provisioning-token")){
    if(!authenticateWithProvisioningToken(request)){
      return;
    }
  } else {
    http_unauthorized(request);
    return;
  }

  if(request->method() != HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  resetHTPServerUsage();

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc(&spiRamAllocator);  //Supports 128 UUID's
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

    case HTTP_OPTIONS:
      http_options(request);
      break;

    case HTTP_GET:
      http_handleClients_GET(request);
      break;

    case HTTP_DELETE:
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

  log_d("http_handleClients_GET: pathArg(0)=%s visual-token=%d provisioning-token=%d", request->pathArg(0).c_str(), request->hasHeader("visual-token"), request->hasHeader("provisioning-token"));

  if(request->hasHeader("visual-token")){
    if(!authToken.authenticate(request->header("visual-token").c_str())){
      http_unauthorized(request);
      return;
    }
  } else if(request->hasHeader("provisioning-token")){
    if(!authenticateWithProvisioningToken(request)){
      return;
    }
  } else {
    log_w("http_handleClients_GET: no auth header from %s", request->client()->remoteIP().toString().c_str());
    http_unauthorized(request);
    return;
  }

  resetHTPServerUsage();

  String filename = CONFIGFS_PATH_CLIENTS + (String)"/" + request->pathArg(0);

  if(!configFS.exists(filename)){
    http_notFound(request);
    return;
  }

  String plaintext;
  if(!secretEncryption.decryptFromFile(configFS, filename, plaintext)){
    http_error(request, "Unable to decrypt file");
    return;
  }
  request->send(200, "application/json", plaintext);

}




/**
 * Handles Client DELETEs
*/
void http_handleClients_DELETE(AsyncWebServerRequest *request){

  if(!request->hasHeader("visual-token")){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header("visual-token").c_str())){
    http_unauthorized(request);
    return;
  }

  resetHTPServerUsage();

  String filename = CONFIGFS_PATH_CLIENTS + (String)"/" + request->pathArg(0);

  if(!configFS.exists(filename)){
    http_notFound(request);
    return;
  }

  if(configFS.remove(filename)){
    request->send(204);
  }else{
    http_error(request, "Failed when trying to delete file");
  }
}


/**
 * Handles Clients PUTs
*/
void http_handleClients_PUT(AsyncWebServerRequest *request, JsonVariant doc){
  log_d("http_handleClients_PUT: method=%d url=%s", request->method(), request->url().c_str());

  if(request->method() == HTTP_OPTIONS){
    http_options(request);
    return;
  }

  if(!request->hasHeader("visual-token")){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header("visual-token").c_str())){
    http_unauthorized(request);
    return;
  }

  if(request->method() != HTTP_PUT){
    http_methodNotAllowed(request);
    return;
  }

  resetHTPServerUsage();

  if(request->pathArg(0).length() != 36){
    http_badRequest(request, "UUID must be exactly 36 characters");
    return;
  }

  if(!deviceIdentity.enabled){
    http_conflict(request, "Device not provisioned");
    return;
  }

  String jsonStr;
  serializeJson(doc, jsonStr);
  if(!secretEncryption.encryptToFile(configFS, CONFIGFS_PATH_CLIENTS + (String)"/" + request->pathArg(0), jsonStr)){
    http_error(request, "Unable to open the file for writing");
    return;
  }

  request->send(204);
}


/**
 * Handles List Clients
*/
void http_handleListClients(AsyncWebServerRequest *request){

  if(request->method() == HTTP_OPTIONS){
    http_options(request);
    return;
  }

  if(request->hasHeader("visual-token")){
    if(!authToken.authenticate(request->header("visual-token").c_str())){
      http_unauthorized(request);
      return;
    }
  } else if(request->hasHeader("provisioning-token")){
    if(!authenticateWithProvisioningToken(request)){
      return;
    }
  } else {
    http_unauthorized(request);
    return;
  }

  if(request->method() != HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  resetHTPServerUsage();

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc(&spiRamAllocator);  //Supports 128 UUID's
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

  if(request->method() == HTTP_OPTIONS){
    http_options(request);
    return;
  }

  if(!request->hasHeader("visual-token")){
    http_unauthorized(request);
    return;
  }

  if(!authToken.authenticate(request->header("visual-token").c_str())){
    http_unauthorized(request);
    return;
  }
  
  switch(request->method()){

    case HTTP_PUT:
      resetHTPServerUsage();
      http_handleProvisioning_PUT(request);
      break;

    case HTTP_GET:
      resetHTPServerUsage();
      http_handleProvisioning_GET(request);
      break;

    case HTTP_DELETE:
      resetHTPServerUsage();
      http_handleProvisioning_DELETE(request);
      break;

    default:
      http_methodNotAllowed(request);
      break;
  }
}


void http_handleProvisioning_GET(AsyncWebServerRequest *request){

  JsonDocument doc;
  doc["enabled"] = provisioningMode.getStatus();

  if(provisioningMode.getStatus()){
    doc["ssid"] = "FireFly-Provisioning";
    char password[13];
    provisioningMode.getSoftAPPassword(password);
    doc["password"] = password;
  }

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  serializeJson(doc, *response);

  request->send(response);
}


void http_handleProvisioning_PUT(AsyncWebServerRequest *request){

  if(!configFS_isMounted){
    http_error(request, "File system not mounted");
    return;
  }

  request->send(202);

  JsonDocument filter;
  filter["mac_address"] = true;

  File root = configFS.open(CONFIGFS_PATH_CLIENTS + (String)"/");
  File file = root.openNextFile();

  while(file){

    if(!file.isDirectory()){
      String clientPath = CONFIGFS_PATH_CLIENTS + (String)"/" + file.name();
      file.close();

      String plaintext;
      if(secretEncryption.decryptFromFile(configFS, clientPath, plaintext)){
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, plaintext, DeserializationOption::Filter(filter));
        if(!error){
          provisioningMode.addAllowedMac(doc["mac_address"].as<std::string>());
        }
      } else {
        eventLog.createEvent("Client decrypt fail", EventLog::LOG_LEVEL_ERROR);
        log_e("Failed to decrypt %s", clientPath.c_str());
      }
    } else {
      file.close();
    }

    file = root.openNextFile();
  }

  root.close();
  provisioningMode.setActive();
}

void http_handleProvisioning_DELETE(AsyncWebServerRequest *request){
  request->send(202);
  provisioningMode.setInactive();
}


void eventHandler_provisioningModeActive(){
  eventLog.createEvent("Provisioning active", EventLog::LOG_LEVEL_NOTIFICATION);
}


void eventHandler_provisioningModeInactive(){
  eventLog.createEvent("Provisioning inactive");
}


void eventHandler_rogueClient(const char* macAddress){
  char oledText[OLED_CHARACTERS_PER_LINE+1];
  snprintf(oledText, sizeof(oledText), "!RC %s", macAddress);
  eventLog.createEvent(oledText, EventLog::LOG_LEVEL_NOTIFICATION);
};


/// @brief Scans client config files to find the UUID whose stored MAC matches the given MAC address
/// @param mac MAC address in xx:xx:xx:xx:xx:xx format (lowercase)
/// @return The UUID string if found, empty string if not found
String findClientUuidByMac(const char* mac){

  JsonDocument filter;
  filter["mac_address"] = true;

  File root = configFS.open(CONFIGFS_PATH_CLIENTS + (String)"/");
  File file = root.openNextFile();

  while(file){

    if(!file.isDirectory()){
      String clientName = file.name();
      String clientPath = CONFIGFS_PATH_CLIENTS + (String)"/" + clientName;
      file.close();

      String plaintext;
      if(secretEncryption.decryptFromFile(configFS, clientPath, plaintext)){
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, plaintext, DeserializationOption::Filter(filter));

        if(!error){
          String storedMac = doc["mac_address"].as<String>();
          storedMac.toLowerCase();
          String incomingMac = String(mac);
          incomingMac.toLowerCase();

          if(storedMac == incomingMac){
            root.close();
            return clientName;
          }
        }
      } else {
        eventLog.createEvent("Client decrypt fail", EventLog::LOG_LEVEL_ERROR);
        log_e("Failed to decrypt %s", clientPath.c_str());
      }
    } else {
      file.close();
    }

    file = root.openNextFile();
  }

  root.close();
  return String();
}


bool isRequestViaSoftAP(AsyncWebServerRequest *request){
  return request->client()->localIP() == WiFi.softAPIP();
}


/**
 * Validates the provisioning-token header against the active provisioning mode.
 * Sends an appropriate error response and returns false if authentication fails.
 * Always enforces SoftAP-only access to prevent Ethernet token replay.
 */
bool authenticateWithProvisioningToken(AsyncWebServerRequest *request){
  if(!isRequestViaSoftAP(request)){
    log_w("Provisioning token auth rejected: not via SoftAP (localIP=%s)", request->client()->localIP().toString().c_str());
    http_forbiddenRequest(request, "Only accessible via provisioning network");
    return false;
  }
  if(provisioningMode.getStatus() != true){
    log_w("Provisioning token auth rejected: provisioning mode inactive");
    http_conflict(request, "Provisioning mode inactive");
    return false;
  }
  if(!request->hasHeader("provisioning-token")){
    log_w("Provisioning token auth rejected: no header from %s", request->client()->remoteIP().toString().c_str());
    http_unauthorized(request);
    return false;
  }
  uint32_t token = (uint32_t)strtoul(request->header("provisioning-token").c_str(), nullptr, 10);
  log_d("Provisioning token auth: received \"%s\" (parsed=%u) from %s", request->header("provisioning-token").c_str(), token, request->client()->remoteIP().toString().c_str());
  if(!provisioningMode.validateToken(token)){
    log_w("Provisioning token rejected: \"%s\" from %s", request->header("provisioning-token").c_str(), request->client()->remoteIP().toString().c_str());
    http_unauthorized(request);
    return false;
  }
  return true;
}


void http_handleProvisioningToken(AsyncWebServerRequest *request, JsonVariant doc){

  if(request->method() == HTTP_OPTIONS){
    http_options(request);
    return;
  }

  if(request->method() != HTTP_POST){
    http_methodNotAllowed(request);
    return;
  }

  if(!isRequestViaSoftAP(request)){
    log_w("Provisioning token request rejected: not via SoftAP (localIP=%s)", request->client()->localIP().toString().c_str());
    http_forbiddenRequest(request, "Only accessible via provisioning network");
    return;
  }

  if(provisioningMode.getStatus() != true){
    log_w("Provisioning token request rejected: provisioning mode inactive");
    http_conflict(request, "Provisioning mode inactive");
    return;
  }

  if(doc["uuid"].isNull() || doc["uuid"].as<String>().length() != 36){
    http_badRequest(request, "uuid must be 36 characters");
    return;
  }

  if(doc["mac_address"].isNull() || doc["mac_address"].as<String>().isEmpty()){
    http_badRequest(request, "mac_address is required");
    return;
  }

  String uuid = doc["uuid"].as<String>();
  String controllerPath = CONFIGFS_PATH_CONTROLLERS + (String)"/" + uuid;

  if(!configFS.exists(controllerPath)){
    log_w("Provisioning token request: no controller file for uuid=%s from %s", uuid.c_str(), request->client()->remoteIP().toString().c_str());
    http_notFound(request);
    return;
  }

  JsonDocument filter;
  filter["mac_address"] = true;

  String plaintext;
  if(!secretEncryption.decryptFromFile(configFS, controllerPath, plaintext)){
    http_error(request, "Unable to decrypt controller file");
    return;
  }

  JsonDocument storedDoc;
  DeserializationError storedErr = deserializeJson(storedDoc, plaintext, DeserializationOption::Filter(filter));
  if(storedErr || storedDoc["mac_address"].isNull()){
    log_w("Provisioning token request: controller %s has no mac_address field", uuid.c_str());
    http_notFound(request);
    return;
  }

  String storedMac = storedDoc["mac_address"].as<String>();
  storedMac.toLowerCase();
  String incomingMac = doc["mac_address"].as<String>();
  incomingMac.toLowerCase();

  if(storedMac != incomingMac){
    log_w("Provisioning token request: MAC mismatch for %s: stored='%s' incoming='%s'", uuid.c_str(), storedMac.c_str(), incomingMac.c_str());
    eventLog.createEvent("Prov mode MAC mismatch", EventLog::LOG_LEVEL_NOTIFICATION);
    http_unauthorized(request);
    return;
  }

  uint32_t token = provisioningMode.issueToken(storedMac.c_str());
  log_i("Provisioning token issued: %u for uuid=%s mac=%s", token, uuid.c_str(), storedMac.c_str());

  JsonDocument responseDoc;
  responseDoc["token"] = token;
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  serializeJson(responseDoc, *response);
  request->send(response);
}


/**
 * Returns the UUID of the controller whose `mac` field matches the provided MAC address,
 * or an empty String if no match is found.
 * @param mac the MAC address to search for (comparison is case-insensitive)
 */
String findControllerUuidByMac(const char* mac){

  JsonDocument filter;
  filter["mac_address"] = true;

  File root = configFS.open(CONFIGFS_PATH_CONTROLLERS + (String)"/");
  if(!root){
    log_e("findControllerUuidByMac: failed to open controllers directory");
    return String();
  }

  File file = root.openNextFile();

  if(!file){
    log_w("findControllerUuidByMac: no files found in controllers directory");
  }

  while(file){

    if(!file.isDirectory()){
      String controllerName = file.name();
      String controllerPath = CONFIGFS_PATH_CONTROLLERS + (String)"/" + controllerName;
      log_d("findControllerUuidByMac: checking file name=%s path=%s", controllerName.c_str(), controllerPath.c_str());
      file.close();

      String plaintext;
      if(secretEncryption.decryptFromFile(configFS, controllerPath, plaintext)){
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, plaintext, DeserializationOption::Filter(filter));

        if(!error){
          if(doc["mac_address"].isNull()){
            log_d("findControllerUuidByMac: %s has no mac_address field", controllerName.c_str());
          } else {
            String storedMac = doc["mac_address"].as<String>();
            log_d("findControllerUuidByMac: stored mac='%s' incoming mac='%s'", storedMac.c_str(), mac);
            storedMac.toLowerCase();
            String incomingMac = String(mac);
            incomingMac.toLowerCase();

            if(storedMac == incomingMac){
              root.close();
              return controllerName;
            }
          }
        } else {
          log_e("findControllerUuidByMac: failed to parse JSON in %s: %s", controllerPath.c_str(), error.c_str());
        }
      } else {
        eventLog.createEvent("Ctlr decrypt fail", EventLog::LOG_LEVEL_ERROR);
        log_e("Failed to decrypt %s", controllerPath.c_str());
      }
    } else {
      file.close();
    }

    file = root.openNextFile();
  }

  root.close();
  return String();
}




/**
 * Computes the SHA-256 of /backup.json and writes it to /backup.etag as a
 * lowercase hex string.  Replaces any existing etag file.  If /backup.json
 * cannot be opened, any existing /backup.etag is removed so the two files
 * remain in sync.
 */
void writeBackupEtag(){
  File f = configFS.open("/backup.json", "r");
  if(!f){
    if(configFS.exists("/backup.etag")){
      configFS.remove("/backup.etag");
    }
    return;
  }

  mbedtls_sha256_context sha;
  mbedtls_sha256_init(&sha);
  mbedtls_sha256_starts(&sha, 0);
  uint8_t buf[256];
  while(f.available()){
    size_t n = f.read(buf, sizeof(buf));
    if(n > 0) mbedtls_sha256_update(&sha, buf, n);
  }
  f.close();

  uint8_t hash[32];
  mbedtls_sha256_finish(&sha, hash);
  mbedtls_sha256_free(&sha);

  char etagHex[65];
  for(int i = 0; i < 32; i++){
    sprintf(etagHex + i*2, "%02x", hash[i]);
  }
  etagHex[64] = '\0';
  if(configFS.exists("/backup.etag")){
    configFS.remove("/backup.etag");
  }
  File etagFile = configFS.open("/backup.etag", "w");
  if(etagFile){
    etagFile.print(etagHex);
    etagFile.close();
  }
}


/**
 * Generic handler for /backup
 */
void http_handleBackup(AsyncWebServerRequest *request){
  switch(request->method()){

    case HTTP_OPTIONS:
      http_options(request);
      break;

    case HTTP_HEAD:
      http_handleBackup_HEAD(request);
      break;

    case HTTP_GET:
      http_handleBackup_GET(request);
      break;

    case HTTP_DELETE:
      http_handleBackup_DELETE(request);
      break;

    default:
      http_methodNotAllowed(request);
      break;
  }
}


/**
 * Handles Backup HEADs — returns ETag of stored backup without body
*/
void http_handleBackup_HEAD(AsyncWebServerRequest *request){

  if(!request->hasHeader("visual-token")){
    http_unauthorized(request);
    return;
  }

  if(!authToken.authenticate(request->header("visual-token").c_str())){
    http_unauthorized(request);
    return;
  }

  if(!configFS.exists("/backup.etag")){
    http_notFound(request);
    return;
  }

  resetHTPServerUsage();

  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "");

  if(configFS.exists("/backup.etag")){
    File etagFile = configFS.open("/backup.etag", "r");
    if(etagFile){
      String etag = etagFile.readString();
      etagFile.close();
      response->addHeader("ETag", "\"" + etag + "\"");
    }
  }

  request->send(response);
}


/**
 * Handles Backup GETs
*/
void http_handleBackup_GET(AsyncWebServerRequest *request){

  if(request->hasHeader("visual-token")){
    if(!authToken.authenticate(request->header("visual-token").c_str())){
      http_unauthorized(request);
      return;
    }
  } else if(request->hasHeader("provisioning-token")){
    if(!authenticateWithProvisioningToken(request)){
      return;
    }
  } else {
    http_unauthorized(request);
    return;
  }

  if(!configFS.exists("/backup.json")){
    http_notFound(request);
    return;
  }

  resetHTPServerUsage();

  File f = configFS.open("/backup.json", "r");
  if(!f){
    http_error(request, "Unable to open backup file");
    return;
  }
  String plaintext = f.readString();
  f.close();

  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", plaintext);

  if(configFS.exists("/backup.etag")){
    File etagFile = configFS.open("/backup.etag", "r");
    if(etagFile){
      String etag = etagFile.readString();
      etagFile.close();
      response->addHeader("ETag", "\"" + etag + "\"");
    }
  }

  request->send(response);
}


static File _backupUploadFile;
static bool _backupUploadAuthorized = false;
static size_t _backupUploadBytesExpected = 0;
static size_t _backupUploadBytesWritten = 0;

/**
 * Body handler for streaming Backup PUTs directly to LittleFS
*/
void http_handleBackup_PUT_body(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){

  if(index == 0){
    _backupUploadAuthorized = false;
    _backupUploadBytesExpected = total;
    _backupUploadBytesWritten = 0;

    if(!request->hasHeader("visual-token") || !authToken.authenticate(request->header("visual-token").c_str())){
      return;
    }

    if(!deviceIdentity.enabled){
      return;
    }

    if(configFS.exists("/backup.json.upload_in_progress")){
      configFS.remove("/backup.json.upload_in_progress");
    }

    _backupUploadFile = configFS.open("/backup.json.upload_in_progress", "w");
    if(!_backupUploadFile){
      return;
    }

    _backupUploadAuthorized = true;
  }

  if(_backupUploadAuthorized && _backupUploadFile){
    _backupUploadFile.write(data, len);
    _backupUploadBytesWritten += len;
  }
}

/**
 * Handles Backup PUTs
*/
void http_handleBackup_PUT(AsyncWebServerRequest *request){

  if(!_backupUploadAuthorized){
    if(_backupUploadFile){
      _backupUploadFile.close();
      configFS.remove("/backup.json.upload_in_progress");
    }
    if(!request->hasHeader("visual-token") || !authToken.authenticate(request->header("visual-token").c_str())){
      http_unauthorized(request);
    }else{
      http_error(request, "Unable to open the file for writing");
    }
    return;
  }

  _backupUploadFile.close();

  if(_backupUploadBytesExpected > 0 && _backupUploadBytesWritten != _backupUploadBytesExpected){
    configFS.remove("/backup.json.upload_in_progress");
    http_error(request, "Incomplete transfer");
    return;
  }

  if(configFS.exists("/backup.json")){
    configFS.remove("/backup.json");
  }
  if(!configFS.rename("/backup.json.upload_in_progress", "/backup.json")){
    http_error(request, "Unable to finalize the file");
    return;
  }

  writeBackupEtag();

  resetHTPServerUsage();
  request->send(204);
}


/**
 * Handles Backup DELETEs
*/
void http_handleBackup_DELETE(AsyncWebServerRequest *request){

  if(!request->hasHeader("visual-token")){
    http_unauthorized(request);
    return;
  }

  if(!authToken.authenticate(request->header("visual-token").c_str())){
    http_unauthorized(request);
    return;
  }

  resetHTPServerUsage();

  if(!configFS.exists("/backup.json")){
    http_notFound(request);
    return;
  }

  if(configFS.remove("/backup.json")){
    configFS.remove("/backup.etag");
    request->send(204);
  }else{
    http_error(request, "Failed when trying to delete file");
  }
}


/**
 * Lists a given filesystem + path to an ArduinoJSON JsonArray
 */
void listDirToJsonArray(fs::FS &fs, const char *dirname, JsonArray &array) {

  File root = fs.open(dirname);
  if(!root) {
    return;
  }
  if(!root.isDirectory()) {
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if(file.isDirectory()) {
      listDirToJsonArray(fs, file.path(), array);
    } else {
      JsonObject entry = array.add<JsonObject>();
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

  if(request->method() == HTTP_OPTIONS){
    http_options(request);
    return;
  }

  if(!request->hasHeader("visual-token")){
        http_unauthorized(request);
        return;
  }

  if(!authToken.authenticate(request->header("visual-token").c_str())){
    http_unauthorized(request);
    return;
  }

  if(request->method() != HTTP_GET){
    http_methodNotAllowed(request);
    return;
  }

  resetHTPServerUsage();

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc(&spiRamAllocator);  //Supports approx 128 controllers, 128 clients, and 64 files in www
  JsonObject root = doc.to<JsonObject>();
  JsonObject configObject = root["config"].to<JsonObject>();
  JsonObject uiObject = root["ui"].to<JsonObject>();

  if(configFS_isMounted){
    configObject["total"] = configFS.totalBytes();
    configObject["used"] = configFS.usedBytes();
    JsonArray fileList = configObject["files"].to<JsonArray>();
    listDirToJsonArray(configFS, "/", fileList);
  }else{
    configObject["error"] = "File system not mounted";
  }

  if(uiFS_isMounted){
    uiObject["total"] = uiFS.totalBytes();
    uiObject["used"] = uiFS.usedBytes();
    JsonArray fileList = uiObject["files"].to<JsonArray>();
    listDirToJsonArray(uiFS, "/", fileList);
  }else{
    uiObject["error"] = "File system not mounted";
  }

  serializeJson(doc, *response);
  request->send(response);
}


/**
 * Handles /certs requests
*/
/**
 * Reads /certs/.cert_types, returning an empty object document if the file
 * does not exist or cannot be parsed.
 */
JsonDocument readCertTypes(){
  JsonDocument doc;
  String path = CONFIGFS_PATH_CERTS + (String)"/.cert_types";
  if(!configFS.exists(path)) return doc;
  File f = configFS.open(path, "r");
  if(!f) return doc;
  deserializeJson(doc, f);
  f.close();
  return doc;
}


/**
 * Serialises doc to /certs/.cert_types.
 */
void writeCertTypes(JsonDocument& doc){
  String path = CONFIGFS_PATH_CERTS + (String)"/.cert_types";
  File f = configFS.open(path, "w");
  if(!f) return;
  serializeJson(doc, f);
  f.close();
}


/**
 * Returns the SHA-256 fingerprint of a PEM certificate formatted as
 * colon-separated uppercase hex ("AA:BB:CC:…"), or an empty String on error.
 */
String computeCertFingerprint(const String& pem){
  int start = pem.indexOf("-----BEGIN CERTIFICATE-----");
  int end   = pem.indexOf("-----END CERTIFICATE-----");
  if(start == -1 || end == -1) return "";
  start += strlen("-----BEGIN CERTIFICATE-----");

  String b64 = pem.substring(start, end);
  b64.replace("\r", "");
  b64.replace("\n", "");
  b64.replace(" ",  "");

  size_t maxDerLen = (b64.length() / 4 + 1) * 3;
  uint8_t* der = (uint8_t*)malloc(maxDerLen);
  if(!der) return "";

  size_t outLen = 0;
  if(mbedtls_base64_decode(der, maxDerLen, &outLen,
      (const unsigned char*)b64.c_str(), b64.length()) != 0){
    free(der);
    return "";
  }

  uint8_t hash[32];
  mbedtls_sha256_context sha_ctx;
  mbedtls_sha256_init(&sha_ctx);
  mbedtls_sha256_starts(&sha_ctx, 0);
  mbedtls_sha256_update(&sha_ctx, der, outLen);
  mbedtls_sha256_finish(&sha_ctx, hash);
  mbedtls_sha256_free(&sha_ctx);
  free(der);

  String fp = "";
  char hex[3];
  for(int i = 0; i < 32; i++){
    if(i > 0) fp += ":";
    snprintf(hex, sizeof(hex), "%02X", hash[i]);
    fp += hex;
  }
  return fp;
}


/**
 * GET /api/provisioning/certs — returns the designated client CA certificate
 * (PEM + SHA-256 fingerprint) to a registered client device connecting via
 * the provisioning SoftAP.  No nonce required; the cert is not sensitive.
 */
static void http_sendProvisioningCert(AsyncWebServerRequest *request, const char* certType){

  JsonDocument certTypes = readCertTypes();
  String filename = "";
  for(JsonPair kv : certTypes.as<JsonObject>()){
    if(kv.value()[certType].as<bool>()){
      filename = kv.key().c_str();
      break;
    }
  }
  if(filename.isEmpty()){
    request->send(404);
    return;
  }

  String certPath = CONFIGFS_PATH_CERTS + (String)"/" + filename;
  if(!configFS.exists(certPath)){
    request->send(404);
    return;
  }

  File f = configFS.open(certPath, "r");
  if(!f){
    http_error(request, "Unable to read certificate");
    return;
  }
  String pem = "";
  while(f.available()) pem += (char)f.read();
  f.close();

  String fingerprint = computeCertFingerprint(pem);
  if(fingerprint.isEmpty()){
    http_error(request, "Unable to compute fingerprint");
    return;
  }

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc;
  doc["fingerprint"] = fingerprint;
  doc["pem"] = pem;
  serializeJson(doc, *response);
  request->send(response);
}


/**
 * GET /api/provisioning/certs/client — returns the designated client CA certificate
 */
void http_handleProvisioningCerts_client(AsyncWebServerRequest *request){

  if(request->method() == HTTP_OPTIONS){ http_options(request); return; }
  if(request->method() != HTTP_GET){ http_methodNotAllowed(request); return; }
  if(!authenticateWithProvisioningToken(request)){ return; }

  http_sendProvisioningCert(request, "client");
}


/**
 * GET /api/provisioning/certs/controller — returns the designated controller CA certificate
 */
void http_handleProvisioningCerts_controller(AsyncWebServerRequest *request){

  if(request->method() == HTTP_OPTIONS){ http_options(request); return; }
  if(request->method() != HTTP_GET){ http_methodNotAllowed(request); return; }
  if(!authenticateWithProvisioningToken(request)){ return; }

  http_sendProvisioningCert(request, "controller");
}


void http_handleCerts(AsyncWebServerRequest *request){

  switch(request->method()){

    case HTTP_OPTIONS:
      http_options(request);
      break;

    case HTTP_POST:

      if(request->_tempFile){
        const AsyncWebParameter* p = request->getParam("file", true, true);
        if(p){
          String uploadedFilename = p->value();
          String certTypeHeader = request->header("X-Cert-Type");
          bool wantsController = (certTypeHeader == "controller" || certTypeHeader == "both");
          bool wantsClient     = (certTypeHeader == "client"     || certTypeHeader == "both");

          request->_tempFile.close();

          JsonDocument certTypes = readCertTypes();
          certTypes[uploadedFilename]["controller"] = wantsController;
          certTypes[uploadedFilename]["client"]     = wantsClient;
          writeCertTypes(certTypes);

          refreshCertBundle();
          if(wantsClient)     mqtt_publishClientCertState();
          if(wantsController) mqtt_publishControllerCertState();

          request->send(201);
        } else {
          request->_tempFile.close();
          http_error(request, "Upload succeeded but filename could not be determined");
        }
      }
      // If _tempFile is null an error response was already sent by http_handleCerts_Upload
      break;

    case HTTP_GET:
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

  if(!request->hasHeader("visual-token")){
    http_unauthorized(request);
    return;
  }

  if(!authToken.authenticate(request->header("visual-token").c_str())){
    http_unauthorized(request);
    return;
  }
  
  resetHTPServerUsage();

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();

  JsonDocument certTypes = readCertTypes();

  File root = configFS.open(CONFIGFS_PATH_CERTS + (String)"/");

  File file = root.openNextFile();
  while(file){
      if(!file.isDirectory()){
          String name = (String)file.name();
          if(name != ".cert_types"){
            JsonObject fileInstance = array.add<JsonObject>();
            fileInstance["file"] = name;
            fileInstance["size"] = file.size();
            fileInstance["controller"] = certTypes[name]["controller"].as<bool>();
            fileInstance["client"]     = certTypes[name]["client"].as<bool>();
          }
      }
      file = root.openNextFile();
    }

  serializeJson(doc, *response);
  request->send(response);

}


/**
 * Handles certificate uploads.  Requires the X-Cert-Type header with value
 * "controller", "client", or "both".  Only one cert may have client: true
 * at a time; a second upload with client type returns 409.
*/
void http_handleCerts_Upload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){

  if(request->method() == HTTP_OPTIONS){
    http_options(request);
    return;
  }

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

  resetHTPServerUsage();

  MatchState ms;
  ms.Target((char*)filename.c_str());

  if(ms.MatchCount("[^a-z0-9_.]") > 0){
    http_badRequest(request, "Invalid filename; Only [a-z0-9_.] permitted");
    return;
  };

  if(!index){

    // Validate X-Cert-Type header
    if(!request->hasHeader("X-Cert-Type")){
      http_badRequest(request, "X-Cert-Type header required; valid values: controller, client, both");
      return;
    }
    String certTypeHeader = request->header("X-Cert-Type");
    if(certTypeHeader != "controller" && certTypeHeader != "client" && certTypeHeader != "both"){
      http_badRequest(request, "X-Cert-Type must be controller, client, or both");
      return;
    }

    // Enforce one-client rule
    bool wantsClient = (certTypeHeader == "client" || certTypeHeader == "both");
    if(wantsClient){
      JsonDocument certTypes = readCertTypes();
      for(JsonPair kv : certTypes.as<JsonObject>()){
        if(kv.value()["client"].as<bool>()){
          http_conflict(request, "A client certificate is already designated");
          return;
        }
      }
    }

    if(configFS.exists(CONFIGFS_PATH_CERTS + (String)"/" + filename)){
      http_forbiddenRequest(request, "Certificate already exists");
      return;
    }

    request->_tempFile = configFS.open(CONFIGFS_PATH_CERTS + (String)"/" + filename, "w");
    if(!request->_tempFile){
      http_error(request, "Unable to open file for writing");
      return;
    }
  }

  if(request->_tempFile) request->_tempFile.write(data, len);
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

  if(!request->hasHeader("visual-token")){
    http_unauthorized(request);
    return;
  }

  if(!authToken.authenticate(request->header("visual-token").c_str())){
    http_unauthorized(request);
    return;
  }

  resetHTPServerUsage();

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

  if(!request->hasHeader("visual-token")){
    http_unauthorized(request);
    return;
  }

  if(!authToken.authenticate(request->header("visual-token").c_str())){
    http_unauthorized(request);
    return;
  }

  resetHTPServerUsage();

  String filename = request->pathArg(0);

  if(!configFS.exists(CONFIGFS_PATH_CERTS + (String)"/" + filename)){
    request->send(404);
    return;
  }

  // Read flags before removing so we know which MQTT topics to update
  JsonDocument certTypes = readCertTypes();
  bool wasClient     = certTypes[filename]["client"].as<bool>();
  bool wasController = certTypes[filename]["controller"].as<bool>();

  configFS.remove(CONFIGFS_PATH_CERTS + (String)"/" + filename);

  certTypes.as<JsonObject>().remove(filename.c_str());
  writeCertTypes(certTypes);

  refreshCertBundle();
  if(wasClient)     mqtt_publishClientCertState();
  if(wasController) mqtt_publishControllerCertState();

  request->send(204);
}


/**
 * POST /api/ota — triggers an OTA update from the firmware version object provided.
 * Accepts the binaries array from a firmwareVersion payload; flashes all listed partitions.
*/
void http_handleOTA_POST(AsyncWebServerRequest *request, JsonVariant doc){

  if(!request->hasHeader("visual-token")){
      http_unauthorized(request);
      return;
  }

  if(!authToken.authenticate(request->header("visual-token").c_str(), true)){
    http_unauthorized(request);
    return;
  }

  if(request->method() != HTTP_POST){
    http_methodNotAllowed(request);
    return;
  }

  resetHTPServerUsage();

  if(_otaUpdateInProcess || _otaPendingRequest){
    http_conflict(request, "Update already in progress");
    return;
  }

  if(!doc["binaries"].is<JsonArray>() || doc["binaries"].as<JsonArray>().size() == 0){
    http_badRequest(request, "Field binaries is required");
    return;
  }

  _otaPendingDoc.set(doc);
  _otaPendingRequest = true;

  request->send(202);
}


/**
 * Builds the PSRAM-backed certificate bundle from built-in root CAs and any user-uploaded
 * certs in the /certs/ directory on configFS.  Call after configFS mount, cert upload,
 * and cert delete so the bundle stays current.
*/
void refreshCertBundle(){

  if(_certBundle != nullptr){
    free(_certBundle);
    _certBundle = nullptr;
  }

  _certBundleSize = 0;

  /* Concatenate certs marked controller: true in .cert_types */
  String bundle = "";
  if(configFS_isMounted){
    JsonDocument certTypes = readCertTypes();
    File certsDir = configFS.open(CONFIGFS_PATH_CERTS);
    if(certsDir && certsDir.isDirectory()){
      File certFile = certsDir.openNextFile();
      while(certFile){
        if(!certFile.isDirectory()){
          String name = (String)certFile.name();
          if(name != ".cert_types" && certTypes[name]["controller"].as<bool>()){
            while(certFile.available()){
              bundle += (char)certFile.read();
            }
          }
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
  if(_certBundle == nullptr){
    _certBundle = (char*)malloc(_certBundleSize + 1);
  }

  if(_certBundle == nullptr){
    log_e("Failed to allocate cert bundle (%u bytes)", (unsigned int)(_certBundleSize + 1));
    _certBundleSize = 0;
    return;
  }

  memcpy(_certBundle, bundle.c_str(), _certBundleSize + 1);

  log_i("Cert bundle built: %u bytes", (unsigned int)_certBundleSize);

  if(!_otaManifestUrl.isEmpty() && _otaManifestUrl.startsWith("https:")){
    if(_certBundle != nullptr){
      _otaHttpsClient.setCACert(_certBundle);
      otaFirmware.setClient(&_otaHttpsClient);
    } else {
      otaFirmware.useBundledCerts();
    }
  }
}


/**
 * Configures the OTA firmware settings
*/
void setup_OtaFirmware(){

  if(deviceIdentity.enabled == false){
    return;
  }

  if(configFS_isMounted == false){
    return;
  }

  String filename = CONFIGFS_PATH_CONTROLLERS + (String)"/" + deviceIdentity.data.uuid;

  if(!configFS.exists(filename)){
    return;
  }

  JsonDocument filter;
  filter["ota"] = true;

  String plaintext;
  if(!secretEncryption.decryptFromFile(configFS, filename, plaintext)){
    eventLog.createEvent("Config decrypt fail", EventLog::LOG_LEVEL_ERROR);
    log_e("Failed to decrypt %s", filename.c_str());
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, plaintext, DeserializationOption::Filter(filter));

  if(error) {
    char text[OLED_CHARACTERS_PER_LINE+1];
    snprintf(text, sizeof(text), "OTA parse err %s", error.c_str());
    eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
    return;
  }

  if(doc["ota"].isNull()){
    return;
  }

  if(doc["ota"]["url"].isNull()){
    eventLog.createEvent("OTA cfg no url");
    return;
  }

  String url = doc["ota"]["url"];

  uint8_t otaEthMac[6];
  esp_read_mac(otaEthMac, ESP_MAC_ETH);

  char otaMacOnly[13] = {0};
  sprintf(otaMacOnly, "%02X%02X%02X%02X%02X%02X", otaEthMac[0], otaEthMac[1], otaEthMac[2], otaEthMac[3], otaEthMac[4], otaEthMac[5]);

  char otaMacDashes[18] = {0};
  sprintf(otaMacDashes, "%02X-%02X-%02X-%02X-%02X-%02X", otaEthMac[0], otaEthMac[1], otaEthMac[2], otaEthMac[3], otaEthMac[4], otaEthMac[5]);

  char otaMacColons[18] = {0};
  sprintf(otaMacColons, "%02X:%02X:%02X:%02X:%02X:%02X", otaEthMac[0], otaEthMac[1], otaEthMac[2], otaEthMac[3], otaEthMac[4], otaEthMac[5]);

  char otaProductHex[11] = {0};
  snprintf(otaProductHex, sizeof(otaProductHex), "0x%08lx", (uint32_t)PRODUCT_HEX);

  url.replace("$$mac$$", otaMacOnly);
  url.replace("$$mac_dashes$$", otaMacDashes);
  url.replace("$$mac_colons$$", otaMacColons);
  url.replace("$$uuid$$", deviceIdentity.data.uuid);

  String otaApplication = APPLICATION;
  otaApplication.toLowerCase();
  otaApplication.replace(" ", "-");

  url.replace("$$class$$", HARDWARE_CLASS);
  url.replace("$$application$$", otaApplication.c_str());
  url.replace("$$product_hex$$", otaProductHex);
  url.replace("$$current_version$$", VERSION);

  if(!url.startsWith("http:") && !url.startsWith("https:")){
    eventLog.createEvent("OTA cfg inv proto");
    return;
  }

  otaFirmware.setCurrentVersion(VERSION);
  otaFirmware.setApplicationName(APPLICATION_NAME);
  otaFirmware.setManifestURL(url.c_str());
  otaFirmware.setBlockedPartitions({"config"});
  otaFirmware.addHeader("uuid", deviceIdentity.data.uuid);

  if(url.startsWith("https:")){
    if(_certBundle != nullptr){
      _otaHttpsClient.setCACert(_certBundle);
      otaFirmware.setClient(&_otaHttpsClient);
    } else {
      otaFirmware.useBundledCerts();
    }
  }

  otaFirmware.onProgress([](const char* partition, size_t written, size_t total){
    _otaUpdateInProcess = true;
    if(strcmp(partition, _otaCurrentPartition) != 0){
      strlcpy(_otaCurrentPartition, partition, sizeof(_otaCurrentPartition));
      oled.setOTAPartition(partition);
      oled.setPage(managerOled::PAGE_OTA_IN_PROGRESS);
      char msg[OLED_CHARACTERS_PER_LINE+1];
      snprintf(msg, sizeof(msg), "OTA %s update start", partition);
      eventLog.createEvent(msg, EventLog::LOG_LEVEL_INFO);
    }
    oled.setProgressBar((float)written / (float)total);
    int pct = (int)((float)written / (float)total * 100);
    if(pct != _otaLastPublishedPercentage && deviceIdentity.enabled && mqttClient.connected()){
      _otaLastPublishedPercentage = pct;
      JsonDocument mqttDoc;
      mqttDoc["installed_version"] = VERSION;
      mqttDoc["latest_version"] = _otaLatestVersion;
      if(strlen(_otaReleaseUrl) > 0){ mqttDoc["release_url"] = _otaReleaseUrl; }
      mqttDoc["in_progress"] = true;
      mqttDoc["update_percentage"] = pct;
      char topic[MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH+1];
      snprintf(topic, sizeof(topic), MQTT_TOPIC_UPDATE_STATE_PATTERN, deviceIdentity.data.uuid);
      char buffer[384];
      serializeJson(mqttDoc, buffer, sizeof(buffer));
      mqttClient.publish(topic, buffer);
    }
    mqttClient.loop();
  });

  otaFirmware.onPartitionComplete([](const char* partition, bool success){
    char msg[OLED_CHARACTERS_PER_LINE+1];
    snprintf(msg, sizeof(msg), "OTA %s %s", partition, success ? "finished" : "failed");
    eventLog.createEvent(msg, success ? EventLog::LOG_LEVEL_INFO : EventLog::LOG_LEVEL_NOTIFICATION);
  });

  otaFirmware.onComplete([](bool success){
    _otaUpdateInProcess = false;
    _otaPendingRequest = false;

    if(mqttClient.connected()){
      char topic[MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH+1];
      snprintf(topic, sizeof(topic), MQTT_TOPIC_UPDATE_STATE_PATTERN, deviceIdentity.data.uuid);
      JsonDocument mqttDoc;
      mqttDoc["in_progress"] = false;
      char buffer[256];
      serializeJson(mqttDoc, buffer);
      mqttClient.publish(topic, buffer);
    }

    if(success){
      eventLog.createEvent("Rebooting...", EventLog::LOG_LEVEL_NOTIFICATION);
      delay(5000);
      ESP.restart();
    }
  });

  otaFirmware.onAvailable([](const char* version, const char* app, const char* releaseUrl){
    strlcpy(_otaLatestVersion, version, sizeof(_otaLatestVersion));
    strlcpy(_otaReleaseUrl, releaseUrl ? releaseUrl : "", sizeof(_otaReleaseUrl));
    _otaLastPublishedPercentage = -1;
    if(deviceIdentity.enabled == false){ return; }
    if(!mqttClient.connected()){ return; }
    eventLog.createEvent("OTA update available");

    JsonDocument mqttDoc;
    mqttDoc["installed_version"] = VERSION;
    mqttDoc["latest_version"] = version;
    if(releaseUrl && strlen(releaseUrl) > 0){
      mqttDoc["release_url"] = releaseUrl;
    }
    mqttDoc["in_progress"] = false;

    char topic[MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH+1];
    snprintf(topic, sizeof(topic), MQTT_TOPIC_UPDATE_STATE_PATTERN, deviceIdentity.data.uuid);
    mqttClient.beginPublish(topic, measureJson(mqttDoc), true);
    BufferingPrint bufferedClient(mqttClient, 32);
    serializeJson(mqttDoc, bufferedClient);
    bufferedClient.flush();
    mqttClient.endPublish();
  });

  otaFirmware.onError([](const char* partition, int err){
    _otaCheckFailed = true;
    _otaCheckErrorCode = err;
    log_e("OTA error on %s: %d", partition, err);
    if(deviceIdentity.enabled == false){ return; }
    if(!mqttClient.connected()){ return; }
    char availability_topic[MQTT_TOPIC_UPDATE_AVAILABILITY_LENGTH+1];
    snprintf(availability_topic, sizeof(availability_topic), MQTT_TOPIC_UPDATE_AVAILABILITY_PATTERN, deviceIdentity.data.uuid);
    mqttClient.publish(availability_topic, "offline");
  });

  _otaManifestUrl = url;
  eventLog.createEvent("OTA update enabled");
}


/**
 * Executes a pending forced OTA update using execOTA(JsonDocument&), matching
 * the HW-Reg implementation exactly. onComplete handles the reboot on success.
*/
void otaFirmware_checkPending(){

  if(!_otaPendingRequest || _otaUpdateInProcess){
    return;
  }

  _otaUpdateInProcess = true;

  otaFirmware.setApplicationName(_otaPendingDoc["application_name"]);

  for(JsonVariant bin : _otaPendingDoc["binaries"].as<JsonArray>()){
    if(String(bin["url"] | "").startsWith("https:")){
      if(_certBundle != nullptr){
        _otaHttpsClient.setCACert(_certBundle);
        otaFirmware.setClient(&_otaHttpsClient);
      } else {
        otaFirmware.useBundledCerts();
      }
      break;
    }
  }

  _otaCurrentPartition[0] = '\0';

  otaFirmware.onProgress([](const char* partition, size_t written, size_t total){
    _otaUpdateInProcess = true;
    if(strcmp(partition, _otaCurrentPartition) != 0){
      strlcpy(_otaCurrentPartition, partition, sizeof(_otaCurrentPartition));
      oled.setOTAPartition(partition);
      oled.setPage(managerOled::PAGE_OTA_IN_PROGRESS);
      char msg[OLED_CHARACTERS_PER_LINE+1];
      snprintf(msg, sizeof(msg), "OTA %s update start", partition);
      eventLog.createEvent(msg, EventLog::LOG_LEVEL_INFO);
    }
    oled.setProgressBar((float)written / (float)total);
    int pct = (int)((float)written / (float)total * 100);
    if(pct != _otaLastPublishedPercentage && deviceIdentity.enabled && mqttClient.connected()){
      _otaLastPublishedPercentage = pct;
      JsonDocument mqttDoc;
      mqttDoc["installed_version"] = VERSION;
      mqttDoc["latest_version"] = _otaLatestVersion;
      if(strlen(_otaReleaseUrl) > 0){ mqttDoc["release_url"] = _otaReleaseUrl; }
      mqttDoc["in_progress"] = true;
      mqttDoc["update_percentage"] = pct;
      char topic[MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH+1];
      snprintf(topic, sizeof(topic), MQTT_TOPIC_UPDATE_STATE_PATTERN, deviceIdentity.data.uuid);
      char buffer[384];
      serializeJson(mqttDoc, buffer, sizeof(buffer));
      mqttClient.publish(topic, buffer);
    }
    mqttClient.loop();
  });

  otaFirmware.onPartitionComplete([](const char* partition, bool success){
    char msg[OLED_CHARACTERS_PER_LINE+1];
    snprintf(msg, sizeof(msg), "OTA %s %s", partition, success ? "finished" : "failed");
    eventLog.createEvent(msg, success ? EventLog::LOG_LEVEL_INFO : EventLog::LOG_LEVEL_NOTIFICATION);
  });

  otaFirmware.onComplete([](bool success){
    _otaUpdateInProcess = false;
    _otaPendingRequest = false;

    if(mqttClient.connected()){
      char topic[MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH+1];
      snprintf(topic, sizeof(topic), MQTT_TOPIC_UPDATE_STATE_PATTERN, deviceIdentity.data.uuid);
      JsonDocument mqttDoc;
      mqttDoc["in_progress"] = false;
      char buffer[256];
      serializeJson(mqttDoc, buffer);
      mqttClient.publish(topic, buffer);
    }

    if(success){
      eventLog.createEvent("Rebooting...", EventLog::LOG_LEVEL_NOTIFICATION);
      delay(5000);
      ESP.restart();
    }
  });

  eventLog.createEvent("OTA update available");
  const char* targetVersion = _otaPendingDoc["version"] | VERSION;
  const char* releaseUrl = _otaPendingDoc["release_url"] | "";
  strlcpy(_otaLatestVersion, targetVersion, sizeof(_otaLatestVersion));
  strlcpy(_otaReleaseUrl, releaseUrl, sizeof(_otaReleaseUrl));
  _otaLastPublishedPercentage = -1;
  if(deviceIdentity.enabled && mqttClient.connected()){
    JsonDocument mqttDoc;
    mqttDoc["installed_version"] = VERSION;
    mqttDoc["latest_version"] = targetVersion;
    if(releaseUrl && strlen(releaseUrl) > 0){
      mqttDoc["release_url"] = releaseUrl;
    }
    mqttDoc["in_progress"] = true;
    char topic[MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH+1];
    snprintf(topic, sizeof(topic), MQTT_TOPIC_UPDATE_STATE_PATTERN, deviceIdentity.data.uuid);
    mqttClient.beginPublish(topic, measureJson(mqttDoc), true);
    BufferingPrint bufferedClient(mqttClient, 32);
    serializeJson(mqttDoc, bufferedClient);
    bufferedClient.flush();
    mqttClient.endPublish();
  }

  otaFirmware.execOTA(_otaPendingDoc);

  /* Only reached on failure — onComplete handles restart on success */
  _otaPendingDoc.clear();
  _otaUpdateInProcess = false;
  _otaPendingRequest = false;
}


/***
 * Configures the I/O
 */
void setupIO(){

  bool isOK = true;

  if(deviceIdentity.enabled == false){
    eventLog.createEvent("No I/O setup (eFuse)", EventLog::LOG_LEVEL_ERROR);
    return;
  }

  if(configFS_isMounted == false){
    eventLog.createEvent("No I/O ConfigFS offline", EventLog::LOG_LEVEL_ERROR);
    return;
  }

  String filename = CONFIGFS_PATH_CONTROLLERS + (String)"/" + deviceIdentity.data.uuid;

  if(!configFS.exists(filename)){
    eventLog.createEvent("No I/O file to read");
    return;
  }

  if(!setup_outputs(filename)){
    isOK = false;
  }

  if(!setup_inputs(filename)){
    isOK = false;
  };

  if(isOK){
      eventLog.createEvent("I/O setup read OK");
  }else{
    eventLog.createEvent("I/O setup read fail");
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
  JsonDocument filter;

  JsonObject filter_outputs__ = filter["outputs"]["*"].to<JsonObject>();
  filter_outputs__["id"] = true;
  filter_outputs__["type"] = true;
  filter_outputs__["enabled"] = true;
  filter_outputs__["start_brightness"] = true;

  String plaintext;
  if(!secretEncryption.decryptFromFile(configFS, filename, plaintext)){
    eventLog.createEvent("Config decrypt fail", EventLog::LOG_LEVEL_ERROR);
    log_e("Failed to decrypt %s", filename.c_str());
    return false;
  }

  JsonDocument doc; //Supports up to 32 ports
  DeserializationError error = deserializeJson(doc, plaintext, DeserializationOption::Filter(filter));

  if(error) {
    char text[OLED_CHARACTERS_PER_LINE+1];
    snprintf(text, sizeof(text), "Out parse err %s", error.c_str());
    eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
    return false;
  }

  for (JsonPair output : doc["outputs"].as<JsonObject>()) {

    int8_t outputPortNumber = atoi(output.key().c_str());

    if(outputPortNumber > (OUTPUT_CONTROLLER_COUNT * OUTPUT_CONTROLLER_COUNT_PINS)){
      char text[OLED_CHARACTERS_PER_LINE+1];
      snprintf(text, sizeof(text), "Out prt %s > max", output.key().c_str());
      eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
      isOK = false;
      continue;
    }

    if(outputPortNumber < 1){
      char text[OLED_CHARACTERS_PER_LINE+1];
      snprintf(text, sizeof(text), "Out prt %s < 1", output.key().c_str());
      eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
      isOK = false;
      continue;
    }

    if(output.value()["id"].isNull()){
      char text[OLED_CHARACTERS_PER_LINE+1];
      snprintf(text, sizeof(text), "Out prt %s no id", output.key().c_str());
      eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
      isOK = false;
      continue;
    }

    outputs.setPortId(outputPortNumber, output.value()["id"]);

    if(!output.value()["type"].isNull()){
      if(strcmp(output.value()["type"], "VARIABLE") == 0){
        outputs.setPortType(outputPortNumber, nsOutputs::outputPin::VARIABLE);
      }
    }

    if(!output.value()["enabled"].isNull()){
      outputs.enablePort(outputPortNumber, output.value()["enabled"].as<boolean>());
    }

    if(!output.value()["start_brightness"].isNull()){
      outputs.setPortStartBrightness(outputPortNumber, output.value()["start_brightness"].as<uint8_t>());
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
  JsonDocument filter;

  JsonObject filter_ports = filter["ports"]["*"].to<JsonObject>();
  filter_ports["id"] = true;
  filter_ports["clientUUID"] = true;

  JsonObject filter_ports_channels = filter_ports["channels"]["*"].to<JsonObject>();
  filter_ports_channels["type"] = true;
  filter_ports_channels["enabled"] = true;
  filter_ports_channels["offset"] = true;
  filter_ports_channels["actions"] = true;

  String plaintext;
  if(!secretEncryption.decryptFromFile(configFS, filename, plaintext)){
    eventLog.createEvent("Config decrypt fail", EventLog::LOG_LEVEL_ERROR);
    log_e("Failed to decrypt %s", filename.c_str());
    return false;
  }

  JsonDocument doc(&spiRamAllocator); //Supports up to 32 ports
  DeserializationError error = deserializeJson(doc, plaintext, DeserializationOption::Filter(filter));

  if(error) {
    char text[OLED_CHARACTERS_PER_LINE+1];
    snprintf(text, sizeof(text), "In parse err %s", error.c_str());
    eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
    return false;
  }

  for (JsonPair port : doc["ports"].as<JsonObject>()) {

    if(atoi(port.key().c_str()) > (IO_EXTENDER_COUNT_PINS / IO_EXTENDER_COUNT_CHANNELS_PER_PORT) * IO_EXTENDER_COUNT){
      char text[OLED_CHARACTERS_PER_LINE+1];
      snprintf(text, sizeof(text), "In prt %s > max", port.key().c_str());
      eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
      isOK = false;
      continue;
    }

    if(atoi(port.key().c_str()) < 1){
      char text[OLED_CHARACTERS_PER_LINE+1];
      snprintf(text, sizeof(text), "In prt %s < 1", port.key().c_str());
      eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
      isOK = false;
      continue;
    }

    strlcpy(inputPorts[atoi(port.key().c_str())-1].id, port.value()["id"], sizeof(inputPorts[atoi(port.key().c_str())-1].id));
    strlcpy(inputPorts[atoi(port.key().c_str())-1].clientUUID, port.value()["clientUUID"] | "", sizeof(inputPorts[atoi(port.key().c_str())-1].clientUUID));

    uint8_t i = 0;

    JsonObject channels = port.value()["channels"].as<JsonObject>();
    for (JsonPair port_value_channel : channels){

      if(i > IO_EXTENDER_COUNT_CHANNELS_PER_PORT){
        char text[OLED_CHARACTERS_PER_LINE+1];
        snprintf(text, sizeof(text), "In prt %s ch > max", port.key().c_str());
        eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
        isOK = false;
        continue;
      }

      managerInputs::portChannel portChannel;

      portChannel.port = atoi(port.key().c_str());
      portChannel.channel = atoi(port_value_channel.key().c_str());
      inputPorts[atoi(port.key().c_str())-1].channels[i].channel = portChannel.channel;

      if(port_value_channel.value()["type"]){
        if(port_value_channel.value()["type"] != "NORMALLY_OPEN"){
          inputs.setPortChannelInputType(portChannel, managerInputs::NORMALLY_CLOSED);
        }
      }

      if(port_value_channel.value()["enabled"]){
        inputs.enablePortChannel(portChannel, port_value_channel.value()["enabled"].as<boolean>());
      }

      if(port_value_channel.value()["offset"]){
        inputs.setOffset(portChannel, port_value_channel.value()["offset"].as<uint8_t>());
      }

      JsonArray actions = port_value_channel.value()["actions"].as<JsonArray>();
      for (JsonObject port_value_channel_value_action : actions) {

        bool actionIsOK = false;

        inputAction newInputAction;

        if(!port_value_channel_value_action["action"].isNull()){
        
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

        if(port_value_channel_value_action["change_state"].isNull()){
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
          char text[OLED_CHARACTERS_PER_LINE+1];
          snprintf(text, sizeof(text), "In prt %s ch %s inv act", port.key().c_str(), port_value_channel.key().c_str());
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
void mqtt_onDisconnect(){
  char text[OLED_CHARACTERS_PER_LINE+1];

  switch(mqttClient.state()){
    case -4: snprintf(text, sizeof(text), "MQTT conn timeout");    break;
    case -3: snprintf(text, sizeof(text), "MQTT conn lost");       break;
    case -2: snprintf(text, sizeof(text), "MQTT conn fail");       break;
    case -1: snprintf(text, sizeof(text), "MQTT conn disconnect"); break;
    case  1: snprintf(text, sizeof(text), "MQTT bad protocol");    break;
    case  2: snprintf(text, sizeof(text), "MQTT bad client ID");   break;
    case  3: snprintf(text, sizeof(text), "MQTT unavailable");     break;
    case  4: snprintf(text, sizeof(text), "MQTT bad creds");       break;
    case  5: snprintf(text, sizeof(text), "MQTT unauthorized");    break;
    default: snprintf(text, sizeof(text), "Unknown MQTT state");   break;
  }

  eventLog.createEvent(text);
  eventLog.createEvent("MQTT disconnected", EventLog::LOG_LEVEL_ERROR);
  _mqttWasConnected = false;
}


void mqtt_reconnect(){

  if(!mqttClient.enabled){
    return;
  }

  if(deviceIdentity.enabled == false){
    return;
  }

  if((esp_timer_get_time() - mqttClient.lastReconnectAttemptTime >= (uint64_t)MQTT_RECONNECT_WAIT_MILLISECONDS * 1000ULL) || mqttClient.lastReconnectAttemptTime == 0){

    if(mqttClient.connect(deviceIdentity.data.uuid, mqttClient.username, mqttClient.password, mqttClient.topic_availability, 2, true, "offline")) {
        mqttClient.lastReconnectAttemptTime = 0;
        _mqttWasConnected = true;
        log_d("MQTT connected at uptime=%llu s; httpServerIsActive=%d lastTimeHttpServerUsed=%lu", esp_timer_get_time() / 1000000ULL, httpServerIsActive, lastTimeHttpServerUsed);
        eventLog.createEvent("MQTT connected");
        eventLog.resolveError("MQTT disconnected");
        mqttClient.publish(mqttClient.topic_availability, "online", true);
        char _httpServerCommandTopic[MQTT_TOPIC_HTTP_SERVER_SET_PATTERN_LENGTH+1];
        snprintf(_httpServerCommandTopic, sizeof(_httpServerCommandTopic), MQTT_TOPIC_HTTP_SERVER_SET_PATTERN, deviceIdentity.data.uuid);
        mqttClient.publish(_httpServerCommandTopic, httpServerIsActive ? "ON" : "OFF", true);
        mqttClient.resubscribe();
        if(!mqttClient.autoDiscovery.sent){
          mqtt_autoDiscovery_update();
          mqtt_autoDiscovery_temperature();
          mqtt_autoDiscovery_outputs();
          mqtt_autoDiscovery_inputs();
          mqtt_autoDiscovery_inputControllers();
          mqtt_autoDiscovery_outputControllers();
          mqtt_autoDiscovery_start_time();
          mqtt_autoDiscovery_ip_address();
          mqtt_autoDiscovery_mac_address();
          mqtt_autoDiscovery_count_errors();
          mqtt_autoDiscovery_http_server();
          mqtt_autoDiscovery_heapFree();
          mqtt_autoDiscovery_heapLargestFreeBlock();
          mqttClient.autoDiscovery.sent = true;
        }
        mqtt_publishAllAvailability();
        mqtt_publishTemperatures();
        mqtt_publishStartTime();
        mqtt_publishIPAddress();
        mqtt_publishMACAddress();
        mqtt_publishCountErrors();
        mqtt_publishHttpServerStateChanged(httpServerIsActive);
        mqtt_publishClientCertState();
        mqtt_publishControllerCertState();
        return;
    }

    mqttClient.lastReconnectAttemptTime = esp_timer_get_time();
  }
}


/***
 * Sets up the MQTT client for use
 */
void setupMQTT(){

  if(deviceIdentity.enabled == false){
    return;
  }

  char topic_availability[sizeof(mqttClient.topic_availability)+1];
  snprintf(topic_availability, sizeof(topic_availability), MQTT_TOPIC_CONTROLLER_AVAILABILITY_PATTERN, deviceIdentity.data.uuid);
  strcpy(mqttClient.topic_availability, topic_availability);
  mqttClient.setCallback(eventHandler_mqttMessageReceived);

  String filename = CONFIGFS_PATH_CONTROLLERS + (String)"/" + deviceIdentity.data.uuid;

  if(!configFS.exists(filename)){
    return;
  }

  JsonDocument filter;
  filter["name"] = true;
  filter["area"] = true;
  filter["mqtt"] = true;

  String plaintext;
  if(!secretEncryption.decryptFromFile(configFS, filename, plaintext)){
    eventLog.createEvent("Config decrypt fail", EventLog::LOG_LEVEL_ERROR);
    log_e("Failed to decrypt %s", filename.c_str());
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, plaintext, DeserializationOption::Filter(filter));

  if(error) {
    char text[OLED_CHARACTERS_PER_LINE+1];
    snprintf(text, sizeof(text), "MQTT parse err %s", error.c_str());
    eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
    return;
  }

  if(!doc["name"].isNull()){
    mqttClient.autoDiscovery.setDeviceName(doc["name"].as<String>().c_str());
  }

  if(!doc["area"].isNull()){
    mqttClient.autoDiscovery.setSuggestedArea(doc["area"].as<String>().c_str());
  }

  if(doc["mqtt"].isNull()){
    char text[OLED_CHARACTERS_PER_LINE+1];
    snprintf(text, sizeof(text), "MQTT obj missing");
    eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
    return;
  }

  JsonObject mqtt = doc["mqtt"];
  uint16_t port = 1883;

  if(mqtt["host"].isNull()){
    char text[OLED_CHARACTERS_PER_LINE+1];
    snprintf(text, sizeof(text), "MQTT host missing");
    eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
    return;
  }

  if(mqtt["username"].isNull()){
    char text[OLED_CHARACTERS_PER_LINE+1];
    snprintf(text, sizeof(text), "MQTT user missing");
    eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
    return;
  }

  if(mqtt["password"].isNull()){
    char text[OLED_CHARACTERS_PER_LINE+1];
    snprintf(text, sizeof(text), "MQTT pass missing");
    eventLog.createEvent(text, EventLog::LOG_LEVEL_ERROR);
    return;
  }

  if(!mqtt["port"].isNull()){
    port = mqtt["port"];
  }

  uint8_t ethMac[6];
  esp_read_mac(ethMac, ESP_MAC_ETH);

  char macOnly[13] = {0};
  sprintf(macOnly, "%02X%02X%02X%02X%02X%02X", ethMac[0],ethMac[1],ethMac[2],ethMac[3], ethMac[4], ethMac[5]);

  char macDashes[18] = {0};
  sprintf(macDashes, "%02X-%02X-%02X-%02X-%02X-%02X", ethMac[0],ethMac[1],ethMac[2],ethMac[3], ethMac[4], ethMac[5]);

  char macColons[18] = {0};
  sprintf(macColons, "%02X:%02X:%02X:%02X:%02X:%02X", ethMac[0],ethMac[1],ethMac[2],ethMac[3], ethMac[4], ethMac[5]);

  char mqttProductHex[11] = {0};
  snprintf(mqttProductHex, sizeof(mqttProductHex), "0x%08lx", (uint32_t)PRODUCT_HEX);

  String mqttApplication = APPLICATION;
  mqttApplication.toLowerCase();
  mqttApplication.replace(" ", "-");

  String host = mqtt["host"].as<String>();
  host.replace("$$mac$$", macOnly);
  host.replace("$$mac_dashes$$", macDashes);
  host.replace("$$mac_colons$$", macColons);
  if(deviceIdentity.enabled == true){
    host.replace("$$uuid$$", deviceIdentity.data.uuid);
  }
  host.replace("$$class$$", HARDWARE_CLASS);
  host.replace("$$application$$", mqttApplication.c_str());
  host.replace("$$product_hex$$", mqttProductHex);
  host.replace("$$current_version$$", VERSION);
  mqttClient.setServer(host.c_str(), port);

  String username = mqtt["username"].as<String>();
  username.replace("$$mac$$", macOnly);
  username.replace("$$mac_dashes$$", macDashes);
  username.replace("$$mac_colons$$", macColons);
  if(deviceIdentity.enabled == true){
    username.replace("$$uuid$$", deviceIdentity.data.uuid);
  }
  username.replace("$$class$$", HARDWARE_CLASS);
  username.replace("$$application$$", mqttApplication.c_str());
  username.replace("$$product_hex$$", mqttProductHex);
  username.replace("$$current_version$$", VERSION);
  mqttClient.setUsername(username.c_str());

  String password = mqtt["password"].as<String>();
  password.replace("$$mac$$", macOnly);
  password.replace("$$mac_dashes$$", macDashes);
  password.replace("$$mac_colons$$", macColons);
  password.replace("$$uuid$$", deviceIdentity.data.uuid);
  password.replace("$$class$$", HARDWARE_CLASS);
  password.replace("$$application$$", mqttApplication.c_str());
  password.replace("$$product_hex$$", mqttProductHex);
  password.replace("$$current_version$$", VERSION);
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

  if(payload.isEmpty()){
    return;
  }

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

    char topic[MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH+1];
    snprintf(topic, sizeof(topic), MQTT_TOPIC_UPDATE_STATE_PATTERN, deviceIdentity.data.uuid);

    JsonDocument doc;
    doc["in_progress"] = true;
  
    char buffer[256];
    serializeJson(doc, buffer);
  
    mqttClient.publish(topic, buffer);

    if(!_otaManifestUrl.isEmpty()){
      otaFirmware.execOTA();
    }
    return;
  }

  if(ms.Match(MQTT_TOPIC_HTTP_SERVER_SET_REGEX)){ //User wants to enable or disable the HTTP server

    payload.toUpperCase();
    log_d("MQTT: http-server/set received '%s' (httpServerIsActive=%d, uptime=%llu s)", payload.c_str(), httpServerIsActive, esp_timer_get_time() / 1000000ULL);

    if(payload == "ON"){
      if(httpServerIsActive){ log_d("MQTT: http-server/set ON ignored, server already active"); return; }
      startHttpServer();
      return;
    }

    if(payload == "OFF"){
      if(!httpServerIsActive){ log_d("MQTT: http-server/set OFF ignored, server already inactive"); return; }
      stopHttpServer();
      return;
    }
  }

  log_w("MQTT subscribed topic [%s] received payload [%s] but was unhandled.", topic, payload);
}


/**
 * Handles MQTT connection events
 */
/**
 * Handles temperature sensor auto discovery broadcasts
 */
void mqtt_autoDiscovery_temperature(){

  if(deviceIdentity.enabled == false){
    return;
  }

  for(int i = 0; i < TEMPERATURE_SENSOR_COUNT; i++){

    const char* sensorLocation = temperatureSensors.getSensorLocation(i);

    JsonDocument doc;

    char topic[MQTT_TOPIC_TEMPERATURE_AUTO_DISCOVERY_LENGTH+1];
    snprintf(topic, sizeof(topic), MQTT_TOPIC_TEMPERATURE_AUTO_DISCOVERY_PATTERN, mqttClient.autoDiscovery.homeAssistantRoot, deviceIdentity.data.uuid, sensorLocation);

    char unique_id[MQTT_TEMPERATURE_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1];
    snprintf(unique_id, sizeof(unique_id), MQTT_TEMPERATURE_AUTO_DISCOVERY_UNIQUE_ID_PATTERN, deviceIdentity.data.uuid, sensorLocation);

    char default_entity_id[MQTT_TEMPERATURE_DEFAULT_ENTITY_ID_LENGTH+1];
    snprintf(default_entity_id, sizeof(default_entity_id), MQTT_TEMPERATURE_DEFAULT_ENTITY_ID_PATTERN, deviceIdentity.data.uuid, sensorLocation);

    char state_topic[MQTT_TOPIC_TEMPERATURE_STATE_PATTERN_LENGTH+1];
    snprintf(state_topic, sizeof(state_topic), MQTT_TOPIC_TEMPERATURE_STATE_PATTERN, deviceIdentity.data.uuid, sensorLocation);

    doc["name"] = sensorLocation;
    doc["unique_id"] = unique_id;
    doc["default_entity_id"] = default_entity_id;
    doc["icon"] = "mdi:thermometer";
    doc["device_class"] = "temperature";
    doc["unit_of_measurement"] = "°C";
    doc["state_class"] = "measurement";

    JsonObject device = doc["device"].to<JsonObject>();
    JsonArray identifiers = device["identifiers"].to<JsonArray>();
    identifiers.add(deviceIdentity.data.uuid);

    if(strlen(mqttClient.autoDiscovery.deviceName) > 0){
      device["name"] =  mqttClient.autoDiscovery.deviceName;
    }

    device["manufacturer"] = HARDWARE_MANUFACTURER_NAME;
    device["model"] = APPLICATION_NAME;
    device["model_id"] = deviceIdentity.data.product_id;
    device["serial_number"] = deviceIdentity.data.uuid;
    device["sw_version"] = VERSION " (" COMMIT_HASH ")";
    device["configuration_url"] = ("http://" + ETH.localIP().toString()).c_str();

    if(strlen(mqttClient.autoDiscovery.suggestedArea) > 0){
      device["suggested_area"] =  mqttClient.autoDiscovery.suggestedArea;
    }

    doc["state_topic"] = state_topic;

    char availability_topic[MQTT_TOPIC_TEMPERATURE_AVAILABILITY_LENGTH+1];
    snprintf(availability_topic, sizeof(availability_topic), MQTT_TOPIC_TEMPERATURE_AVAILABILITY_PATTERN, deviceIdentity.data.uuid, sensorLocation);

    JsonArray availability = doc["availability"].to<JsonArray>();
    JsonObject sensor_specific = availability.add<JsonObject>();
    JsonObject controller_level = availability.add<JsonObject>();
    sensor_specific["topic"] = availability_topic;
    controller_level["topic"] = mqttClient.topic_availability;
    doc["availability_mode"] = "all";

    mqttClient.beginPublish(topic, measureJson(doc), true);
    BufferingPrint bufferedClient(mqttClient, 32);
    serializeJson(doc, bufferedClient);
    bufferedClient.flush();
    mqttClient.endPublish();
  }
}


/**
 * Publishes the per-sensor MQTT availability for all temperature sensors based on current health
 */
void mqtt_publishTemperatureAvailability(){

  if(deviceIdentity.enabled == false){
    return;
  }

  if(!mqttClient.connected()){
    return;
  }

  managerTemperatureSensors::healthResult health = temperatureSensors.health();

  for(int i = 0; i < health.count; i++){

    char availability_topic[MQTT_TOPIC_TEMPERATURE_AVAILABILITY_LENGTH+1];
    snprintf(availability_topic, sizeof(availability_topic), MQTT_TOPIC_TEMPERATURE_AVAILABILITY_PATTERN, deviceIdentity.data.uuid, temperatureSensors.getSensorLocation(i));

    mqttClient.publish(availability_topic, health.sensor[i].enabled ? "online" : "offline", true);
  }
}


/**
 * Publishes the per-chip MQTT availability for all output controllers based on current health
 */
void mqtt_publishOutputControllerAvailability(){

  if(deviceIdentity.enabled == false){
    return;
  }

  if(!mqttClient.connected()){
    return;
  }

  nsOutputs::managerOutputs::healthResult health = outputs.health();

  for(int i = 0; i < health.count; i++){

    char availability_topic[MQTT_TOPIC_OUTPUT_CONTROLLER_AVAILABILITY_LENGTH+1];
    snprintf(availability_topic, sizeof(availability_topic), MQTT_TOPIC_OUTPUT_CONTROLLER_AVAILABILITY_PATTERN, deviceIdentity.data.uuid, health.outputControllers[i].address);

    mqttClient.publish(availability_topic, health.outputControllers[i].enabled ? "online" : "offline", true);
  }
}


/**
 * Publishes the per-chip MQTT availability for all input controllers based on current health
 */
void mqtt_publishInputControllerAvailability(){

  if(deviceIdentity.enabled == false){
    return;
  }

  if(!mqttClient.connected()){
    return;
  }

  managerInputs::healthResult health = inputs.health();

  for(int i = 0; i < health.count; i++){

    char availability_topic[MQTT_TOPIC_INPUT_CONTROLLER_AVAILABILITY_LENGTH+1];
    snprintf(availability_topic, sizeof(availability_topic), MQTT_TOPIC_INPUT_CONTROLLER_AVAILABILITY_PATTERN, deviceIdentity.data.uuid, health.inputControllers[i].address);

    mqttClient.publish(availability_topic, health.inputControllers[i].enabled ? "online" : "offline", true);
  }
}


/**
 * Publishes MQTT availability for all per-component availability topics
 */
void mqtt_publishAllAvailability(){
  mqtt_publishTemperatureAvailability();
  mqtt_publishOutputControllerAvailability();
  mqtt_publishInputControllerAvailability();
}


/**
 * Publishes the designated client CA certificate (fingerprint + PEM) to
 * FireFly/clients/cert/state as a retained message, or clears the retained
 * message with an empty payload when no client cert is designated.
 */
void mqtt_publishClientCertState(){

  if(!mqttClient.connected()) return;

  JsonDocument certTypes = readCertTypes();
  String clientFilename = "";
  for(JsonPair kv : certTypes.as<JsonObject>()){
    if(kv.value()["client"].as<bool>()){
      clientFilename = kv.key().c_str();
      break;
    }
  }

  if(clientFilename.isEmpty()){
    mqttClient.publish("FireFly/clients/cert/state", "", true);
    return;
  }

  String certPath = CONFIGFS_PATH_CERTS + (String)"/" + clientFilename;
  if(!configFS.exists(certPath)){
    mqttClient.publish("FireFly/clients/cert/state", "", true);
    return;
  }

  File f = configFS.open(certPath, "r");
  if(!f){
    mqttClient.publish("FireFly/clients/cert/state", "", true);
    return;
  }
  String pem = "";
  while(f.available()) pem += (char)f.read();
  f.close();

  String fingerprint = computeCertFingerprint(pem);
  if(fingerprint.isEmpty()){
    mqttClient.publish("FireFly/clients/cert/state", "", true);
    return;
  }

  JsonDocument doc;
  doc["fingerprint"] = fingerprint;
  doc["pem"] = pem;

  mqttClient.beginPublish("FireFly/clients/cert/state", measureJson(doc), true);
  BufferingPrint bufferedClient(mqttClient, 32);
  serializeJson(doc, bufferedClient);
  bufferedClient.flush();
  mqttClient.endPublish();
}


/**
 * Publishes the set of controller CA certificates (fingerprint + filename for each)
 * to FireFly/controllers/cert/state as a retained message.  Publishes an empty
 * JSON array when no controller-typed certs are present (Mozilla bundle in use).
 */
void mqtt_publishControllerCertState(){

  if(!mqttClient.connected()) return;

  JsonDocument certTypes = readCertTypes();
  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();

  for(JsonPair kv : certTypes.as<JsonObject>()){
    if(!kv.value()["controller"].as<bool>()) continue;
    String name = kv.key().c_str();
    String certPath = CONFIGFS_PATH_CERTS + (String)"/" + name;
    if(!configFS.exists(certPath)) continue;

    File f = configFS.open(certPath, "r");
    if(!f) continue;
    String pem = "";
    while(f.available()) pem += (char)f.read();
    f.close();

    String fingerprint = computeCertFingerprint(pem);
    if(fingerprint.isEmpty()) continue;

    JsonObject entry = array.add<JsonObject>();
    entry["filename"]    = name;
    entry["fingerprint"] = fingerprint;
  }

  mqttClient.beginPublish("FireFly/controllers/cert/state", measureJson(doc), true);
  BufferingPrint bufferedClient(mqttClient, 32);
  serializeJson(doc, bufferedClient);
  bufferedClient.flush();
  mqttClient.endPublish();
}


/**
 * Publishes the currently observed temperatures for all sensor locations to MQTT
 */
void mqtt_publishTemperatures(){

  if(deviceIdentity.enabled == false){
    return;
  }

  if(!mqttClient.connected()){
    return;
  }

  for(int i = 0; i < TEMPERATURE_SENSOR_COUNT; i++){

    char temperature[6];
    snprintf(temperature, sizeof(temperature), "%.2f", temperatureSensors.getCurrentTemp(i));

    char topic[MQTT_TOPIC_TEMPERATURE_STATE_PATTERN_LENGTH+1];
    snprintf(topic, sizeof(topic), MQTT_TOPIC_TEMPERATURE_STATE_PATTERN, deviceIdentity.data.uuid, temperatureSensors.getSensorLocation(i));

    mqttClient.publish(topic, temperature, true);
  }
}


/**
 * Handles output auto discovery broadcasts
 */
void mqtt_autoDiscovery_outputs(){

  if(deviceIdentity.enabled == false){
    return;
  }

  if(!configFS.exists(CONFIGFS_PATH_CONTROLLERS + (String)"/" + deviceIdentity.data.uuid)){
    log_v("Controller config file does not exist");
    return;
  };

  JsonDocument controllerFilterDoc;

  JsonObject filter_outputs__ = controllerFilterDoc["outputs"]["*"].to<JsonObject>();
  filter_outputs__["id"] = true;
  filter_outputs__["name"] = true;
  filter_outputs__["area"] = true;
  filter_outputs__["icon"] = true;
  filter_outputs__["type"] = true;
  filter_outputs__["relay_manufacturer"] = true;
  filter_outputs__["relay_model"] = true;

  String plaintext;
  if(!secretEncryption.decryptFromFile(configFS, CONFIGFS_PATH_CONTROLLERS + (String)"/" + deviceIdentity.data.uuid, plaintext)){
    eventLog.createEvent("Config decrypt fail", EventLog::LOG_LEVEL_ERROR);
    log_e("Failed to decrypt controller config in mqtt_autoDiscovery_outputs");
    return;
  }

  JsonDocument controllerDoc(&spiRamAllocator); //Supports up to 32 ports
  DeserializationError errorControllerFileDeserialization = deserializeJson(controllerDoc, plaintext, DeserializationOption::Filter(controllerFilterDoc));

  if(errorControllerFileDeserialization) {
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

    if(output.value()["id"].isNull()){
      continue;
    }

    char devicePlatform[WORD_LENGTH_INTEGRATION+1];
    strcpy(devicePlatform,"switch"); //Default

    if(!output.value()["icon"].isNull()){

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

    if(!output.value()["type"].isNull()){
      if(strcmp(output.value()["type"].as<const char*>(), "VARIABLE") == 0){
        isVariableOutput = true;
      }
    }

    JsonDocument mqttDoc;

    char autodiscovery_topic[MQTT_TOPIC_OUTPUT_AUTO_DISCOVERY_LENGTH+1];
    snprintf(autodiscovery_topic, sizeof(autodiscovery_topic), MQTT_TOPIC_OUTPUT_AUTO_DISCOVERY_PATTERN, mqttClient.autoDiscovery.homeAssistantRoot, devicePlatform, output.value()["id"].as<const char*>());

    char unique_id[MQTT_OUTPUT_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1];
    snprintf(unique_id, sizeof(unique_id), MQTT_OUTPUT_AUTO_DISCOVERY_UNIQUE_ID_PATTERN, output.value()["id"].as<const char*>());

    char default_entity_id[MQTT_OUTPUT_DEFAULT_ENTITY_ID_LENGTH+1];
    snprintf(default_entity_id, sizeof(default_entity_id), MQTT_OUTPUT_DEFAULT_ENTITY_ID_PATTERN, devicePlatform, output.value()["id"].as<const char*>());

    char state_topic[MQTT_TOPIC_OUTPUT_STATE_LENGTH+1];
    snprintf(state_topic, sizeof(state_topic), MQTT_TOPIC_OUTPUT_STATE_PATTERN, output.value()["id"].as<const char*>());

    char command_topic[MQTT_TOPIC_OUTPUT_SET_LENGTH+1];
    snprintf(command_topic, sizeof(command_topic), MQTT_TOPIC_OUTPUT_SET_PATTERN, output.value()["id"].as<const char*>());

    mqttDoc["name"] = (char*)NULL;
    mqttDoc["unique_id"] = unique_id;
    mqttDoc["default_entity_id"] = default_entity_id;

    if(!output.value()["icon"].isNull()){
        mqttDoc["icon"] = output.value()["icon"].as<const char*>();
    }

    if(isVariableOutput){
      mqttDoc["on_command_type"] = "brightness";
      mqttDoc["brightness_scale"] = 100;
      mqttDoc["brightness_command_topic"] = command_topic;
      mqttDoc["brightness_state_topic"] = state_topic;
    }

    mqttDoc["state_value_template"] = "{% if value|int > 0 %}ON{% else %}OFF{% endif %}";

    JsonObject device = mqttDoc["device"].to<JsonObject>();
    JsonArray identifiers = device["identifiers"].to<JsonArray>();
    identifiers.add(unique_id);

    if(!output.value()["name"].isNull()){
      device["name"] = output.value()["name"].as<const char*>();
    }else{
      device["name"] = output.value()["id"].as<const char*>();
    }

    device["via_device"] = deviceIdentity.data.uuid;

    if(!output.value()["area"].isNull()){
      device["suggested_area"] =  output.value()["area"].as<const char*>();
    }

    if(!output.value()["relay_manufacturer"].isNull()){
      device["manufacturer"] = output.value()["relay_manufacturer"].as<const char*>();
    }

    if(!output.value()["relay_model"].isNull()){
      device["model"] = output.value()["relay_model"].as<const char*>();
    }

    mqttDoc["state_topic"] = state_topic;
    mqttDoc["command_topic"] = command_topic;

    uint8_t chipIndex = (outputPortNumber - 1) / OUTPUT_CONTROLLER_COUNT_PINS;
    const uint8_t chipAddresses[] = OUTPUT_CONTROLLER_ADDRESSES;
    char chip_availability_topic[MQTT_TOPIC_OUTPUT_CONTROLLER_AVAILABILITY_LENGTH+1];
    snprintf(chip_availability_topic, sizeof(chip_availability_topic), MQTT_TOPIC_OUTPUT_CONTROLLER_AVAILABILITY_PATTERN, deviceIdentity.data.uuid, chipAddresses[chipIndex]);

    JsonArray availability = mqttDoc["availability"].to<JsonArray>();
    JsonObject chip_specific = availability.add<JsonObject>();
    JsonObject controller_level = availability.add<JsonObject>();
    chip_specific["topic"] = chip_availability_topic;
    controller_level["topic"] = mqttClient.topic_availability;
    mqttDoc["availability_mode"] = "all";

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
 * Handles input channel sensor auto discovery broadcasts.
 * Publishes one retained discovery message per configured input channel so that
 * Home Assistant exposes each channel as a sensor entity whose state reflects the
 * most recently published value (NORMAL, SHORT, LONG, or EXCESSIVE).
 */
void mqtt_autoDiscovery_inputs(){

  if(deviceIdentity.enabled == false){
    return;
  }

  uint8_t portCount = (IO_EXTENDER_COUNT_PINS / IO_EXTENDER_COUNT_CHANNELS_PER_PORT) * IO_EXTENDER_COUNT;

  for(uint8_t p = 0; p < portCount; p++){

    if(inputPorts[p].id[0] == '\0'){
      continue;
    }

    for(uint8_t c = 0; c < IO_EXTENDER_COUNT_CHANNELS_PER_PORT; c++){

      if(inputPorts[p].channels[c].channel == 0){
        continue;
      }

      managerInputs::portChannel pc = {(uint8_t)(p+1), inputPorts[p].channels[c].channel};
      managerInputs::portChannelInfo info = inputs.getPortChannelInfo(pc);
      uint8_t chipAddress = info.chipAddress;
      uint8_t channelOffset = info.offset;

      uint8_t logicalChannel = inputPorts[p].channels[c].channel + channelOffset;

      char sanitized_id[PORT_ID_MAX_LENGTH+1];
      strlcpy(sanitized_id, inputPorts[p].id, sizeof(sanitized_id));
      for(uint8_t i = 0; sanitized_id[i] != '\0'; i++){
        if(!isalnum((unsigned char)sanitized_id[i]) && sanitized_id[i] != '-'){
          sanitized_id[i] = '_';
        }
      }

      char autodiscovery_topic[MQTT_TOPIC_INPUT_AUTO_DISCOVERY_LENGTH+1];
      snprintf(autodiscovery_topic, sizeof(autodiscovery_topic), MQTT_TOPIC_INPUT_AUTO_DISCOVERY_PATTERN,
        mqttClient.autoDiscovery.homeAssistantRoot, deviceIdentity.data.uuid, sanitized_id, logicalChannel);

      char unique_id[MQTT_INPUT_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1];
      snprintf(unique_id, sizeof(unique_id), MQTT_INPUT_AUTO_DISCOVERY_UNIQUE_ID_PATTERN,
        deviceIdentity.data.uuid, sanitized_id, logicalChannel);

      char default_entity_id[MQTT_INPUT_AUTO_DISCOVERY_DEFAULT_ENTITY_ID_LENGTH+1];
      snprintf(default_entity_id, sizeof(default_entity_id), MQTT_INPUT_AUTO_DISCOVERY_DEFAULT_ENTITY_ID_PATTERN,
        deviceIdentity.data.uuid, sanitized_id, logicalChannel);

      char state_topic[MQTT_TOPIC_INPUT_STATE_PATTERN_LENGTH+1];
      snprintf(state_topic, sizeof(state_topic), MQTT_TOPIC_INPUT_STATE_PATTERN, inputPorts[p].id, logicalChannel);

      char device_id[MQTT_INPUT_AUTO_DISCOVERY_DEVICE_ID_LENGTH+1];
      if(inputPorts[p].clientUUID[0] != '\0'){
        snprintf(device_id, sizeof(device_id), MQTT_INPUT_AUTO_DISCOVERY_CLIENT_DEVICE_ID_PATTERN,
          inputPorts[p].clientUUID);
      } else {
        snprintf(device_id, sizeof(device_id), MQTT_INPUT_AUTO_DISCOVERY_DEVICE_ID_PATTERN,
          deviceIdentity.data.uuid, sanitized_id);
      }

      char chip_availability_topic[MQTT_TOPIC_INPUT_CONTROLLER_AVAILABILITY_LENGTH+1];
      snprintf(chip_availability_topic, sizeof(chip_availability_topic), MQTT_TOPIC_INPUT_CONTROLLER_AVAILABILITY_PATTERN,
        deviceIdentity.data.uuid, chipAddress);

      char channel_name[16];
      snprintf(channel_name, sizeof(channel_name), "Channel %u", logicalChannel);

      JsonDocument mqttDoc;
      mqttDoc["name"] = channel_name;
      mqttDoc["unique_id"] = unique_id;
      mqttDoc["default_entity_id"] = default_entity_id;
      mqttDoc["state_topic"] = state_topic;
      mqttDoc["value_template"] = "{{ value | title }}";
      mqttDoc["icon"] = "mdi:gesture-tap";

      JsonObject device = mqttDoc["device"].to<JsonObject>();
      JsonArray identifiers = device["identifiers"].to<JsonArray>();
      identifiers.add(device_id);
      device["name"] = inputPorts[p].id;
      device["via_device"] = deviceIdentity.data.uuid;

      JsonArray availability = mqttDoc["availability"].to<JsonArray>();
      JsonObject chip_avail = availability.add<JsonObject>();
      JsonObject controller_avail = availability.add<JsonObject>();
      chip_avail["topic"] = chip_availability_topic;
      controller_avail["topic"] = mqttClient.topic_availability;
      mqttDoc["availability_mode"] = "all";

      mqttClient.beginPublish(autodiscovery_topic, measureJson(mqttDoc), true);
      BufferingPrint bufferedClient(mqttClient, 32);
      serializeJson(mqttDoc, bufferedClient);
      bufferedClient.flush();
      mqttClient.endPublish();
    }
  }
}


/**
 * Handles input controller chip sensor auto discovery broadcasts.
 * Publishes one retained discovery message per IO extender chip so that
 * Home Assistant exposes each chip as a sensor entity linked to the controller device.
 * The sensor state_topic is the chip's existing availability topic, so state values
 * are "online" or "offline".
 */
void mqtt_autoDiscovery_inputControllers(){

  if(deviceIdentity.enabled == false){
    return;
  }

  const uint8_t portsPerChip = IO_EXTENDER_COUNT_PINS / IO_EXTENDER_COUNT_CHANNELS_PER_PORT;
  const uint8_t chipAddresses[] = IO_EXTENDER_ADDRESSES;

  for(int i = 0; i < IO_EXTENDER_COUNT; i++){

    JsonDocument doc;

    char topic[MQTT_TOPIC_INPUT_CONTROLLER_AUTO_DISCOVERY_LENGTH+1];
    snprintf(topic, sizeof(topic), MQTT_TOPIC_INPUT_CONTROLLER_AUTO_DISCOVERY_PATTERN, mqttClient.autoDiscovery.homeAssistantRoot, deviceIdentity.data.uuid, chipAddresses[i]);

    char unique_id[MQTT_INPUT_CONTROLLER_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1];
    snprintf(unique_id, sizeof(unique_id), MQTT_INPUT_CONTROLLER_AUTO_DISCOVERY_UNIQUE_ID_PATTERN, deviceIdentity.data.uuid, chipAddresses[i]);

    char default_entity_id[MQTT_INPUT_CONTROLLER_AUTO_DISCOVERY_DEFAULT_ENTITY_ID_LENGTH+1];
    snprintf(default_entity_id, sizeof(default_entity_id), MQTT_INPUT_CONTROLLER_AUTO_DISCOVERY_DEFAULT_ENTITY_ID_PATTERN, deviceIdentity.data.uuid, chipAddresses[i]);

    char state_topic[MQTT_TOPIC_INPUT_CONTROLLER_AVAILABILITY_LENGTH+1];
    snprintf(state_topic, sizeof(state_topic), MQTT_TOPIC_INPUT_CONTROLLER_AVAILABILITY_PATTERN, deviceIdentity.data.uuid, chipAddresses[i]);

    uint8_t firstPort = (i * portsPerChip) + 1;
    uint8_t lastPort = (i + 1) * portsPerChip;
    char name[32];
    snprintf(name, sizeof(name), "Inputs %u-%u", firstPort, lastPort);

    doc["name"] = name;
    doc["unique_id"] = unique_id;
    doc["default_entity_id"] = default_entity_id;
    doc["icon"] = "mdi:chip";
    doc["entity_category"] = "diagnostic";
    doc["state_topic"] = state_topic;
    doc["value_template"] = "{% if value == 'online' %}Online{% else %}Offline{% endif %}";

    JsonObject device = doc["device"].to<JsonObject>();
    JsonArray identifiers = device["identifiers"].to<JsonArray>();
    identifiers.add(deviceIdentity.data.uuid);

    if(strlen(mqttClient.autoDiscovery.deviceName) > 0){
      device["name"] = mqttClient.autoDiscovery.deviceName;
    }

    device["manufacturer"] = HARDWARE_MANUFACTURER_NAME;
    device["model"] = APPLICATION_NAME;
    device["model_id"] = deviceIdentity.data.product_id;
    device["serial_number"] = deviceIdentity.data.uuid;
    device["sw_version"] = VERSION " (" COMMIT_HASH ")";
    device["configuration_url"] = ("http://" + ETH.localIP().toString()).c_str();

    if(strlen(mqttClient.autoDiscovery.suggestedArea) > 0){
      device["suggested_area"] = mqttClient.autoDiscovery.suggestedArea;
    }

    doc["availability_topic"] = mqttClient.topic_availability;

    mqttClient.beginPublish(topic, measureJson(doc), true);
    BufferingPrint bufferedClient(mqttClient, 32);
    serializeJson(doc, bufferedClient);
    bufferedClient.flush();
    mqttClient.endPublish();
  }
}


/**
 * Handles output controller chip sensor auto discovery broadcasts.
 * Publishes one retained discovery message per PWM controller chip so that
 * Home Assistant exposes each chip as a sensor entity linked to the controller device.
 * The sensor state_topic is the chip's existing availability topic, so state values
 * are "online" or "offline".
 */
void mqtt_autoDiscovery_outputControllers(){

  if(deviceIdentity.enabled == false){
    return;
  }

  const uint8_t chipAddresses[] = OUTPUT_CONTROLLER_ADDRESSES;

  for(int i = 0; i < OUTPUT_CONTROLLER_COUNT; i++){

    JsonDocument doc;

    char topic[MQTT_TOPIC_OUTPUT_CONTROLLER_AUTO_DISCOVERY_LENGTH+1];
    snprintf(topic, sizeof(topic), MQTT_TOPIC_OUTPUT_CONTROLLER_AUTO_DISCOVERY_PATTERN, mqttClient.autoDiscovery.homeAssistantRoot, deviceIdentity.data.uuid, chipAddresses[i]);

    char unique_id[MQTT_OUTPUT_CONTROLLER_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1];
    snprintf(unique_id, sizeof(unique_id), MQTT_OUTPUT_CONTROLLER_AUTO_DISCOVERY_UNIQUE_ID_PATTERN, deviceIdentity.data.uuid, chipAddresses[i]);

    char default_entity_id[MQTT_OUTPUT_CONTROLLER_AUTO_DISCOVERY_DEFAULT_ENTITY_ID_LENGTH+1];
    snprintf(default_entity_id, sizeof(default_entity_id), MQTT_OUTPUT_CONTROLLER_AUTO_DISCOVERY_DEFAULT_ENTITY_ID_PATTERN, deviceIdentity.data.uuid, chipAddresses[i]);

    char state_topic[MQTT_TOPIC_OUTPUT_CONTROLLER_AVAILABILITY_LENGTH+1];
    snprintf(state_topic, sizeof(state_topic), MQTT_TOPIC_OUTPUT_CONTROLLER_AVAILABILITY_PATTERN, deviceIdentity.data.uuid, chipAddresses[i]);

    uint8_t firstCircuit = (i * OUTPUT_CONTROLLER_COUNT_PINS) + 1;
    uint8_t lastCircuit = (i + 1) * OUTPUT_CONTROLLER_COUNT_PINS;
    char name[32];
    snprintf(name, sizeof(name), "Outputs %u-%u", firstCircuit, lastCircuit);

    doc["name"] = name;
    doc["unique_id"] = unique_id;
    doc["default_entity_id"] = default_entity_id;
    doc["icon"] = "mdi:chip";
    doc["entity_category"] = "diagnostic";
    doc["state_topic"] = state_topic;
    doc["value_template"] = "{% if value == 'online' %}Online{% else %}Offline{% endif %}";

    JsonObject device = doc["device"].to<JsonObject>();
    JsonArray identifiers = device["identifiers"].to<JsonArray>();
    identifiers.add(deviceIdentity.data.uuid);

    if(strlen(mqttClient.autoDiscovery.deviceName) > 0){
      device["name"] = mqttClient.autoDiscovery.deviceName;
    }

    device["manufacturer"] = HARDWARE_MANUFACTURER_NAME;
    device["model"] = APPLICATION_NAME;
    device["model_id"] = deviceIdentity.data.product_id;
    device["serial_number"] = deviceIdentity.data.uuid;
    device["sw_version"] = VERSION " (" COMMIT_HASH ")";
    device["configuration_url"] = ("http://" + ETH.localIP().toString()).c_str();

    if(strlen(mqttClient.autoDiscovery.suggestedArea) > 0){
      device["suggested_area"] = mqttClient.autoDiscovery.suggestedArea;
    }

    doc["availability_topic"] = mqttClient.topic_availability;

    mqttClient.beginPublish(topic, measureJson(doc), true);
    BufferingPrint bufferedClient(mqttClient, 32);
    serializeJson(doc, bufferedClient);
    bufferedClient.flush();
    mqttClient.endPublish();
  }
}


/**
 * Handles start time sensor auto discovery broadcasts
 */
void mqtt_autoDiscovery_start_time(){

  if(deviceIdentity.enabled == false){
    return;
  }

  JsonDocument doc;

  char topic[MQTT_TOPIC_TIME_START_AUTO_DISCOVERY_LENGTH+1];
  snprintf(topic, sizeof(topic), MQTT_TOPIC_TIME_START_AUTO_DISCOVERY_PATTERN, mqttClient.autoDiscovery.homeAssistantRoot, deviceIdentity.data.uuid);

  char unique_id[MQTT_TIME_START_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1];
  snprintf(unique_id, sizeof(unique_id), MQTT_TIME_START_AUTO_DISCOVERY_UNIQUE_ID_PATTERN, deviceIdentity.data.uuid);

  char default_entity_id[MQTT_TIME_START_DEFAULT_ENTITY_ID_LENGTH+1];
  snprintf(default_entity_id, sizeof(default_entity_id), MQTT_TIME_START_DEFAULT_ENTITY_ID_PATTERN, deviceIdentity.data.uuid);

  char state_topic[MQTT_TOPIC_TIME_START_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, sizeof(state_topic), MQTT_TOPIC_TIME_START_STATE_PATTERN, deviceIdentity.data.uuid);

  doc["name"] = "Start Time";
  doc["unique_id"] = unique_id;
  doc["default_entity_id"] = default_entity_id;
  doc["icon"] = "mdi:calendar-clock";
  doc["entity_category"] = "diagnostic";

  JsonObject device = doc["device"].to<JsonObject>();
  JsonArray identifiers = device["identifiers"].to<JsonArray>();
  identifiers.add(deviceIdentity.data.uuid);

  if(strlen(mqttClient.autoDiscovery.deviceName) > 0){
    device["name"] =  mqttClient.autoDiscovery.deviceName;
  }

  device["manufacturer"] = HARDWARE_MANUFACTURER_NAME;
  device["model"] = APPLICATION_NAME;
  device["model_id"] = deviceIdentity.data.product_id;
  device["serial_number"] = deviceIdentity.data.uuid;
  device["sw_version"] = VERSION " (" COMMIT_HASH ")";
  device["configuration_url"] = ("http://" + ETH.localIP().toString()).c_str();

  if(strlen(mqttClient.autoDiscovery.suggestedArea) > 0){
        device["suggested_area"] =  mqttClient.autoDiscovery.suggestedArea;
  }

  doc["state_topic"] = state_topic;
  doc["device_class"] = "timestamp";
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

  if(deviceIdentity.enabled == false){
    return;
  }

  if(!mqttClient.connected()){
    return;
  }

  char state_topic[MQTT_TOPIC_TIME_START_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, sizeof(state_topic), MQTT_TOPIC_TIME_START_STATE_PATTERN, deviceIdentity.data.uuid);

  char start_time[21];
  snprintf(start_time, sizeof(start_time), "%llu", bootTime);

  mqttClient.publish(state_topic, start_time, true);
}


/**
 * Handles MAC address sensor auto discovery broadcasts
 */
void mqtt_autoDiscovery_mac_address(){

  if(deviceIdentity.enabled == false){
    return;
  }

  JsonDocument doc;

  char topic[MQTT_TOPIC_MAC_ADDRESS_AUTO_DISCOVERY_LENGTH+1];
  snprintf(topic, sizeof(topic), MQTT_TOPIC_MAC_ADDRESS_AUTO_DISCOVERY_PATTERN, mqttClient.autoDiscovery.homeAssistantRoot, deviceIdentity.data.uuid);

  char unique_id[MQTT_MAC_ADDRESS_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1];
  snprintf(unique_id, sizeof(unique_id), MQTT_MAC_ADDRESS_AUTO_DISCOVERY_UNIQUE_ID_PATTERN, deviceIdentity.data.uuid);

  char default_entity_id[MQTT_MAC_ADDRESS_DEFAULT_ENTITY_ID_LENGTH+1];
  snprintf(default_entity_id, sizeof(default_entity_id), MQTT_MAC_ADDRESS_DEFAULT_ENTITY_ID_PATTERN, deviceIdentity.data.uuid);

  char state_topic[MQTT_TOPIC_MAC_ADDRESS_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, sizeof(state_topic), MQTT_TOPIC_MAC_ADDRESS_STATE_PATTERN, deviceIdentity.data.uuid);

  doc["name"] = "MAC Address";
  doc["unique_id"] = unique_id;
  doc["default_entity_id"] = default_entity_id;
  doc["icon"] = "mdi:ethernet";
  doc["entity_category"] = "diagnostic";

  JsonObject device = doc["device"].to<JsonObject>();
  JsonArray identifiers = device["identifiers"].to<JsonArray>();
  identifiers.add(deviceIdentity.data.uuid);

  if(strlen(mqttClient.autoDiscovery.deviceName) > 0){
    device["name"] =  mqttClient.autoDiscovery.deviceName;
  }

  device["manufacturer"] = HARDWARE_MANUFACTURER_NAME;
  device["model"] = APPLICATION_NAME;
  device["model_id"] = deviceIdentity.data.product_id;
  device["serial_number"] = deviceIdentity.data.uuid;
  device["sw_version"] = VERSION " (" COMMIT_HASH ")";
  device["configuration_url"] = ("http://" + ETH.localIP().toString()).c_str();

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

  if(deviceIdentity.enabled == false){
    return;
  }

  if(!mqttClient.connected()){
    return;
  }

  char state_topic[MQTT_TOPIC_MAC_ADDRESS_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, sizeof(state_topic), MQTT_TOPIC_MAC_ADDRESS_STATE_PATTERN, deviceIdentity.data.uuid);

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

  JsonDocument doc;

  char topic[MQTT_TOPIC_IP_ADDRESS_AUTO_DISCOVERY_LENGTH+1];
  snprintf(topic, sizeof(topic), MQTT_TOPIC_IP_ADDRESS_AUTO_DISCOVERY_PATTERN, mqttClient.autoDiscovery.homeAssistantRoot, deviceIdentity.data.uuid);

  char unique_id[MQTT_IP_ADDRESS_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1];
  snprintf(unique_id, sizeof(unique_id), MQTT_IP_ADDRESS_AUTO_DISCOVERY_UNIQUE_ID_PATTERN, deviceIdentity.data.uuid);

  char default_entity_id[MQTT_IP_ADDRESS_DEFAULT_ENTITY_ID_LENGTH+1];
  snprintf(default_entity_id, sizeof(default_entity_id), MQTT_IP_ADDRESS_DEFAULT_ENTITY_ID_PATTERN, deviceIdentity.data.uuid);

  char state_topic[MQTT_TOPIC_IP_ADDRESS_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, sizeof(state_topic), MQTT_TOPIC_IP_ADDRESS_STATE_PATTERN, deviceIdentity.data.uuid);

  doc["name"] = "IP Address";
  doc["unique_id"] = unique_id;
  doc["default_entity_id"] = default_entity_id;
  doc["icon"] = "mdi:ip";
  doc["entity_category"] = "diagnostic";

  JsonObject device = doc["device"].to<JsonObject>();
  JsonArray identifiers = device["identifiers"].to<JsonArray>();
  identifiers.add(deviceIdentity.data.uuid);

  if(strlen(mqttClient.autoDiscovery.deviceName) > 0){
    device["name"] =  mqttClient.autoDiscovery.deviceName;
  }

  device["manufacturer"] = HARDWARE_MANUFACTURER_NAME;
  device["model"] = APPLICATION_NAME;
  device["model_id"] = deviceIdentity.data.product_id;
  device["serial_number"] = deviceIdentity.data.uuid;
  device["sw_version"] = VERSION " (" COMMIT_HASH ")";
  device["configuration_url"] = ("http://" + ETH.localIP().toString()).c_str();

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

  if(deviceIdentity.enabled == false){
    return;
  }

  if(!mqttClient.connected()){
    return;
  }

  char state_topic[MQTT_TOPIC_IP_ADDRESS_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, sizeof(state_topic), MQTT_TOPIC_IP_ADDRESS_STATE_PATTERN, deviceIdentity.data.uuid);

  #if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_W5500
    mqttClient.publish(state_topic, ETH.localIP().toString().c_str(), true);
  #else
    #warning Unknown Ethernet Controller; IP address will not be published to MQTT
  #endif
}


/**
 * Handles count errors sensor auto discovery broadcasts
 */
void mqtt_autoDiscovery_count_errors(){

  if(deviceIdentity.enabled == false){
    return;
  }

  JsonDocument doc;

  char topic[MQTT_TOPIC_COUNT_ERRORS_AUTO_DISCOVERY_LENGTH+1];
  snprintf(topic, sizeof(topic), MQTT_TOPIC_COUNT_ERRORS_AUTO_DISCOVERY_PATTERN, mqttClient.autoDiscovery.homeAssistantRoot, deviceIdentity.data.uuid);

  char unique_id[MQTT_COUNT_ERRORS_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1];
  snprintf(unique_id, sizeof(unique_id), MQTT_COUNT_ERRORS_AUTO_DISCOVERY_UNIQUE_ID_PATTERN, deviceIdentity.data.uuid);

  char default_entity_id[MQTT_COUNT_ERRORS_DEFAULT_ENTITY_ID_LENGTH+1];
  snprintf(default_entity_id, sizeof(default_entity_id), MQTT_COUNT_ERRORS_DEFAULT_ENTITY_ID_PATTERN, deviceIdentity.data.uuid);

  char state_topic[MQTT_TOPIC_COUNT_ERRORS_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, sizeof(state_topic), MQTT_TOPIC_COUNT_ERRORS_STATE_PATTERN, deviceIdentity.data.uuid);

  doc["name"] = "Error Count";
  doc["unique_id"] = unique_id;
  doc["default_entity_id"] = default_entity_id;
  doc["icon"] = "mdi:alert";
  doc["entity_category"] = "diagnostic";

  JsonObject device = doc["device"].to<JsonObject>();
  JsonArray identifiers = device["identifiers"].to<JsonArray>();
  identifiers.add(deviceIdentity.data.uuid);

  if(strlen(mqttClient.autoDiscovery.deviceName) > 0){
    device["name"] =  mqttClient.autoDiscovery.deviceName;
  }

  device["manufacturer"] = HARDWARE_MANUFACTURER_NAME;
  device["model"] = APPLICATION_NAME;
  device["model_id"] = deviceIdentity.data.product_id;
  device["serial_number"] = deviceIdentity.data.uuid;
  device["sw_version"] = VERSION " (" COMMIT_HASH ")";
  device["configuration_url"] = ("http://" + ETH.localIP().toString()).c_str();

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

  if(deviceIdentity.enabled == false){
    return;
  }

  if(!mqttClient.connected()){
    return;
  }

  char state_topic[MQTT_TOPIC_COUNT_ERRORS_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, sizeof(state_topic), MQTT_TOPIC_COUNT_ERRORS_STATE_PATTERN, deviceIdentity.data.uuid);

  char count[3];
  snprintf(count, sizeof(count), "%i", eventLog.getErrors()->size());

  mqttClient.publish(state_topic, count, true);
}


/**
 * Handles update auto discovery broadcasts
 */
void mqtt_autoDiscovery_update(){

  if(deviceIdentity.enabled == false){
    return;
  }
  
  char topic[MQTT_TOPIC_UPDATE_AUTO_DISCOVERY_PATTERN_LENGTH+1];
  snprintf(topic, sizeof(topic), MQTT_TOPIC_UPDATE_AUTO_DISCOVERY_PATTERN, mqttClient.autoDiscovery.homeAssistantRoot, deviceIdentity.data.uuid);

  char unique_id[MQTT_UPDATE_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1];
  snprintf(unique_id, sizeof(unique_id), MQTT_UPDATE_AUTO_DISCOVERY_UNIQUE_ID_PATTERN, deviceIdentity.data.uuid);

  char default_entity_id[MQTT_UPDATE_DEFAULT_ENTITY_ID_LENGTH+1];
  snprintf(default_entity_id, sizeof(default_entity_id), MQTT_UPDATE_DEFAULT_ENTITY_ID_PATTERN, deviceIdentity.data.uuid);

  char state_topic[MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, sizeof(state_topic), MQTT_TOPIC_UPDATE_STATE_PATTERN, deviceIdentity.data.uuid);

  char command_topic[MQTT_TOPIC_UPDATE_SET_PATTERN_LENGTH+1];
  snprintf(command_topic, sizeof(command_topic), MQTT_TOPIC_UPDATE_SET_PATTERN, deviceIdentity.data.uuid);

  char availability_topic[MQTT_TOPIC_UPDATE_AVAILABILITY_LENGTH+1];
  snprintf(availability_topic, sizeof(availability_topic), MQTT_TOPIC_UPDATE_AVAILABILITY_PATTERN, deviceIdentity.data.uuid);

  JsonDocument doc;

  doc["name"] = "Firmware";
  doc["unique_id"] = unique_id;
  doc["default_entity_id"] = default_entity_id;
  doc["icon"] = "mdi:update";

  JsonObject device = doc["device"].to<JsonObject>();
  JsonArray identifiers = device["identifiers"].to<JsonArray>();
  identifiers.add(deviceIdentity.data.uuid);

  if(strlen(mqttClient.autoDiscovery.deviceName) > 0){
    device["name"] =  mqttClient.autoDiscovery.deviceName;
  }

  device["manufacturer"] = HARDWARE_MANUFACTURER_NAME;
  device["model"] = APPLICATION_NAME;
  device["model_id"] = deviceIdentity.data.product_id;
  device["serial_number"] = deviceIdentity.data.uuid;
  device["sw_version"] = VERSION " (" COMMIT_HASH ")";
  device["configuration_url"] = ("http://" + ETH.localIP().toString()).c_str();

  if(strlen(mqttClient.autoDiscovery.suggestedArea) > 0){
        device["suggested_area"] =  mqttClient.autoDiscovery.suggestedArea;
  }

  JsonArray availability = doc["availability"].to<JsonArray>(); //Note, this is different from others
  JsonObject update_specific = availability.add<JsonObject>();
  JsonObject controller_level = availability.add<JsonObject>();
  update_specific["topic"] = availability_topic;
  controller_level["topic"] = mqttClient.topic_availability;
  doc["availability_mode"] = "all";

  doc["state_topic"] = state_topic;
  doc["command_topic"] = command_topic;
  doc["payload_install"] = "do-update";

  mqttClient.beginPublish(topic, measureJson(doc), true);
  BufferingPrint bufferedClient(mqttClient, 32);
  serializeJson(doc, bufferedClient);
  bufferedClient.flush();
  mqttClient.endPublish();

  mqttClient.addSubscription(command_topic);
}


/**
 * Handles http server auto discovery broadcasts
 */
void mqtt_autoDiscovery_http_server(){

  JsonDocument doc;

  char topic[MQTT_TOPIC_HTTP_SERVER_AUTO_DISCOVERY_LENGTH+1];
  snprintf(topic, sizeof(topic), MQTT_TOPIC_HTTP_SERVER_AUTO_DISCOVERY_PATTERN, mqttClient.autoDiscovery.homeAssistantRoot, deviceIdentity.data.uuid);

  char unique_id[MQTT_HTTP_SERVER_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1];
  snprintf(unique_id, sizeof(unique_id), MQTT_HTTP_SERVER_AUTO_DISCOVERY_UNIQUE_ID_PATTERN, deviceIdentity.data.uuid);

  char default_entity_id[MQTT_HTTP_SERVER_DEFAULT_ENTITY_ID_LENGTH+1];
  snprintf(default_entity_id, sizeof(default_entity_id), MQTT_HTTP_SERVER_DEFAULT_ENTITY_ID_PATTERN, deviceIdentity.data.uuid);

  char state_topic[MQTT_TOPIC_HTTP_SERVER_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, sizeof(state_topic), MQTT_TOPIC_HTTP_SERVER_STATE_PATTERN, deviceIdentity.data.uuid);

  char command_topic[MQTT_TOPIC_HTTP_SERVER_SET_PATTERN_LENGTH+1];
  snprintf(command_topic, sizeof(command_topic), MQTT_TOPIC_HTTP_SERVER_SET_PATTERN, deviceIdentity.data.uuid);

  doc["name"] = "HTTP Server";
  doc["unique_id"] = unique_id;
  doc["default_entity_id"] = default_entity_id;
  doc["icon"] = "mdi:web";

  JsonObject device = doc["device"].to<JsonObject>();
  JsonArray identifiers = device["identifiers"].to<JsonArray>();
  identifiers.add(deviceIdentity.data.uuid);

  if(strlen(mqttClient.autoDiscovery.deviceName) > 0){
    device["name"] =  mqttClient.autoDiscovery.deviceName;
  }

  device["manufacturer"] = HARDWARE_MANUFACTURER_NAME;
  device["model"] = APPLICATION_NAME;
  device["model_id"] = deviceIdentity.data.product_id;
  device["serial_number"] = deviceIdentity.data.uuid;
  device["sw_version"] = VERSION " (" COMMIT_HASH ")";
  device["configuration_url"] = ("http://" + ETH.localIP().toString()).c_str();

  if(strlen(mqttClient.autoDiscovery.suggestedArea) > 0){
        device["suggested_area"] =  mqttClient.autoDiscovery.suggestedArea;
  }

  doc["state_topic"] = state_topic;
  doc["command_topic"] = command_topic;
  doc["availability_topic"] = mqttClient.topic_availability;
  doc["payload_on"] = "ON";
  doc["payload_off"] = "OFF";
  doc["state_on"] = "ON";
  doc["state_off"] ="OFF";

  mqttClient.beginPublish(topic, measureJson(doc), true);
  BufferingPrint bufferedClient(mqttClient, 32);
  serializeJson(doc, bufferedClient);
  bufferedClient.flush();
  mqttClient.endPublish();

  mqttClient.addSubscription(command_topic);
}


/**
 * Broadcasts an MQTT message that the HTTP server state has changed
 * @param state as the HTTP servers' current state (True = Enabled, False = Disabled)
 */
void mqtt_publishHttpServerStateChanged(boolean state){

  if(deviceIdentity.enabled == false){
    return;
  }

  if(!mqttClient.connected()){
    return;
  }

  char state_topic[MQTT_TOPIC_HTTP_SERVER_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, sizeof(state_topic), MQTT_TOPIC_HTTP_SERVER_STATE_PATTERN, deviceIdentity.data.uuid);

  char value_char[4];
  if(state == true){
    snprintf(value_char, sizeof(value_char), "%s", "ON");
  }else{
    snprintf(value_char, sizeof(value_char), "%s", "OFF");
  }

  mqttClient.publish(state_topic, value_char, true);
}


/**
 * Handles heap free auto discovery broadcasts
 */
void mqtt_autoDiscovery_heapFree(){

  JsonDocument doc;

  char topic[MQTT_TOPIC_HEAP_FREE_AUTO_DISCOVERY_LENGTH+1];
  snprintf(topic, sizeof(topic), MQTT_TOPIC_HEAP_FREE_AUTO_DISCOVERY_PATTERN, mqttClient.autoDiscovery.homeAssistantRoot, deviceIdentity.data.uuid);

  char unique_id[MQTT_HEAP_FREE_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1];
  snprintf(unique_id, sizeof(unique_id), MQTT_HEAP_FREE_AUTO_DISCOVERY_UNIQUE_ID_PATTERN, deviceIdentity.data.uuid);

  char default_entity_id[MQTT_HEAP_FREE_DEFAULT_ENTITY_ID_LENGTH+1];
  snprintf(default_entity_id, sizeof(default_entity_id), MQTT_HEAP_FREE_DEFAULT_ENTITY_ID_PATTERN, deviceIdentity.data.uuid);

  char state_topic[MQTT_TOPIC_HEAP_FREE_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, sizeof(state_topic), MQTT_TOPIC_HEAP_FREE_STATE_PATTERN, deviceIdentity.data.uuid);

  doc["name"] = "Heap Free";
  doc["unique_id"] = unique_id;
  doc["default_entity_id"] = default_entity_id;
  doc["icon"] = "mdi:memory";
  doc["entity_category"] = "diagnostic";
  doc["enabled_by_default"] = false;
  doc["unit_of_measurement"] = "B";
  doc["device_class"] = "data_size";
  doc["state_class"] = "measurement";

  JsonObject device = doc["device"].to<JsonObject>();
  JsonArray identifiers = device["identifiers"].to<JsonArray>();
  identifiers.add(deviceIdentity.data.uuid);

  if(strlen(mqttClient.autoDiscovery.deviceName) > 0){
    device["name"] =  mqttClient.autoDiscovery.deviceName;
  }

  device["manufacturer"] = HARDWARE_MANUFACTURER_NAME;
  device["model"] = APPLICATION_NAME;
  device["model_id"] = deviceIdentity.data.product_id;
  device["serial_number"] = deviceIdentity.data.uuid;
  device["sw_version"] = VERSION " (" COMMIT_HASH ")";
  device["configuration_url"] = ("http://" + ETH.localIP().toString()).c_str();

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
 * Broadcasts an MQTT message containing the current free heap
 */
void mqtt_publish_heapFree(){

  if(deviceIdentity.enabled == false){
    return;
  }

  if(!mqttClient.connected()){
    return;
  }

  char state_topic[MQTT_TOPIC_HEAP_FREE_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, sizeof(state_topic), MQTT_TOPIC_HEAP_FREE_STATE_PATTERN, deviceIdentity.data.uuid);

  char buf[12];
  sprintf(buf, "%u", (unsigned int)ESP.getFreeHeap());

  mqttClient.publish(state_topic, buf, true);
}


/**
 * Handles heap largest free block auto discovery broadcasts
 */
void mqtt_autoDiscovery_heapLargestFreeBlock(){

  JsonDocument doc;

  char topic[MQTT_TOPIC_HEAP_LARGEST_FREE_BLOCK_AUTO_DISCOVERY_LENGTH+1];
  snprintf(topic, sizeof(topic), MQTT_TOPIC_HEAP_LARGEST_FREE_BLOCK_AUTO_DISCOVERY_PATTERN, mqttClient.autoDiscovery.homeAssistantRoot, deviceIdentity.data.uuid);

  char unique_id[MQTT_HEAP_LARGEST_FREE_BLOCK_AUTO_DISCOVERY_UNIQUE_ID_LENGTH+1];
  snprintf(unique_id, sizeof(unique_id), MQTT_HEAP_LARGEST_FREE_BLOCK_AUTO_DISCOVERY_UNIQUE_ID_PATTERN, deviceIdentity.data.uuid);

  char default_entity_id[MQTT_HEAP_LARGEST_FREE_BLOCK_DEFAULT_ENTITY_ID_LENGTH+1];
  snprintf(default_entity_id, sizeof(default_entity_id), MQTT_HEAP_LARGEST_FREE_BLOCK_DEFAULT_ENTITY_ID_PATTERN, deviceIdentity.data.uuid);

  char state_topic[MQTT_TOPIC_HEAP_LARGEST_FREE_BLOCK_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, sizeof(state_topic), MQTT_TOPIC_HEAP_LARGEST_FREE_BLOCK_STATE_PATTERN, deviceIdentity.data.uuid);

  doc["name"] = "Heap Largest Free Block";
  doc["unique_id"] = unique_id;
  doc["default_entity_id"] = default_entity_id;
  doc["icon"] = "mdi:memory";
  doc["entity_category"] = "diagnostic";
  doc["enabled_by_default"] = false;
  doc["unit_of_measurement"] = "B";
  doc["device_class"] = "data_size";
  doc["state_class"] = "measurement";

  JsonObject device = doc["device"].to<JsonObject>();
  JsonArray identifiers = device["identifiers"].to<JsonArray>();
  identifiers.add(deviceIdentity.data.uuid);

  if(strlen(mqttClient.autoDiscovery.deviceName) > 0){
    device["name"] =  mqttClient.autoDiscovery.deviceName;
  }

  device["manufacturer"] = HARDWARE_MANUFACTURER_NAME;
  device["model"] = APPLICATION_NAME;
  device["model_id"] = deviceIdentity.data.product_id;
  device["serial_number"] = deviceIdentity.data.uuid;
  device["sw_version"] = VERSION " (" COMMIT_HASH ")";
  device["configuration_url"] = ("http://" + ETH.localIP().toString()).c_str();

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
 * Broadcasts an MQTT message containing the largest free block of heap
 */
void mqtt_publish_heapLargestFreeBlock(){

  if(deviceIdentity.enabled == false){
    return;
  }

  if(!mqttClient.connected()){
    return;
  }

  char state_topic[MQTT_TOPIC_HEAP_LARGEST_FREE_BLOCK_STATE_PATTERN_LENGTH+1];
  snprintf(state_topic, sizeof(state_topic), MQTT_TOPIC_HEAP_LARGEST_FREE_BLOCK_STATE_PATTERN, deviceIdentity.data.uuid);

  char buf[12];
  sprintf(buf, "%lu", (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));

  mqttClient.publish(state_topic, buf, true);
}


/**
 * Publishes memory usage to MQTT
 */
void mqtt_publishMemoryUsage(){

    mqtt_publish_heapFree();
    mqtt_publish_heapLargestFreeBlock();
}


/**
 * Broadcasts an MQTT message that an output value has changed
 * @param id as the output's unique ID
 * @param value the new value that should be sent to any subscribers
 */
void mqtt_publishOutputValueChanged(char* id, uint8_t value){

  if(!mqttClient.connected()){
    return;
  }

  char state_topic[MQTT_TOPIC_OUTPUT_STATE_LENGTH+1];
  snprintf(state_topic, sizeof(state_topic), MQTT_TOPIC_OUTPUT_STATE_PATTERN, id);

  char value_char[4];
  snprintf(value_char, sizeof(value_char), "%i", value);

  mqttClient.publish(state_topic, value_char, true);
}

/**
 * Starts the HTTP server
 */
void startHttpServer(){

  if(httpServerIsActive == true){
    log_w("HTTP server requested to start when already running.");
    return;
  }

    #if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_W5500 || WIFI_MODEL == ENUM_WIFI_MODEL_ESP32

      if(configFS.exists("/backup.json.upload_in_progress")){
        configFS.remove("/backup.json.upload_in_progress");
      }

      httpServer.begin();
      httpServerIsActive = true;

      eventLog.createEvent("HTTP server started");
      #if CORE_DEBUG_LEVEL >= 4
        reportMemoryUsage("HTTP server started.");
      #endif /* CORE_DEBUG_LEVEL >= 4 */

      resetHTPServerUsage();
      
      mqtt_publishHttpServerStateChanged(httpServerIsActive);

    #endif
}


/**
 * Stops the HTTP server
 */
void stopHttpServer(){

  if(httpServerIsActive == false){
    log_w("HTTP server requested to stop when not running.");
    return;
  }

  #if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_W5500 || WIFI_MODEL == ENUM_WIFI_MODEL_ESP32

    if(configFS.exists("/backup.json.upload_in_progress")){
      configFS.remove("/backup.json.upload_in_progress");
    }

    if(provisioningMode.getStatus()){
      provisioningMode.setInactive();
    }

    httpServer.end();
    httpServerIsActive = false;

    eventLog.createEvent("HTTP server stopped");
    #if CORE_DEBUG_LEVEL >= 4
      reportMemoryUsage("HTTP server stopped.");
    #endif /* CORE_DEBUG_LEVEL >= 4 */

    lastTimeHttpServerUsed = 0;

    mqtt_publishHttpServerStateChanged(httpServerIsActive);

  #endif

}

/**
 * Resets the lastTimeHttpServerUsed variable anytime the HTTP server is successfully called
 * with an authorized request
 */
void resetHTPServerUsage(){
  lastTimeHttpServerUsed = (uint32_t)esp_timer_get_time();
}


// ─── Cloud Backup ─────────────────────────────────────────────────────────────

static bool _cloudAuth_setHeaders(esp_http_client_handle_t client) {
  if (!deviceIdentity.enabled) return false;
  return cloudDeviceAuth_setHeaders(client, deviceIdentity.data.uuid, (time_t)timeClient.getEpochTime());
}


/**
 * Reads /backup.json from configFS, encrypts with key_backup, and uploads to the
 * cloud.  Called automatically by the OTA check loop if /backup.json exists.
 * Silently returns on any error.
 */
/**
 * Reads /backup.json, encrypts it with key_backup, and POSTs it to the
 * cloud backup endpoint with the local ETag header (if one exists).
 * Returns true when the HTTP request completed; httpCode is set to the
 * server's response code.  Returns false on any local failure and sets
 * errorMsg to a description of the problem.
 */
bool cloudBackup_performUpload(int &httpCode, String &errorMsg) {
  httpCode = 0;

  File f = configFS.open("/backup.json", "r");
  if (!f) {
    errorMsg = "Failed to open backup.json";
    return false;
  }

  size_t fileSize = f.size();
  if (fileSize > 512UL * 1024UL) {
    f.close();
    errorMsg = "Backup file exceeds 512KB limit";
    return false;
  }

  uint8_t* plainBuf = (uint8_t*)(psramFound() ? ps_malloc(fileSize) : malloc(fileSize));
  if (!plainBuf) {
    f.close();
    errorMsg = "Out of memory";
    return false;
  }

  if (f.read(plainBuf, fileSize) != fileSize) {
    memset(plainBuf, 0, fileSize);
    free(plainBuf);
    f.close();
    errorMsg = "Failed to read backup.json";
    return false;
  }
  f.close();

  size_t blobLen = 0;
  uint8_t* blob = secretEncryption.encryptBackup(plainBuf, fileSize, blobLen);
  memset(plainBuf, 0, fileSize);
  free(plainBuf);

  if (!blob) {
    errorMsg = "Encryption failed";
    return false;
  }

  String url = FIREFLY_CLOUD_API_ROOT;
  url += "/devices/";
  url += deviceIdentity.data.uuid;
  url += "/backup";

  esp_http_client_config_t cfg = {};
  cfg.url               = url.c_str();
  cfg.crt_bundle_attach = esp_crt_bundle_attach;
  cfg.timeout_ms        = 30000;
  cfg.method            = HTTP_METHOD_POST;
  cfg.buffer_size_tx    = 2048;

  esp_http_client_handle_t client = esp_http_client_init(&cfg);

  if (!_cloudAuth_setHeaders(client)) {
    esp_http_client_cleanup(client);
    memset(blob, 0, blobLen);
    free(blob);
    errorMsg = "Auth header build failed";
    return false;
  }

  esp_http_client_set_header(client, "Content-Type", "application/octet-stream");

  if (configFS.exists("/backup.etag")) {
    File etagFile = configFS.open("/backup.etag", "r");
    if (etagFile) {
      String etag = etagFile.readString();
      etagFile.close();
      esp_http_client_set_header(client, "ETag", ("\"" + etag + "\"").c_str());
    }
  }

  esp_http_client_set_post_field(client, (const char*)blob, blobLen);

  esp_err_t err = esp_http_client_perform(client);
  httpCode      = esp_http_client_get_status_code(client);
  esp_http_client_cleanup(client);

  memset(blob, 0, blobLen);
  free(blob);

  if (err != ESP_OK) {
    errorMsg = "Cloud request failed";
    return false;
  }

  return true;
}


void cloudBackup_scheduleHandler() {

  if (!deviceIdentity.enabled) return;
  if (!secretEncryption.isReady()) return;
  if (!timeClient.isTimeSet()) return;

  eventLog.createEvent("Backup upload start");

  int    httpCode = 0;
  String errorMsg;

  if (!cloudBackup_performUpload(httpCode, errorMsg)) {
    log_e("cloudBackup_scheduleHandler: %s", errorMsg.c_str());
    eventLog.createEvent("Backup upload fail", EventLog::LOG_LEVEL_ERROR);
    return;
  }

  log_i("cloudBackup_scheduleHandler: status=%d", httpCode);

  if (httpCode == 200 || httpCode == 204 || httpCode == 304) {
    eventLog.resolveError("Backup upload fail");
    eventLog.createEvent("Backup uploaded");
  } else {
    char text[OLED_CHARACTERS_PER_LINE + 1];
    snprintf(text, sizeof(text), "Backup up fail %d", httpCode);
    eventLog.createEvent(text, EventLog::LOG_LEVEL_NOTIFICATION);
  }
}


/**
 * Generic dispatcher for /api/cloud-backup
 */
void http_handleCloudBackup(AsyncWebServerRequest *request) {
  switch(request->method()) {

    case HTTP_OPTIONS:
      http_options(request);
      break;

    case HTTP_POST:
      http_handleCloudBackup_POST(request);
      break;

    case HTTP_GET:
      http_handleCloudBackup_GET(request);
      break;

    case HTTP_DELETE:
      http_handleCloudBackup_DELETE(request);
      break;

    default:
      http_methodNotAllowed(request);
      break;
  }
}


/**
 * POST /api/cloud-backup
 * Reads backup.json from configFS, encrypts with key_backup, uploads to cloud.
 * Responds with the cloud HTTP status code.
 */
void http_handleCloudBackup_POST(AsyncWebServerRequest *request) {

  if (!request->hasHeader("visual-token") ||
      !authToken.authenticate(request->header("visual-token").c_str())) {
    http_unauthorized(request);
    return;
  }

  if (!deviceIdentity.enabled) {
    http_conflict(request, "Device not provisioned");
    return;
  }

  if (!secretEncryption.isReady()) {
    http_error(request, "Encryption not ready");
    return;
  }

  if (!timeClient.isTimeSet()) {
    http_serviceUnavailable(request, "Clock not synchronized");
    return;
  }

  if (!configFS.exists("/backup.json")) {
    http_notFound(request);
    return;
  }

  resetHTPServerUsage();

  int    httpCode = 0;
  String errorMsg;

  if (!cloudBackup_performUpload(httpCode, errorMsg)) {
    http_error(request, errorMsg.c_str());
    return;
  }

  log_i("http_handleCloudBackup_POST: status=%d", httpCode);

  AsyncResponseStream *resp = request->beginResponseStream("application/json");
  JsonDocument doc;
  doc["status"] = httpCode;
  serializeJson(doc, *resp);
  resp->setCode(httpCode == 200 || httpCode == 204 || httpCode == 304 ? 200 : 502);
  request->send(resp);
}


/**
 * GET /api/cloud-backup
 * Downloads the encrypted backup from cloud, decrypts with key_backup,
 * writes to /backup.json on configFS, returns 200 with the plaintext JSON.
 */
void http_handleCloudBackup_GET(AsyncWebServerRequest *request) {

  if (!request->hasHeader("visual-token") ||
      !authToken.authenticate(request->header("visual-token").c_str())) {
    http_unauthorized(request);
    return;
  }

  if (!deviceIdentity.enabled) {
    http_conflict(request, "Device not provisioned");
    return;
  }

  if (!secretEncryption.isReady()) {
    http_error(request, "Encryption not ready");
    return;
  }

  if (!timeClient.isTimeSet()) {
    http_serviceUnavailable(request, "Clock not synchronized");
    return;
  }

  resetHTPServerUsage();

  String url = FIREFLY_CLOUD_API_ROOT;
  url += "/devices/";
  url += deviceIdentity.data.uuid;
  url += "/backup";

  esp_http_client_config_t cfg = {};
  cfg.url               = url.c_str();
  cfg.crt_bundle_attach = esp_crt_bundle_attach;
  cfg.timeout_ms        = 30000;
  cfg.method            = HTTP_METHOD_GET;

  esp_http_client_handle_t client = esp_http_client_init(&cfg);

  if (!_cloudAuth_setHeaders(client)) {
    esp_http_client_cleanup(client);
    http_error(request, "Auth header build failed");
    return;
  }

  // Open connection and read the response into a buffer
  esp_err_t err = esp_http_client_open(client, 0);
  if (err != ESP_OK) {
    esp_http_client_cleanup(client);
    http_error(request, "Cloud connection failed");
    return;
  }

  int64_t contentLength = esp_http_client_fetch_headers(client);
  int     code          = esp_http_client_get_status_code(client);

  // Capture ETag from response headers before client is closed
  String etagFromCloud;
  {
    char* etagValue = nullptr;
    if(esp_http_client_get_header(client, "ETag", &etagValue) == ESP_OK && etagValue != nullptr){
      etagFromCloud = String(etagValue);
      etagFromCloud.replace("\"", "");
    }
  }

  if (code == 404) {
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    http_notFound(request);
    return;
  }

  if (code != 200 || contentLength <= 0 || contentLength > 512LL * 1024LL + 37LL) {
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    AsyncResponseStream *resp = request->beginResponseStream("application/json");
    JsonDocument doc;
    doc["message"] = "Cloud returned unexpected response";
    doc["status"]  = code;
    serializeJson(doc, *resp);
    resp->setCode(502);
    request->send(resp);
    return;
  }

  uint8_t* blobBuf = (uint8_t*)(psramFound() ? ps_malloc((size_t)contentLength) : malloc((size_t)contentLength));
  if (!blobBuf) {
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    http_error(request, "Out of memory");
    return;
  }

  int bytesRead = esp_http_client_read(client, (char*)blobBuf, (int)contentLength);
  esp_http_client_close(client);
  esp_http_client_cleanup(client);

  if (bytesRead != (int)contentLength) {
    memset(blobBuf, 0, (size_t)contentLength);
    free(blobBuf);
    http_error(request, "Incomplete read from cloud");
    return;
  }

  String plaintext;
  if (!secretEncryption.decryptBackup(blobBuf, (size_t)contentLength, plaintext)) {
    memset(blobBuf, 0, (size_t)contentLength);
    free(blobBuf);
    eventLog.createEvent("Backup decrypt fail", EventLog::LOG_LEVEL_ERROR);
    http_error(request, "Decryption failed");
    return;
  }

  memset(blobBuf, 0, (size_t)contentLength);
  free(blobBuf);

  // Write restored backup.json to configFS
  File outFile = configFS.open("/backup.json", "w");
  if (!outFile) {
    http_error(request, "Failed to write backup.json");
    return;
  }
  outFile.print(plaintext);
  outFile.close();

  // Persist ETag sidecar if cloud provided one
  if(etagFromCloud.length() == 64){
    if(configFS.exists("/backup.etag")){
      configFS.remove("/backup.etag");
    }
    File etagFile = configFS.open("/backup.etag", "w");
    if(etagFile){
      etagFile.print(etagFromCloud);
      etagFile.close();
    }
  }

  request->send(200, "application/json", plaintext);
}


/**
 * DELETE /api/cloud-backup
 * Sends DELETE to cloud and removes the local /backup.json if it exists.
 */
void http_handleCloudBackup_DELETE(AsyncWebServerRequest *request) {

  if (!request->hasHeader("visual-token") ||
      !authToken.authenticate(request->header("visual-token").c_str())) {
    http_unauthorized(request);
    return;
  }

  if (!deviceIdentity.enabled) {
    http_conflict(request, "Device not provisioned");
    return;
  }

  if (!timeClient.isTimeSet()) {
    http_serviceUnavailable(request, "Clock not synchronized");
    return;
  }

  resetHTPServerUsage();

  String url = FIREFLY_CLOUD_API_ROOT;
  url += "/devices/";
  url += deviceIdentity.data.uuid;
  url += "/backup";

  esp_http_client_config_t cfg = {};
  cfg.url               = url.c_str();
  cfg.crt_bundle_attach = esp_crt_bundle_attach;
  cfg.timeout_ms        = 15000;
  cfg.method            = HTTP_METHOD_DELETE;

  esp_http_client_handle_t client = esp_http_client_init(&cfg);

  if (!_cloudAuth_setHeaders(client)) {
    esp_http_client_cleanup(client);
    http_error(request, "Auth header build failed");
    return;
  }

  esp_err_t err  = esp_http_client_perform(client);
  int       code = esp_http_client_get_status_code(client);
  esp_http_client_cleanup(client);

  if (err != ESP_OK) {
    http_error(request, "Cloud request failed");
    return;
  }

  if (code == 200 || code == 204) {
    if (configFS.exists("/backup.json")) {
      configFS.remove("/backup.json");
    }
    request->send(204);
  } else if (code == 404) {
    http_notFound(request);
  } else {
    AsyncResponseStream *resp = request->beginResponseStream("application/json");
    JsonDocument doc;
    doc["message"] = "Cloud returned unexpected status";
    doc["status"]  = code;
    serializeJson(doc, *resp);
    resp->setCode(502);
    request->send(resp);
  }
}