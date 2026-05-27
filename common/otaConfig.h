#ifndef otaConfig_h
    #define otaConfig_h

    #include <esp32OTA.h>
    #include <LinkedList.h>

    #ifndef FIRMWARE_CHECK_SECONDS
        #define FIRMWARE_CHECK_SECONDS 86400 /* Number of seconds between OTA firmware checks */
    #endif
    #ifndef CLOUD_BACKUP_INTERVAL_SECONDS
        #define CLOUD_BACKUP_INTERVAL_SECONDS 86400 /* Number of seconds between automatic cloud backup uploads */
    #endif

    enum otaUpdateType{
        OTA_UPDATE_APP = 2,
        OTA_UPDATE_UI = 1
    };

    struct forcedOtaUpdateConfig{
        String url;
        otaUpdateType type;
    };

#endif
