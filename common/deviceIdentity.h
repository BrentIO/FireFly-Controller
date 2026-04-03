#pragma once
#include <Preferences.h>

#define DEVICE_IDENTITY_NVS_NAMESPACE "device"

class managerDeviceIdentity {

    public:

        struct deviceType {
            char uuid[37];       /**< Hardware unique identifier */
            char product_id[33]; /**< Product ID */
            char key[65];        /**< Security key */
            uint32_t product_hex; /**< Product hex code, set at provisioning time */
        } data;

        bool enabled = false; /**< True if the device has been provisioned (uuid is non-empty). */

        void begin() {

            Preferences prefs;
            prefs.begin(DEVICE_IDENTITY_NVS_NAMESPACE, true);
            strlcpy(data.uuid,       prefs.getString("uuid", "").c_str(), sizeof(data.uuid));
            strlcpy(data.product_id, prefs.getString("pid",  "").c_str(), sizeof(data.product_id));
            strlcpy(data.key,        prefs.getString("key",  "").c_str(), sizeof(data.key));
            data.product_hex = prefs.getUInt("phex", 0);
            prefs.end();

            enabled = (strlen(data.uuid) > 0);
        }

        /**
         * Writes the current data struct to NVS.
         * @returns true on success, false on failure.
         */
        bool write() {

            Preferences prefs;

            if (!prefs.begin(DEVICE_IDENTITY_NVS_NAMESPACE, false)) {
                log_e("Failed to open NVS namespace '%s' for writing", DEVICE_IDENTITY_NVS_NAMESPACE);
                return false;
            }

            prefs.putString("uuid", data.uuid);
            prefs.putString("pid",  data.product_id);
            prefs.putString("key",  data.key);
            prefs.putUInt("phex",   data.product_hex);
            prefs.end();

            enabled = (strlen(data.uuid) > 0);
            return true;
        }

        /**
         * Clears all device identity data from NVS.
         * @returns true on success, false on failure.
         */
        bool destroy() {

            Preferences prefs;

            if (!prefs.begin(DEVICE_IDENTITY_NVS_NAMESPACE, false)) {
                log_e("Failed to open NVS namespace '%s' for clearing", DEVICE_IDENTITY_NVS_NAMESPACE);
                return false;
            }

            prefs.clear();
            prefs.end();

            memset(&data, 0, sizeof(data));
            enabled = false;
            return true;
        }
};
