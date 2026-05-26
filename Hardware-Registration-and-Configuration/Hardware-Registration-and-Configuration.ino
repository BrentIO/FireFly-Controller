/**
 * Hardware Registration and Configuration.ino
 * 
 * Application allows for the reading of the device identity, as well as programming of the device identity data.  This is
 * separated from the main program to ensure the device identity can't be accidentally configured and lose the device information.
 * 
 * Exposes a set of API's that are described in the openapi.yaml file.
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
  #ifndef COMMIT_HASH
    #define COMMIT_HASH "DEBUG"
  #endif
#endif
#define APPLICATION_NAME "HW Reg and Config"

#if BURN_VDD_SDIO_EFUSE
  #include "esp_efuse.h"
  #include "esp_efuse_table.h"
#endif

#include "common/hardware.h"
#include "common/cloudConfig.h"
#include "common/deviceIdentity.h"
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
#include "AsyncJson.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <mbedtls/sha256.h>
#include <mbedtls/hkdf.h>
#include <mbedtls/md.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/base64.h>
#include <esp_http_client.h>
#include <esp_crt_bundle.h>

unsigned long bootTime = 0; /* Approximate Epoch time the device booted */
AsyncWebServer httpServer(80);
exEsp32FOTA otaFirmware(APPLICATION_NAME, VERSION, false);
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

#define DEVICE_CLASS "CONTROLLER"
#define REGISTRATION_APPLICATION_NAME "Hardware-Registration-and-Configuration"

struct {
  bool   registered    = false;
  bool   error         = false;
  String errorMessage;
  time_t checkedAt     = 0;
} _registrationState;

struct {
  String  json;
  bool    ready     = false;
  bool    pending   = false;
  int64_t lastFetch = 0;
} _firmwareState;

/* Hardware RNG wrapper for mbedtls ECP/ECDSA calls */
static int _espRng(void*, unsigned char* buf, size_t len) {
    esp_fill_random(buf, len);
    return 0;
}

void updateNTPTime(bool force = false);
void fetchFirmwareList();
void otaFirmware_checkPending();
void eventHandler_otaFirmwareProgress(size_t progress, size_t size);
void eventHandler_otaFirmwareFailed(int partition);
void eventHandler_otaFirmwareFinished(int partition, bool needs_restart);

fs::LittleFSFS uiFS;
bool uiFS_isMounted = false;


/**
 * One-time setup
*/
void setup() {

  #if BURN_VDD_SDIO_EFUSE
    if (!esp_efuse_read_field_bit(ESP_EFUSE_XPD_SDIO_FORCE) ||
        !esp_efuse_read_field_bit(ESP_EFUSE_XPD_SDIO_REG) ||
        !esp_efuse_read_field_bit(ESP_EFUSE_XPD_SDIO_TIEH)) {
      esp_efuse_write_field_bit(ESP_EFUSE_XPD_SDIO_FORCE);
      esp_efuse_write_field_bit(ESP_EFUSE_XPD_SDIO_REG);
      esp_efuse_write_field_bit(ESP_EFUSE_XPD_SDIO_TIEH);
      log_i("VDD_SDIO eFuse burned; rebooting");
      delay(100);
      ESP.restart();
    }
    log_i("VDD_SDIO eFuse: set");
  #endif

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

    oled.setWiFiInfo(&WiFi);

  #endif

  #if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_W5500 && defined(ESP32)

    ESP32_W5500_onEvent();
    ESP32_W5500_setCallback_connected(&eventHandler_ethernetConnect);
    ESP32_W5500_setCallback_disconnected(&eventHandler_ethernetDisconnect);

    log_i("Setting up Ethernet on W5500");

    ETH.setHostname(hostname);
    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);

    uint32_t ethernet_start_time = (uint32_t)(esp_timer_get_time() / 1000ULL);
    ETH.begin(ETH_PHY_W5500, 1, ETHERNET_PIN, ETHERNET_PIN_INTERRUPT, ETHERNET_PIN_RESET,
              ETH_SPI_HOST, SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SPI_CLOCK_MHZ);

    while(!ESP32_W5500_isConnected()){

      delay(100);

      if((uint32_t)(esp_timer_get_time() / 1000ULL) - ethernet_start_time > ETHERNET_TIMEOUT){
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


  /* Start device identity (eFuse) */
  deviceIdentity.begin();

  if(deviceIdentity.enabled == true){

    oled.setProductID(deviceIdentity.data.product_id);
    oled.setUUID(deviceIdentity.data.uuid);

    log_i("eFuse UUID: %s", deviceIdentity.data.uuid);
    log_i("eFuse Product ID: %s", deviceIdentity.data.product_id);
    log_i("eFuse master key bytes 0-3: %02x %02x %02x %02x",
          deviceIdentity.data.key[0], deviceIdentity.data.key[1],
          deviceIdentity.data.key[2], deviceIdentity.data.key[3]);
    memset(deviceIdentity.data.key, 0, sizeof(deviceIdentity.data.key));
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


  /* Start LittleFS for ui */
  if (uiFS.begin(false, "/uiFS", (uint8_t)10U, "ui"))
  {
    uiFS_isMounted = true;
  }
  else{
    eventLog.createEvent("uiFS mount fail", EventLog::LOG_LEVEL_ERROR);
    log_e("An Error has occurred while mounting ui");
  }



  /* Configure the web server.  
    IMPORTANT: *** Sequence below matters, they are sorted specific to generic *** 
  */
  AsyncCallbackJsonWebHandler *jsonHandler_handleIdentity_POST = new AsyncCallbackJsonWebHandler("/api/identity", http_handleIdentity_POST);
  jsonHandler_handleIdentity_POST->setMethod(HTTP_POST);
  httpServer.addHandler(jsonHandler_handleIdentity_POST);
  httpServer.on("/api/identity", http_handleIdentity);
  AsyncCallbackJsonWebHandler *jsonHandler_handleRegistration_POST = new AsyncCallbackJsonWebHandler("/api/registration", http_handleRegistration_POST);
  jsonHandler_handleRegistration_POST->setMethod(HTTP_POST);
  httpServer.addHandler(jsonHandler_handleRegistration_POST);
  httpServer.on("/api/registration", http_handleRegistration_GET);
  httpServer.on("^/api/mcu$", http_handleMCU);
  httpServer.on("^/api/mcu/reboot$", http_handleReboot);
  httpServer.on("/api/partitions", http_handlePartitions);
  httpServer.on("/api/peripherals", http_handlePeripherals);
  httpServer.on("/api/version", http_handleVersion);
  httpServer.on("/api/events", http_handleEventLog);
  httpServer.on("/api/errors", http_handleErrorLog);
  httpServer.on("^/api/network/([a-z_]+)$", http_handleNetworkInterface);
  httpServer.on("/api/network", http_handleNetworkInterfaceAll);
  httpServer.on("/api/firmware", http_handleFirmware);
  AsyncCallbackJsonWebHandler *jsonHandler_handleOTA_app_POST = new AsyncCallbackJsonWebHandler("/api/ota/app", http_handleOTA_forced_POST);
  jsonHandler_handleOTA_app_POST->setMethod(HTTP_POST);
  httpServer.addHandler(jsonHandler_handleOTA_app_POST);
  AsyncCallbackJsonWebHandler *jsonHandler_handleOTA_ui_POST = new AsyncCallbackJsonWebHandler("/api/ota/ui", http_handleOTA_forced_POST);
  jsonHandler_handleOTA_ui_POST->setMethod(HTTP_POST);
  httpServer.addHandler(jsonHandler_handleOTA_ui_POST);

  httpServer.on("/auth", http_handleAuth);

  if(uiFS_isMounted){
    httpServer.serveStatic("/", uiFS, "/", "public, max-age=86400");
    httpServer.rewrite("/", "/index.html");
  }

  httpServer.onNotFound(http_notFound);

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*"); //Ignore CORS
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "visual-token, Content-Type, X-Registration-Key"); //Ignore CORS
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS, DELETE"); //Ignore CORS

  String serverHeader = String(APPLICATION_NAME);
  serverHeader.replace(" ", "-");
  serverHeader += "/" + String(VERSION);
  DefaultHeaders::Instance().addHeader("Server", serverHeader);

  #if WIFI_MODEL == ENUM_WIFI_MODEL_ESP32

    if(WiFi.getMode() != wifi_mode_t::WIFI_MODE_NULL){
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
}


/**
 * Main loop
*/
void loop() {

  updateNTPTime();

  if (_registrationState.checkedAt == 0 && deviceIdentity.enabled && timeClient.isTimeSet()) {
    checkCloudRegistration();
  }

  if (_firmwareState.pending && deviceIdentity.enabled) {
    fetchFirmwareList();
  }

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
 * Sends a 409 response indicating a conflict (e.g. resource already exists and cannot be overwritten)
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
 * Sends a 403 response indicating the caller is authenticated but not permitted
*/
void http_forbidden(AsyncWebServerRequest *request, String message){

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc;
  doc["message"] = message;

  serializeJson(doc, *response);
  response->setCode(403);
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
 * Sends a 502 response indicating an upstream gateway error
*/
void http_badGateway(AsyncWebServerRequest *request, String message){

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc;
  doc["message"] = message;

  serializeJson(doc, *response);
  response->setCode(502);
  request->send(response);
}


/**
 * Sends a 200 response for any OPTIONS requests
*/
void http_options(AsyncWebServerRequest *request) {
  request->send(200);
};


/**
 * Handles partitions requests
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
  doc["cpu_freq_mhz"] = ESP.getCpuFreqMHz();
  doc["chip_cores"] = ESP.getChipCores();
  doc["sdk_version"] = ESP.getSdkVersion();
  doc["flash_chip_size"] = ESP.getFlashChipSize();
  doc["flash_chip_speed"] = ESP.getFlashChipSpeed() / 1000000;
  const char* flashModeNames[] = {"QIO", "QOUT", "DIO", "DOUT", "FAST_READ", "SLOW_READ"};
  int flashModeIdx = (int)ESP.getFlashChipMode();
  doc["flash_chip_mode"] = (flashModeIdx >= 0 && flashModeIdx <= 5) ? flashModeNames[flashModeIdx] : "UNKNOWN";
  doc["free_heap"] = ESP.getFreeHeap();
  doc["psram_size"] = ESP.getPsramSize();
  doc["free_psram"] = ESP.getFreePsram();

  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  JsonArray featuresArr = doc["chip_features"].to<JsonArray>();
  if (chip_info.features & CHIP_FEATURE_EMB_FLASH)  featuresArr.add("Embedded-Flash");
  if (chip_info.features & CHIP_FEATURE_WIFI_BGN)   featuresArr.add("WiFi-bgn");
  if (chip_info.features & CHIP_FEATURE_BLE)        featuresArr.add("BLE");
  if (chip_info.features & CHIP_FEATURE_BT)         featuresArr.add("BT");
  if (chip_info.features & CHIP_FEATURE_EMB_PSRAM)  featuresArr.add("Embedded-PSRAM");
  if (chip_info.features & CHIP_FEATURE_IEEE802154) featuresArr.add("IEEE-802.15.4");

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
 * Fetches the list of released Controller firmware from FireFly-Cloud for this device's product.
 * Called from loop() when _firmwareState.pending is set. Stores up to 5 most recent versions
 * (newest-first) in _firmwareState.json.
*/
void fetchFirmwareList() {

  _firmwareState.pending = false;

  char url[128];
  snprintf(url, sizeof(url), "%s/ota/controller/0x%08" PRIx32 "/controller",
           FIREFLY_CLOUD_API_ROOT, deviceIdentity.data.product_hex);

  esp_http_client_config_t cfg = {};
  cfg.url                = url;
  cfg.crt_bundle_attach  = esp_crt_bundle_attach;
  cfg.timeout_ms         = 10000;

  esp_http_client_handle_t client = esp_http_client_init(&cfg);
  esp_err_t err = esp_http_client_open(client, 0);

  if (err != ESP_OK) {
    esp_http_client_cleanup(client);
    eventLog.createEvent("Firmware list failed", EventLog::LOG_LEVEL_NOTIFICATION);
    return;
  }

  esp_http_client_fetch_headers(client);
  int status = esp_http_client_get_status_code(client);

  if (status == 200) {
    const int BUF_SIZE = 4096;
    char* buf = new char[BUF_SIZE];
    if (buf) {
      int total = 0, read_len;
      while ((read_len = esp_http_client_read(client, buf + total, BUF_SIZE - total - 1)) > 0) {
        total += read_len;
        if (total >= BUF_SIZE - 1) break;
      }
      buf[total] = '\0';

      JsonDocument parsed;
      DeserializationError parseErr = deserializeJson(parsed, buf);
      delete[] buf;

      if (!parseErr && parsed["versions"].is<JsonArray>()) {
        JsonArray versions = parsed["versions"].as<JsonArray>();
        int count = versions.size();
        int start = (count > 5) ? count - 5 : 0;

        JsonDocument out;
        JsonArray arr = out["versions"].to<JsonArray>();
        for (int i = count - 1; i >= start; i--) {
          arr.add(versions[i]);
        }

        _firmwareState.json = "";
        serializeJson(out, _firmwareState.json);
        _firmwareState.ready = true;
        eventLog.createEvent("Firmware list fetched", EventLog::LOG_LEVEL_INFO);
      }
    } else {
      delete[] buf;
    }
  } else if (status == 404) {
    /* No released firmware for this product — return an empty list */
    _firmwareState.json = "{\"versions\":[]}";
    _firmwareState.ready = true;
  }

  esp_http_client_close(client);
  esp_http_client_cleanup(client);
}


/**
 * GET /api/firmware — returns the list of released Controller firmware for this device.
 * Always triggers a fresh cloud fetch unless one is already in flight (rate-limited to
 * once per 5 seconds). Returns 202 while the fetch is pending, 200 with data when ready.
*/
void http_handleFirmware(AsyncWebServerRequest *request) {

  if (request->method() != HTTP_GET) {
    http_methodNotAllowed(request);
    return;
  }

  if (!request->hasHeader("visual-token") ||
      !authToken.authenticate(request->header("visual-token").c_str())) {
    http_unauthorized(request);
    return;
  }

  if (!deviceIdentity.enabled) {
    http_conflict(request, "Device not provisioned");
    return;
  }

  if (esp_timer_get_time() - _firmwareState.lastFetch >= 5000000LL) {
    _firmwareState.ready     = false;
    _firmwareState.pending   = true;
    _firmwareState.lastFetch = esp_timer_get_time();
  }

  if (!_firmwareState.ready) {
    request->send(202);
    return;
  }

  request->send(200, "application/json", _firmwareState.json);
}


/**
 * POST /api/ota/app — queues a forced OTA app firmware update from the provided URL.
 * POST /api/ota/ui  — queues a forced OTA UI firmware update from the provided URL.
*/
void http_handleOTA_forced_POST(AsyncWebServerRequest *request, JsonVariant &doc) {

  if (!request->hasHeader("visual-token") ||
      !authToken.authenticate(request->header("visual-token").c_str())) {
    http_unauthorized(request);
    return;
  }

  if (request->method() != HTTP_POST) {
    http_methodNotAllowed(request);
    return;
  }

  if (doc["url"].isNull()) {
    http_badRequest(request, "Field url is required");
    return;
  }

  forcedOtaUpdateConfig newFirmwareRequest;
  newFirmwareRequest.url = doc["url"].as<String>();

  if (!newFirmwareRequest.url.endsWith(".bin")) {
    http_badRequest(request, "Bad url; File must be of type '.bin'");
    return;
  }

  if (!newFirmwareRequest.url.startsWith("http:") && !newFirmwareRequest.url.startsWith("https:")) {
    http_badRequest(request, "Bad url; http or https required");
    return;
  }

  newFirmwareRequest.type = request->url().endsWith("app") ? OTA_UPDATE_APP : OTA_UPDATE_UI;
  otaFirmware.pending.add(newFirmwareRequest);

  request->send(202);
}


/**
 * Checks if there are any pending OTA firmware updates in the queue.
 * This is necessary to allow the async web server to raise requests to the main loop.
*/
void otaFirmware_checkPending() {

  if (otaFirmware.pending.size() == 0) {
    return;
  }

  if (otaFirmware.updateInProcess == true) {
    return;
  }

  while (otaFirmware.pending.size() > 0) {

    exEsp32FOTA::esp32FOTA forceFirmwareUpdate;
    forceFirmwareUpdate.setProgressCb(eventHandler_otaFirmwareProgress);
    forceFirmwareUpdate.setUpdateBeginFailCb(eventHandler_otaFirmwareFailed);
    forceFirmwareUpdate.setUpdateFinishedCb(eventHandler_otaFirmwareFinished);
    forceFirmwareUpdate.setSPIFFsPartitionLabel("ui");
    forceFirmwareUpdate.setCertFileSystem(nullptr);

    if (otaFirmware.pending.get(0).url.startsWith("https:")) {
      forceFirmwareUpdate.useBundledCerts();
    }

    bool updateSuccess = false;
    if (otaFirmware.pending.get(0).type == OTA_UPDATE_UI) {
      eventLog.createEvent("OTA UI forced", EventLog::LOG_LEVEL_NOTIFICATION);
      updateSuccess = forceFirmwareUpdate.forceUpdateSPIFFS(otaFirmware.pending.get(0).url.c_str(), false);
    } else {
      eventLog.createEvent("OTA app forced", EventLog::LOG_LEVEL_NOTIFICATION);
      updateSuccess = forceFirmwareUpdate.forceUpdate(otaFirmware.pending.get(0).url.c_str(), false);
    }

    otaFirmware.pending.remove(0);

    if (!updateSuccess) {
      eventLog.createEvent("OTA update failed", EventLog::LOG_LEVEL_NOTIFICATION);
      while (otaFirmware.pending.size() > 0) {
        otaFirmware.pending.remove(0);
      }
      return;
    }
  }
}


/**
 * A callback function to report firmware upgrade progress, which draws a status on the OLED.
 * @param progress The number of bytes that have been processed so far
 * @param size The total size of the update in bytes
*/
void eventHandler_otaFirmwareProgress(size_t progress, size_t size) {

  otaFirmware.updateInProcess = true;

  float percentage = ((float)progress / (float)size);

  if (progress == 0) {
    eventLog.createEvent("OTA firmware started", EventLog::LOG_LEVEL_NOTIFICATION);
    oled.setPage(managerOled::PAGE_OTA_IN_PROGRESS);
  }

  oled.setProgressBar(percentage);
}


/**
 * Callback function to report that the OTA update has failed.
 * @param partition inherited from esp32FOTA library
*/
void eventHandler_otaFirmwareFailed(int partition) {
  log_e("OTA failed partition: [%i]", partition);
  eventLog.createEvent("OTA firmware failed", EventLog::LOG_LEVEL_NOTIFICATION);
  otaFirmware.updateInProcess = false;
}


/**
 * A callback function for when the OTA firmware update has finished.
*/
void eventHandler_otaFirmwareFinished(int partition, bool needs_restart) {
  log_i("OTA finished partition: [%i] needs restart: [%s]", partition, needs_restart ? "true" : "false");
  eventLog.createEvent("OTA update finished", EventLog::LOG_LEVEL_NOTIFICATION);

  if (needs_restart) {
    eventLog.createEvent("Rebooting...", EventLog::LOG_LEVEL_NOTIFICATION);
    delay(5000);
  }

  otaFirmware.updateInProcess = false;
}


/**
 * Checks device registration status with FireFly-Cloud using a signed nonce.
 * Called once from loop() after NTP sync. Updates _registrationState in-place.
*/
void checkCloudRegistration() {
  eventLog.createEvent("Cloud reg check", EventLog::LOG_LEVEL_INFO);

  String url = FIREFLY_CLOUD_API_ROOT;
  url += "/devices/";
  url += deviceIdentity.data.uuid;
  url += "/registration";

  esp_http_client_config_t cfg = {};
  cfg.url                = url.c_str();
  cfg.crt_bundle_attach  = esp_crt_bundle_attach;
  cfg.timeout_ms         = 10000;

  esp_http_client_handle_t client = esp_http_client_init(&cfg);

  _registrationState.checkedAt = timeClient.getEpochTime();

  if (!cloudDeviceAuth_setHeaders(client, deviceIdentity.data.uuid, (time_t)timeClient.getEpochTime())) {
    esp_http_client_cleanup(client);
    eventLog.createEvent("Cloud reg fail", EventLog::LOG_LEVEL_NOTIFICATION);
    return;
  }

  int  status  = 0;
  char bodyBuf[256] = {0};

  esp_err_t err = esp_http_client_open(client, 0);
  if (err == ESP_OK) {
    esp_http_client_fetch_headers(client);
    status = esp_http_client_get_status_code(client);
    int bodyLen = esp_http_client_read(client, bodyBuf, sizeof(bodyBuf) - 1);
    if (bodyLen > 0) {
      log_i("checkCloudRegistration: response body: %s", bodyBuf);
    }
  }
  esp_http_client_close(client);
  esp_http_client_cleanup(client);

  log_i("checkCloudRegistration: url=%s err=%d status=%d", url.c_str(), (int)err, status);

  _registrationState.error        = false;
  _registrationState.errorMessage = String();

  if (err != ESP_OK) {
    _registrationState.registered   = false;
    _registrationState.error        = true;
    _registrationState.errorMessage = "Cloud unreachable";
    eventLog.createEvent("Cloud reg fail", EventLog::LOG_LEVEL_NOTIFICATION);
  } else if (status == 200) {
    _registrationState.registered = true;
    eventLog.createEvent("Cloud registered", EventLog::LOG_LEVEL_INFO);
  } else if (status == 404) {
    _registrationState.registered = false;
  } else {
    _registrationState.registered = false;
    _registrationState.error      = true;
    JsonDocument cloudBody;
    if (strlen(bodyBuf) > 0 && deserializeJson(cloudBody, bodyBuf) == DeserializationError::Ok) {
      _registrationState.errorMessage = cloudBody["message"].as<String>();
    } else {
      _registrationState.errorMessage = "Cloud verification failed";
    }
    if (status == 401) {
      eventLog.createEvent("Cloud reg sig vf fail", EventLog::LOG_LEVEL_ERROR);
    } else {
      eventLog.createEvent("Cloud reg fail", EventLog::LOG_LEVEL_NOTIFICATION);
    }
  }
}


/**
 * GET /api/registration — returns in-RAM cloud registration state
*/
void http_handleRegistration_GET(AsyncWebServerRequest *request) {
  if (!request->hasHeader("visual-token") ||
      !authToken.authenticate(request->header("visual-token").c_str())) {
    http_unauthorized(request);
    return;
  }

  if (!deviceIdentity.enabled) {
    http_conflict(request, "Device not provisioned");
    return;
  }

  if (_registrationState.checkedAt == 0) {
    http_serviceUnavailable(request, "Registration check pending");
    return;
  }

  if (_registrationState.error) {
    http_badGateway(request, _registrationState.errorMessage);
    return;
  }

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc;
  doc["registered"]     = _registrationState.registered;
  doc["checked_at"]     = (long)_registrationState.checkedAt;
  doc["cloud_api_root"] = FIREFLY_CLOUD_API_ROOT;
  serializeJson(doc, *response);
  request->send(response);
}


/**
 * POST /api/registration — registers device with FireFly-Cloud.
 * Requires X-Registration-Key header (6-char one-time code) and optional { "url": "..." } body.
*/
void http_handleRegistration_POST(AsyncWebServerRequest *request, JsonVariant &doc) {
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

  if (!request->hasHeader("X-Registration-Key") ||
      strlen(request->header("X-Registration-Key").c_str()) != 6) {
    http_badRequest(request, "X-Registration-Key header is required and must be 6 characters");
    return;
  }

  String regKey = request->header("X-Registration-Key");

  String cloudUrl = FIREFLY_CLOUD_API_ROOT;
  if (!doc.isNull() && !doc["url"].isNull()) {
    cloudUrl = doc["url"].as<String>();
  }

  /* Derive key_auth and compute P-256 public key */
  uint8_t master_key[32] = {};
  esp_efuse_read_block(EFUSE_BLK3, master_key, EFUSE_KEY_OFFSET_BITS, EFUSE_KEY_SIZE_BITS);
  uint8_t key_auth[32];
  if (mbedtls_hkdf(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
                   nullptr, 0,
                   master_key, sizeof(master_key),
                   (const uint8_t*)"firefly-auth-v1", 15,
                   key_auth, 32) != 0) {
    memset(master_key, 0, 32);
    memset(key_auth, 0, 32);
    http_error(request, "Key derivation failed");
    return;
  }
  memset(master_key, 0, 32);

  mbedtls_ecp_group  grp;  mbedtls_ecp_group_init(&grp);
  mbedtls_mpi        d;    mbedtls_mpi_init(&d);
  mbedtls_ecp_point  Q;    mbedtls_ecp_point_init(&Q);

  mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);
  mbedtls_mpi_read_binary(&d, key_auth, 32);
  memset(key_auth, 0, 32);

  bool keyOk = (mbedtls_ecp_mul(&grp, &Q, &d, &grp.G, _espRng, nullptr) == 0);

  uint8_t pubKeyRaw[65]; size_t pubKeyLen = 0;
  if (keyOk) {
    keyOk = (mbedtls_ecp_point_write_binary(&grp, &Q,
                                            MBEDTLS_ECP_PF_UNCOMPRESSED,
                                            &pubKeyLen, pubKeyRaw, 65) == 0);
  }

  mbedtls_ecp_group_free(&grp);
  mbedtls_mpi_free(&d);
  mbedtls_ecp_point_free(&Q);

  if (!keyOk) {
    http_error(request, "Public key computation failed");
    return;
  }

  uint8_t pubKeyB64[92]; size_t pubKeyB64Len = 0;
  mbedtls_base64_encode(pubKeyB64, sizeof(pubKeyB64), &pubKeyB64Len, pubKeyRaw, pubKeyLen);

  /* Build registration payload */
  char product_hex_str[11] = {0};
  sprintf(product_hex_str, "0x%08lX", deviceIdentity.data.product_hex);

  JsonDocument payloadDoc;
  payloadDoc["uuid"]                    = deviceIdentity.data.uuid;
  payloadDoc["product_id"]              = deviceIdentity.data.product_id;
  payloadDoc["product_hex"]             = product_hex_str;
  payloadDoc["device_class"]            = DEVICE_CLASS;
  payloadDoc["public_key"]              = (char*)pubKeyB64;
  payloadDoc["registering_application"] = REGISTRATION_APPLICATION_NAME;
  payloadDoc["registering_version"]     = VERSION;

  JsonObject mcu = payloadDoc["mcu"].to<JsonObject>();
  mcu["model"]           = ESP.getChipModel();
  mcu["revision"]        = (int)ESP.getChipRevision();
  mcu["cpu_freq_mhz"]    = ESP.getCpuFreqMHz();
  mcu["cores"]           = ESP.getChipCores();
  mcu["flash_chip_size"] = ESP.getFlashChipSize();
  mcu["flash_chip_speed"]= ESP.getFlashChipSpeed() / 1000000;
  int flashModeIdx = (int)ESP.getFlashChipMode();
  const char* flashModeNames[] = {"QIO","QOUT","DIO","DOUT","FAST_READ","SLOW_READ"};
  mcu["flash_chip_mode"] = (flashModeIdx >= 0 && flashModeIdx <= 5) ? flashModeNames[flashModeIdx] : "UNKNOWN";
  mcu["psram_size"]      = ESP.getPsramSize();

  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  JsonArray features = mcu["features"].to<JsonArray>();
  if (chip_info.features & CHIP_FEATURE_EMB_FLASH)  features.add("Embedded-Flash");
  if (chip_info.features & CHIP_FEATURE_WIFI_BGN)   features.add("WiFi-bgn");
  if (chip_info.features & CHIP_FEATURE_BLE)        features.add("BLE");
  if (chip_info.features & CHIP_FEATURE_BT)         features.add("BT");
  if (chip_info.features & CHIP_FEATURE_EMB_PSRAM)  features.add("Embedded-PSRAM");
  if (chip_info.features & CHIP_FEATURE_IEEE802154) features.add("IEEE-802.15.4");
  mcu["idf_version"] = esp_get_idf_version();

  char netMac[18] = {0};
  JsonArray network = payloadDoc["network"].to<JsonArray>();
  JsonObject netWifi = network.add<JsonObject>();
  getMacAddress(ESP_MAC_WIFI_STA, netMac);
  netWifi["interface"] = "wifi";
  netWifi["mac_address"] = netMac;

  JsonObject netAP = network.add<JsonObject>();
  getMacAddress(ESP_MAC_WIFI_SOFTAP, netMac);
  netAP["interface"] = "wifi_ap";
  netAP["mac_address"] = netMac;

  JsonObject netBT = network.add<JsonObject>();
  getMacAddress(ESP_MAC_BT, netMac);
  netBT["interface"] = "bluetooth";
  netBT["mac_address"] = netMac;

  JsonObject netEth = network.add<JsonObject>();
  getMacAddress(ESP_MAC_ETH, netMac);
  netEth["interface"] = "ethernet";
  netEth["mac_address"] = netMac;

  JsonArray partitions = payloadDoc["partitions"].to<JsonArray>();
  esp_partition_iterator_t partIter = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
  if (partIter != NULL) {
    do {
      const esp_partition_t* p = esp_partition_get(partIter);
      JsonObject jp = partitions.add<JsonObject>();
      jp["type"]    = p->type;
      jp["subtype"] = p->subtype;
      jp["address"] = p->address;
      jp["size"]    = p->size;
      jp["label"]   = p->label;
    } while ((partIter = esp_partition_next(partIter)));
    esp_partition_iterator_release(partIter);
  }

  String payload;
  serializeJson(payloadDoc, payload);

  /* POST to FireFly-Cloud */
  String postUrl = cloudUrl + "/devices/register";

  esp_http_client_config_t cfg = {};
  cfg.url               = postUrl.c_str();
  cfg.method            = HTTP_METHOD_POST;
  cfg.crt_bundle_attach = esp_crt_bundle_attach;
  cfg.timeout_ms        = 10000;

  esp_http_client_handle_t client = esp_http_client_init(&cfg);
  esp_http_client_set_header(client, "Content-Type",       "application/json");
  esp_http_client_set_header(client, "X-Registration-Key", regKey.c_str());
  esp_http_client_set_post_field(client, payload.c_str(), payload.length());

  esp_err_t err    = esp_http_client_perform(client);
  int       status = esp_http_client_get_status_code(client);
  esp_http_client_cleanup(client);

  if (err == ESP_OK && status == 204) {
    _registrationState.registered = true;
    _registrationState.checkedAt  = timeClient.getEpochTime();
    eventLog.createEvent("Cloud registered", EventLog::LOG_LEVEL_INFO);
    request->send(204);
  } else if (err == ESP_OK && (status == 401 || status == 403)) {
    eventLog.createEvent("Cloud reg fail", EventLog::LOG_LEVEL_NOTIFICATION);
    http_forbidden(request, "Invalid or expired registration key");
  } else {
    eventLog.createEvent("Cloud reg fail", EventLog::LOG_LEVEL_NOTIFICATION);
    AsyncResponseStream *resp = request->beginResponseStream("application/json");
    JsonDocument errDoc;
    errDoc["message"] = "Cloud registration failed";
    serializeJson(errDoc, *resp);
    resp->setCode(502);
    request->send(resp);
  }
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

  request->onDisconnect([]() {
      ESP.restart();
  });

  request->send(204);

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

    default:
      http_methodNotAllowed(request);
      break;
  }
}


/**
 * Handles GET requests for device identity
*/
void http_handleIdentity_GET(AsyncWebServerRequest *request){

  if(!deviceIdentity.enabled){
    http_notFound(request);
    return;
  }

  AsyncResponseStream *response = request->beginResponseStream("application/json");
  JsonDocument doc;
  doc["uuid"] = deviceIdentity.data.uuid;
  doc["product_id"] = deviceIdentity.data.product_id;
  char product_hex_str[11] = {0};
  sprintf(product_hex_str, "0x%08lX", deviceIdentity.data.product_hex);
  doc["product_hex"] = product_hex_str;

  serializeJson(doc, *response);
  request->send(response);
}


/**
 * Handles POST requests for device identity, which burns the identity data to eFuse.
 * The response is synchronous to the operation completing.
 * Returns 409 if identity is already provisioned (eFuse is irreversible).
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

  if(deviceIdentity.enabled){
    http_conflict(request, "Device already provisioned; eFuse identity cannot be overwritten");
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

  if(doc["product_hex"].isNull()){
    http_badRequest(request, "Field product_hex is required");
    return;
  }

  if(strlen(doc["product_hex"]) != 10){
    http_badRequest(request, "Field product_hex must be exactly 10 characters");
    return;
  }

  char product_hex_buf[11];
  strlcpy(product_hex_buf, doc["product_hex"], sizeof(product_hex_buf));
  ms.Target(product_hex_buf);

  if(ms.MatchCount("^0x[0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f]$") != 1){
    http_badRequest(request, "Invalid product_hex, see docs");
    return;
  }

  postedData.product_hex = (uint32_t)strtoul(product_hex_buf + 2, NULL, 16);

  deviceIdentity.data = postedData;

  // Generate master key on-device; key never leaves the chip
  esp_fill_random(deviceIdentity.data.key, sizeof(deviceIdentity.data.key));

  if(deviceIdentity.write() == false){
    http_error(request, "Error during eFuse write");
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


