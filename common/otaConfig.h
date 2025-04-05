#ifndef otaConfig_h
    #define otaConfig_h

    #include <LittleFS.h>
    #include <ArduinoJson.h>
    #include <esp32FOTA.hpp>

    #ifndef FIRMWARE_CHECK_SECONDS
        #define FIRMWARE_CHECK_SECONDS 86400 /* Number of seconds between OTA firmware checks */
    #endif
    #ifndef JSON_FW_BUFF_SIZE
        #define JSON_FW_BUFF_SIZE 3072 //Supports 5 versions with maximum recommended string lengths
    #endif


    enum otaUpdateType{
        OTA_UPDATE_APP = 2,
        OTA_UPDATE_SPIFFS = 1
    };
    
    
    struct forcedOtaUpdateConfig{
        String url;
        String certificate;
        otaUpdateType type;
    };


    class exEsp32FOTA : public esp32FOTA
    {           

        public:
            using esp32FOTA::esp32FOTA; /* Inherit the base esp32FOTA */
            uint64_t lastCheckedTime = 0; /* The time (millis() or equivalent) when the firmware was last checked against the remote system */
            bool enabled = false; /* Determines if the OTA firmware automation should be run */
            LinkedList<forcedOtaUpdateConfig> pending; /* List of pending updates */
    };

#endif