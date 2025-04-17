#ifndef provisioningMode_h
    #define provisioningMode_h

    #include <LinkedList.h>

    #ifndef PROVISIONING_MODE_TTL
        #define PROVISIONING_MODE_TTL 60000*1000*30 //Number of microseconds the provisioning mode will be active after starting, default 30 minutes
    #endif


    #if WIFI_MODEL == ENUM_WIFI_MODEL_ESP32
        #include <WiFi.h>
    #endif


    #ifndef ESP32
        #error Unsupported Hardware
    #endif


    class managerProvisioningMode{

        private:
            #if WIFI_MODEL == ENUM_WIFI_MODEL_ESP32
                WiFiClass *_wifi;
            #endif

            bool _isActive = false;
            int64_t _disableAtTime = 0;
            LinkedList<std::string> _whitelistMACs;

            /** Reference to the callback function that will be called when provisioning is changed */
            void (*ptrActiveCallback)();
            void (*ptrInactiveCallback)();

            /** Reference to the callback function that will be called when a rogue client is detected */
            void (*ptrRogueClientCallback)(const char*);

            void isMacAllowed(std::string macToCheck){

                for(int i=0; i < this->_whitelistMACs.size(); i++){

                    if(strcmp(macToCheck.c_str(), this->_whitelistMACs.get(i).c_str()) == 0){
                        return; //MAC permitted
                    }

                }

                if(this->ptrRogueClientCallback){
                    this->ptrRogueClientCallback(macToCheck.c_str());
                }

                this->setInactive();
            }

        public:

            #if WIFI_MODEL == ENUM_WIFI_MODEL_ESP32
                /// @brief Sets the WiFi instance to be manipulated
                /// @param value 
                void setWiFI(WiFiClass *value){
                    this->_wifi = value;
                }
            #endif


            /// @brief Add a MAC address to the whitelist
            /// @param mac MAC address (in xx:xx:xx:xx:xx:xx format)
            void addAllowedMac(std::string mac){
                this->_whitelistMACs.add(mac);
            }


            /// @brief Sets provisioning mode to an active state
            void setActive(){
                #if WIFI_MODEL == ENUM_WIFI_MODEL_ESP32
                    this->_wifi->softAP(F("FireFly-Provisioning"));

                    this->_wifi->onEvent(
                        [&](WiFiEvent_t event, WiFiEventInfo_t info ) {
                          char macAddress[18] = {0};
                          sprintf(macAddress, "%02x:%02x:%02x:%02x:%02x:%02x", info.wifi_ap_staconnected.mac[0],info.wifi_ap_staconnected.mac[1],info.wifi_ap_staconnected.mac[2],info.wifi_ap_staconnected.mac[3], info.wifi_ap_staconnected.mac[4], info.wifi_ap_staconnected.mac[5]);
                          this->isMacAllowed(macAddress);
                        },
                        WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STACONNECTED
                      );
                #endif

                #ifdef ESP32
                    this->_disableAtTime = esp_timer_get_time() + PROVISIONING_MODE_TTL;
                #endif

                this->_isActive = true;

                if(this->ptrActiveCallback){
                    this->ptrActiveCallback();
                }
            }


            /// @brief Sets provisioning mode to an inactive state
            void setInactive(){

                _whitelistMACs.clear();

                #if WIFI_MODEL == ENUM_WIFI_MODEL_ESP32
                    this->_wifi->softAPdisconnect(true);
                #endif

                this->_disableAtTime = 0;
                this->_isActive = false;

                if(this->ptrInactiveCallback){
                    this->ptrInactiveCallback();
                }
            }


            /// @brief Get the current state of provisioning mode
            /// @return True if Active, False if Inactive
            bool getStatus(){
                return this->_isActive;
            }


            /// @brief Call from the main loop() to automatically expire the mode
            void loop(){

                #ifdef ESP32
                    if(this->_isActive){
                        if(esp_timer_get_time() > this->_disableAtTime){
                            setInactive();
                        }
                    }
                #endif
            }


        /** Callback function that is called when a provisioning mode is activated */
        void setCallback_active(void (*userDefinedCallback)()) {
            ptrActiveCallback = userDefinedCallback; }

        
        /** Callback function that is called when a provisioning mode is deactivated */
        void setCallback_inactive(void (*userDefinedCallback)()) {
            ptrInactiveCallback = userDefinedCallback; }


        /** Callback function that is called when a rogue client connects */
        void setCallback_rogueClient(void (*userDefinedCallback)(const char*)) {
            ptrRogueClientCallback = userDefinedCallback; }
    };

#endif