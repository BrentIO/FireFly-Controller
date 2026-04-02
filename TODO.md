# FireFly-Controller To-Do

## Docs Inconsistencies to Fix (FireFly-Docs repo)
- [ ] `auxillary_data.json` referenced in docs does not exist — rename to `devices.json`
- [ ] Visual token validity: docs say "~1 hour", code (`common.js`) is 30 minutes

## Backlog

### Do First (before any PRs)
- [x] **Local act build setup** — complete

- [ ] **PR-10 (first PR)** — OIDC for S3 upload (FireFly-Cloud already operational; enables automatic Cloud delivery from every build)
  - [x] Add `permissions: id-token: write` + `contents: read` to `ConfigureAndBuild` job
  - [x] Replace `NotCoffee418/s3-zip-upload` step with `aws-actions/configure-aws-credentials@v4` (OIDC) + `aws s3 cp`
  - [ ] Create AWS IAM OIDC identity provider + role with `s3:PutObject`, trust restricted to this repo — add `AWS_ROLE_ARN` secret to repo
  - [ ] Delete `AWS_ACCESS_KEY_ID` / `AWS_SECRET_ACCESS_KEY` secrets from repo settings (after role is working)

### Critical / Do Next
- [ ] **PR-12A** — Defect fixes: memory leaks
  - [ ] Replace `new char[]` with stack allocation in 5 failure handler callbacks (`Hardware-Registration-and-Configuration.ino` lines 1327, 1345, 1364, 1382, 1400)
  - [ ] Fix `strdup` leak in `extendedPubSubClient.h:283` `addSubscription()` — change `LinkedList<const char*>` to `LinkedList<String>`
  - [ ] Fix lambda `[&]` → `[this]` in `provisioningMode.h:85`; deregister event handler in `setInactive()`

- [ ] **PR-1** — Fix OTA `current_version` mismatch
  - [ ] Append `?current_version=VERSION` to manifest URL in `Controller.ino` `setupOTA()` (~line 2304) so Cloud lambda doesn't return 400

- [ ] **PR-10** — OIDC for S3 upload
  - [ ] Replace long-lived AWS keys with OIDC role assumption in `build-firmware.yaml`
  - [ ] Create IAM OIDC provider + role in AWS
  - [ ] Delete `AWS_ACCESS_KEY_ID` / `AWS_SECRET_ACCESS_KEY` secrets from repo

- [ ] **PR-9a** — Update libraries (non-core)
  - [ ] Check latest versions of: Adafruit_BusIO, Adafruit-GFX, Adafruit_SSD1306, ArduinoStreamUtils, PCA95x5, PCT2075, Ethernet, LinkedList, NTPClient, PCA9685_RT, Regexp
  - [ ] Update Arduino CLI from 0.35.3 to 1.x stable

- [ ] **PR-12B** — Defect fixes: strcpy → strlcpy hardening
  - [ ] `extendedPubSubClient.h` setters (lines 266, 270, 274, 302, 307)
  - [ ] `Hardware-Registration-and-Configuration.ino` EEPROM POST handler (lines 1129, 1148, 1160)
  - [ ] `outputs.h:524` `getPortId()` — add `maxLen` parameter

### Medium Priority
- [ ] **PR-11** — devices.json as single source of truth
  - [ ] Add `inputs_count` / `outputs_count` to each device in `devices.json`
  - [ ] Generate `database.js` `controller_products` array from `devices.json` in CI (pre-build `jq` step)
  - [ ] Publish `devices.json` as a CI artifact for Cloud/Docs repos

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

### Large / Later
- [ ] **PR-9b** — Update ESP32 core 3.x + async stack libraries (depends on PR-5, PR-6)
  - [ ] Update AsyncTCP, ESPAsyncWebServer, AsyncWebServer_ESP32_W5500, esp32FOTA, pubsubclient
  - [ ] Test W5500 async compatibility with core 3.x
  - [ ] Adjust `mklittlefs` glob in CI if toolchain path changes

- [ ] **PR-8** — Vue UI rewrite (depends on PR-11, PR-7)
  - [ ] Initialize Vue 3 + Vite + Pinia + Vue Router project in `Controller/www-vue/`
  - [ ] Implement dual mode: connected (ESP32 API) and standalone (File System Access API for local config files, Dexie as working cache)
  - [ ] Move auth tokens to `sessionStorage` (fix plaintext localStorage exposure)
  - [ ] Add Vite build step to `build-firmware.yaml`
  - [ ] Migrate all 9 pages
