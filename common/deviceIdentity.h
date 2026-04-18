#pragma once
#include <Preferences.h>
#include <esp_efuse.h>
#include "esp_efuse_table.h"

#define DEVICE_IDENTITY_NVS_NAMESPACE "device"

// BLOCK1: bytes 0-15 = UUID (128 bits), bytes 16-19 = product_hex (32 bits)
// BLOCK3: bytes 0-31 = master key (256 bits)
#define EFUSE_UUID_OFFSET_BITS   0
#define EFUSE_UUID_SIZE_BITS     128
#define EFUSE_PHEX_OFFSET_BITS   128
#define EFUSE_PHEX_SIZE_BITS     32
#define EFUSE_KEY_OFFSET_BITS    0
#define EFUSE_KEY_SIZE_BITS      256

class managerDeviceIdentity {

    public:

        struct deviceType {
            char uuid[37];       /**< Hardware unique identifier */
            char product_id[33]; /**< Product ID */
            uint8_t key[32];     /**< 32-byte master secret (never transmitted) */
            uint32_t product_hex; /**< Product hex code */
        } data;

        bool enabled = false; /**< True if device has been provisioned (UUID is non-zero in eFuse). */

        static void uuidBytesToString(const uint8_t bytes[16], char out[37]) {
            snprintf(out, 37,
                "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                bytes[0],  bytes[1],  bytes[2],  bytes[3],
                bytes[4],  bytes[5],
                bytes[6],  bytes[7],
                bytes[8],  bytes[9],
                bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
        }

        static bool uuidStringToBytes(const char* str, uint8_t out[16]) {
            if (strlen(str) != 36) return false;
            const char* src = str;
            int i = 0;
            const int groupLen[] = { 8, 4, 4, 4, 12 };
            for (int g = 0; g < 5; g++) {
                for (int j = 0; j < groupLen[g]; j += 2) {
                    char hex[3] = { src[j], src[j + 1], '\0' };
                    out[i++] = (uint8_t)strtoul(hex, NULL, 16);
                }
                src += groupLen[g];
                if (g < 4) src++; // skip dash
            }
            return true;
        }

        void begin() {

            uint8_t uuidBytes[16] = {0};
            esp_efuse_read_block(EFUSE_BLK1, uuidBytes, EFUSE_UUID_OFFSET_BITS, EFUSE_UUID_SIZE_BITS);
            esp_efuse_read_block(EFUSE_BLK1, &data.product_hex, EFUSE_PHEX_OFFSET_BITS, EFUSE_PHEX_SIZE_BITS);
            esp_efuse_read_block(EFUSE_BLK3, data.key, EFUSE_KEY_OFFSET_BITS, EFUSE_KEY_SIZE_BITS);

            enabled = false;
            for (int i = 0; i < 16; i++) {
                if (uuidBytes[i] != 0) { enabled = true; break; }
            }

            if (enabled) {
                uuidBytesToString(uuidBytes, data.uuid);
            } else {
                memset(&data, 0, sizeof(data));
            }

            Preferences prefs;
            prefs.begin(DEVICE_IDENTITY_NVS_NAMESPACE, true);
            strlcpy(data.product_id, prefs.getString("pid", "").c_str(), sizeof(data.product_id));
            prefs.end();
        }

        /**
         * Burns identity data to eFuse and writes product_id to NVS.
         * Returns false if already provisioned or if any eFuse write fails.
         */
        bool write() {

            if (enabled) {
                log_e("Device identity already provisioned in eFuse; cannot overwrite");
                return false;
            }

            uint8_t uuidBytes[16] = {0};
            if (!uuidStringToBytes(data.uuid, uuidBytes)) {
                log_e("Invalid UUID format");
                return false;
            }

            if (esp_efuse_write_block(EFUSE_BLK1, uuidBytes, EFUSE_UUID_OFFSET_BITS, EFUSE_UUID_SIZE_BITS) != ESP_OK) {
                log_e("Failed to write UUID to eFuse BLOCK1");
                return false;
            }

            if (esp_efuse_write_block(EFUSE_BLK1, &data.product_hex, EFUSE_PHEX_OFFSET_BITS, EFUSE_PHEX_SIZE_BITS) != ESP_OK) {
                log_e("Failed to write product_hex to eFuse BLOCK1");
                return false;
            }

            if (esp_efuse_write_block(EFUSE_BLK3, data.key, EFUSE_KEY_OFFSET_BITS, EFUSE_KEY_SIZE_BITS) != ESP_OK) {
                log_e("Failed to write master key to eFuse BLOCK3");
                return false;
            }

            Preferences prefs;
            if (!prefs.begin(DEVICE_IDENTITY_NVS_NAMESPACE, false)) {
                log_e("Failed to open NVS namespace '%s' for writing", DEVICE_IDENTITY_NVS_NAMESPACE);
                return false;
            }
            prefs.putString("pid", data.product_id);
            prefs.end();

            enabled = true;
            return true;
        }
};
