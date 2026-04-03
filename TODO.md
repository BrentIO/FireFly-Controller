# FireFly-Controller To-Do

## Docs Inconsistencies to Fix (FireFly-Docs repo)
- [x] `auxillary_data.json` referenced in docs does not exist — renamed to `devices.json`

## Backlog

### Do First (before any PRs)
- [x] **Local act build setup** — complete

- [x] **PR-10** — OIDC for S3 upload — complete

### Critical / Do Next
- [x] **PR-12A** — Defect fixes: memory leaks + Node.js 24 opt-in — complete

- [x] **PR-1** — Fix OTA `current_version` mismatch — complete

- [x] **PR-9a** — Update libraries (non-core) — complete

- [x] **PR-19** — Fix build matrix (flat include-only list for all product × application combinations) — complete

- [ ] **Defect (FireFly-Cloud)** — `config.bin` is flashed during OTA even when "Factory Reset" was not checked in the UI; fix should be in FireFly-Cloud

- [ ] **PR-12B** — Defect fixes: strcpy → strlcpy hardening
  - [ ] `extendedPubSubClient.h` setters (lines 266, 270, 274, 302, 307)
  - [ ] `Hardware-Registration-and-Configuration.ino` EEPROM POST handler (lines 1129, 1148, 1160)
  - [ ] `outputs.h:524` `getPortId()` — add `maxLen` parameter

### Medium Priority
- [ ] **PR-11** — devices.yaml as single source of truth (convert `devices.json` → `devices.yaml`)
  - [ ] Convert `devices.json` to `devices.yaml`
  - [ ] Update `generate-matrix.yaml` to parse YAML instead of JSON (replace `jq` with `yq` or convert to JSON inline)
  - [ ] Add `inputs_count` / `outputs_count` to each device in `devices.yaml`
  - [ ] Generate `database.js` `controller_products` array from `devices.yaml` in CI (pre-build step)
  - [ ] Publish `devices.yaml` as a CI artifact for Cloud/Docs repos
  - [ ] Update FireFly-Docs `controller/development_environment/index.md`: rename `devices.json` → `devices.yaml` in repo tree (line 225) and in "Adding a new hardware version" section (line 283)

- [ ] **PR-5** — Update ArduinoJSON to v7 (42 usages in Controller.ino, 13 in Hardware-Reg.ino)
  - [ ] Replace all `DynamicJsonDocument(N)` / `StaticJsonDocument<N>` → `JsonDocument`
  - [ ] Replace `createNestedObject` / `createNestedArray` → `.to<JsonObject>()` / `.to<JsonArray>()`
  - [ ] Replace `containsKey("k")` → `!doc["k"].isNull()` (~20 sites)
  - [ ] Update library pin in `build-firmware.yaml`

- [ ] **PR-6** — Use PSRAM for large allocations (depends on PR-5)
  - [ ] Create `common/psramAllocator.h` with `SpiRamAllocator` and `SpiRamJsonDocument`
  - [ ] Apply to 3 large JsonDocuments (49152, 32768, 12288 bytes) in `Controller.ino`
  - [ ] Add PSRAM availability check in `setup()`

- [ ] **PR-2** — EEPROM→NVS hard cutover (depends on PR-12A, PR-12B; all devices re-provisioned via Hardware-Reg app)
  - [ ] Create `common/deviceIdentity.h` wrapping `<Preferences.h>` — same `.data.uuid/.product_id/.key` struct interface
  - [ ] Update both `.ino` files to use new header
  - [ ] Update Hardware-Reg POST handler to write to NVS; update destroy handler to call `preferences.clear()`
  - [ ] Audit all `if(externalEEPROM.enabled)` guards (~10 sites) — `enabled` now means uuid is non-empty
  - [ ] Remove `I2C_EEPROM` library from build
  - [ ] Remove write-protect GPIO logic from `hardware.h`

- [ ] **PR-3** — Encrypt MQTT secret (depends on PR-2b)
  - [ ] Create `common/secretEncryption.h` using `mbedtls/gcm.h` (AES-256-GCM)
  - [ ] Store encrypted password as `"enc:v1:<base64(nonce||tag||ciphertext)>"` in LittleFS config
  - [ ] Update `Controller.ino` `setupMQTT()` to decrypt on read; encrypt on save
  - [ ] Backward-compatible: plaintext password still works (triggers re-encrypt on next save)

- [ ] **PR-4** — Bundle root CA for fireflylx.com in firmware (depends on PR-2b loosely)
  - [ ] Create `common/rootCA.h` with ISRG Root X1 PEM (expires 2035-06-04)
  - [ ] Update `setupOTA()` in `Controller.ino`: use `CryptoMemAsset(FIREFLYLX_ROOT_CA)` as fallback when no cert configured; custom LittleFS certs take priority

- [ ] **PR-7** — Improve WiFi bootstrapping (depends on PR-2b)
  - [ ] Add WPA2 password to soft-AP in both apps (derived from device MAC)
  - [ ] Add `POST /settings/wifi` endpoint in `Controller.ino`, save to LittleFS
  - [ ] Update `wifi.html` to POST to controller endpoint
  - [ ] Add mDNS (`firefly-XXXXXX.local`) via `ESPmDNS`

### Research
- [ ] **Temperature sensor graph smoothing** — research options before implementing
  - Missing `state_class: measurement` in auto-discovery (`Controller.ino` ~line 3069) — likely worth adding regardless
  - Options: firmware-side EMA in `common/temperature.h`, HA `filter` integration, or tuning `TEMPERATURE_SENSOR_DEGREES_VARIATION_ALLOWED` (currently 0.125°C in `common/hardware.h`)

### Large / Later
- [ ] **PR-9b** — Update ESP32 core 3.x + async stack libraries (depends on PR-5, PR-6)
  - [x] Fix `BrentIO/esp32FOTA`: rebased onto upstream v0.3.0, tagged `2026.4.1`. Includes mbedtls fix for IDF 5.x.
  - [ ] Fix `BrentIO/AsyncWebServer_ESP32_W5500`: uses `IPv6Address` which was removed in ESP32 core 3.x. Prerequisite for 3.x upgrade.
  - [ ] Update AsyncTCP, ESPAsyncWebServer, AsyncWebServer_ESP32_W5500, esp32FOTA, pubsubclient
  - [ ] Test W5500 async compatibility with core 3.x
  - [ ] Adjust `mklittlefs` glob in CI if toolchain path changes

- [ ] **PR-8** — Vue UI rewrite (depends on PR-11, PR-7)
  - [ ] Initialize Vue 3 + Vite + Pinia + Vue Router project in `Controller/www-vue/`
  - [ ] Implement dual mode: connected (ESP32 API) and standalone (File System Access API for local config files, Dexie as working cache)
  - [ ] Move auth tokens to `sessionStorage` (fix plaintext localStorage exposure)
  - [ ] Add Vite build step to `build-firmware.yaml`
  - [ ] Migrate all 9 pages
