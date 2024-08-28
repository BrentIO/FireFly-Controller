#ifndef otaConfig_h
    #define otaConfig_h


    #include <LittleFS.h>
    #include <ArduinoJson.h>
    #include <esp32FOTA.hpp>

    #define OTA_FILE_NAME F("/ota.json")
    #ifndef FIRMWARE_CHECK_SECONDS
        #define FIRMWARE_CHECK_SECONDS 86400 /* Number of seconds between OTA firmware checks */
    #endif
    #ifndef
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


    class otaConfig{
        private:
            fs::LittleFSFS &_filesystem;

        public:

            /// @brief URL of the firmware manifest
            String url;

            /// @brief Path to certificate stored in the file system (required for https)
            String certificate;

            /**
             * @param filesystem Reference to the file system where the file is stored
            */
            otaConfig(fs::LittleFSFS &filesystem): _filesystem(filesystem){}

            enum operationStatus{
                /// @brief Operation completed without a reported error
                SUCCESS_NO_ERROR = 0,
                /// @brief The OTA configuration file exists
                FILE_EXISTS = 1,
                /// @brief The OTA configuration file does not exist
                FILE_NOT_EXISTS = 2,
                /// @brief An IO error occurred on the file system, likely because the file system is not mounted
                IO_ERROR = 3
            };


            /**
             * Checks if an OTA configuration exists in the file system
             * @returns Status of the request
            */
            operationStatus exists(){
                if(_filesystem.exists(OTA_FILE_NAME)){
                    return FILE_EXISTS;
                }

                return FILE_NOT_EXISTS;
            }


            /**
             * Creates a new OTA configuration in the file system
            *  @returns Status of the request
            */
            operationStatus create(){

                if(this->exists() == FILE_EXISTS){
                    return FILE_EXISTS;
                }

                File file = _filesystem.open(OTA_FILE_NAME, "w");

                if(!file){
                    return IO_ERROR;
                }

                StaticJsonDocument<192> doc;
                doc["url"] = this->url;

                if(!this->certificate.isEmpty()){
                    doc["certificate"] = this->certificate;
                }

                if (serializeJson(doc, file) == 0){
                    file.close();
                    return IO_ERROR;
                }

                file.close();

                return SUCCESS_NO_ERROR;
            }


            /**
             * Gets the current OTA configuration from the file system
             * @returns Status of the request
            */
            operationStatus get(){

                if(this->exists() == FILE_NOT_EXISTS){
                    return FILE_NOT_EXISTS;
                }

                File file = _filesystem.open(OTA_FILE_NAME, "r");

                StaticJsonDocument<192> doc;
                DeserializationError error = deserializeJson(doc, file);
                if(error){
                    return IO_ERROR;
                }

                file.close();
                
                this->url = (const char*)doc["url"];
                this->certificate = (const char*)doc["certificate"];

                return SUCCESS_NO_ERROR;
            }


            /**
             * Deletes the OTA current configuration from the file system
             * @returns Status of the request
            */
            operationStatus destroy(){

                if(this->exists() == FILE_NOT_EXISTS){
                    return FILE_NOT_EXISTS;
                }

                if(_filesystem.remove(OTA_FILE_NAME)){
                    return SUCCESS_NO_ERROR;
                }
                    
                return IO_ERROR;
            }
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