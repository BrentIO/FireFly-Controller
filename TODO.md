# FireFly-Controller To-Do

## Backlog

### Medium Priority

- [ ] **Cloud backup encryption** — Encrypt ConfigFS backup before uploading to FireFly-Cloud *(depends on device registration)*
  - [ ] Derive `key_cloud = HKDF-SHA256(master, "firefly-cloud-v1")` from eFuse master key
  - [ ] Encrypt backup JSON with AES-256-GCM before upload; only the device can decrypt it
  - [ ] New FireFly-Cloud endpoints: `POST /backups/{uuid}`, `GET /backups/{uuid}`, `DELETE /backups/{uuid}`
  - [ ] If device is destroyed, encrypted backup is permanently unrecoverable (key exists only in eFuse)

- [ ] **Future: Controller-to-controller provisioning** — One controller provisions another via the same SoftAP flow

- [ ] **Future: FireFly-Cloud MAC→UUID recovery** — Endpoint for clients that factory-reset and lose their stored UUID *(depends on device registration)*

- [ ] **Speed up Build Firmware** — FireFly-Controller CI build is slow; investigate and reduce build time.

- [x] **OTA endpoint redesign** — `GET /ota/{class}/{product_hex}` (lowercase, case-insensitive); eliminate `FIRMWARE_TYPE_MAP` GitHub variable by embedding `firmware_type` in `manifest.json` at build time; DynamoDB PK becomes `{class}#{product_hex}` (both lowercase); S3 path becomes `{class}/{product_hex}/{version}/`; wipe existing DynamoDB firmware records and S3 firmware bucket as clean-slate migration. Changes span: CI build script, upload lambda, OTA lambda, API Gateway route, Controller OTA URL template — FireFly-Controller PR #90, FireFly-Cloud PR #204, FireFly-Docs PR #134

### UI / Config

- [ ] **FireFly-Docs — swagger-ui dark mode** — As of swagger-ui 5.31.0, dark mode is natively supported. The current FireFly-Docs implementation has custom CSS overrides that look bad. Retire the custom dark-mode CSS and rely on swagger-ui's built-in support.

- [ ] **FireFly-Cloud — remove success toast on /registration-keys "Generate new key"** — The success toast shown after generating a new key is unnecessary; remove it.

- [ ] **`/clients` — limit to 4 HIDs per client, implement `extends`** — A controller input port supports a maximum of 4 HIDs. When a physical client device requires more than 4 HIDs, it spans two input ports: a primary client record (≤4 HIDs) and a secondary client record that `extends` the primary. The two records are linked and appear as a single logical client in the UI. The secondary record is also subject to the 4-HID limit, giving a maximum of 8 HIDs per physical client.

- [x] **FireFly-Docs — eliminate duplicate content in cloud/development_environment.md** — Replaced stale deploy/delete workflow tables with a link to `cloud/github_actions/index.md`; secrets and variables tables remain canonical in `development_environment.md` — PR #130

- [x] **Rename "UI" → "FMC" (FireFly Management Console) across all repos** — Renamed all workflow files, CloudFormation templates, stack names, parameters, resource IDs, env vars (`FIREFLY_UI_URL`→`FIREFLY_FMC_URL`, `FIREFLY_UI_BUCKET`→`FIREFLY_FMC_BUCKET`), GitHub secrets/variables (`S3_UI_BUCKET_NAME`→`S3_FMC_BUCKET_NAME`, `UI_DOMAIN_NAME`→`FMC_DOMAIN_NAME`), and documentation — FireFly-Cloud PR #203, FireFly-Docs PR #133; deployed to dev and prod

- [x] **FireFly-Docs — audit cloud/github_actions documentation** — Added `cloudfront-configurator`, `configurator-ui`, `s3-configurator`; updated sidebar config and shared-layer.md — PR #131

- [x] **FireFly-Docs — audit cloud/lambdas documentation** — Removed stale `func-api-appconfig-delete.md`; added `func-api-devices-get.md` and `func-api-devices-registration-get.md` — PR #130

- [x] **FireFly-Docs — add Configurator UI to cloud.md** — Added description and CloudFormation stack entries — PR #131

- [x] **FireFly-Docs — update HW-Reg index description** — Split into two bullets: registration (done) and cloud backup (TODO badge) — PR #131

- [x] **FireFly-Docs — document func-api-devices-get** — Added `cloud/github_actions/func-api-devices-get.md` (PR #129) and `cloud/lambdas/func-api-devices-get.md` (PR #130)

- [x] **FireFly-Docs — swagger CI automation** — Specs downloaded at build time into `public/openapi/`; all three `api_reference.vue` components updated to use local CloudFront paths — PR #132

- [x] **All tables — banded row backgrounds (screen + print)** — Apply alternating row backgrounds (`even:bg-gray-50 dark:even:bg-gray-800/30` on screen; `print:even:bg-gray-100` on print) consistently across all tables in all three repos (FireFly-Controller, FireFly-Cloud, FireFly-Docs). This is a cross-cutting visual change; do all tables together rather than piecemeal.

- [ ] **Input assignments — enforce same-controller constraint** — When assigning inputs, ensure the client is assigned to a port on the same controller it is physically connected to; prevent cross-controller input assignments in the UI

### Large / Later

- [ ] **FireFly-Client provisioning rewrite** — Rewrite client firmware with new provisioning flow (scan for "FireFly-Provisioning", derive WPA2 password from BSSID, call nonce + client endpoints, store UUID in EEPROM)

---

### Future Vision — Hardware Model Registry

Adding a new hardware model currently requires touching many files across multiple repos manually and in the right order:

- `devices.yaml` (FireFly-Controller) — product_hex, product_id, status, partition scheme, inputs_count, outputs_count
- `common/hardware.h` (or per-product file after PR-13) — pin mappings, I2C addresses, display config, all hardware constants
- `Controller/www/database.js` — `controller_products` array entry (pid, inputs.count, outputs.count); currently generated from devices.yaml in CI but the hardware constants are still manual
- `.act/dockerfile` — no change needed if CI generates artifacts correctly, but currently requires manual awareness
- `FireFly-Docs` — repo tree listing, "Adding a new hardware version" section, partition table docs
- `FireFly-Cloud` — any product-aware logic (OTA routing, manifest parsing, etc.)

The ideal end state is a single authoritative source — likely a structured YAML or a lightweight web UI — that owns all per-product data and drives code generation for every downstream artifact. A developer adding a new hardware variant would fill in one form (or edit one file) and get PRs opened automatically across all affected repos via a GitHub Actions workflow or CLI tool. This is a significant undertaking and touches every repo in the ecosystem, so it should only be designed once the per-product file separation (PR-13) and devices.yaml migration (PR-11) are stable.

---

### Open issues — FireFly-Cloud

- [x] Color scheme is slightly more blue than HW-Reg — verified in sync across all three UIs, no changes needed
- [x] We are missing lambdas to configure in /appconfig — no code change needed; the lambda auto-discovers all `firefly-func-*` functions via the Lambda API paginator at runtime
- [x] When deleting firmware, set the "Release Status" to "Deleting" immediately while the S3 lambda deletes it, so that it disappears from the table immediately — PR #201
- [x] Clean up test records in PROD — manual one-time purge of firefly-devices and firefly-firmware; root cause (device record leak) fixed in PR #200; firmware cleanup automatic via CLEANUP_TEST_RECORDS in PROD
- [x] Move the "Generate Registration Key" button to a new dedicated page where all registration keys are shown in a table — PR #198
- [x] Registration keys should be scoped to the generating user — each user sees only their own keys — PR #198
- [x] When registering a device, look up the registration key used, determine who generated it, and write that to the DynamoDB record; show the registering user's name in the /devices modal (click to reveal email) — PR #198
- [x] /firmware: add a History section — stamp each release status change with a UTC timestamp; show a collapsed "History ({count})" panel above "Manifest Files" in the firmware detail view — PR #199
