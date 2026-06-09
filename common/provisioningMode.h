#ifndef provisioningMode_h
    #define provisioningMode_h

    #include <LinkedList.h>

    #ifndef PROVISIONING_MODE_TTL
        #if CORE_DEBUG_LEVEL >= 4
            #define PROVISIONING_MODE_TTL 60000*1000*30 //Number of microseconds the provisioning mode will be active after starting, 30 minutes for debug builds
        #else
            #define PROVISIONING_MODE_TTL 60000*1000*5 //Number of microseconds the provisioning mode will be active after starting, 5 minutes for production builds
        #endif
    #endif


    #if WIFI_MODEL == ENUM_WIFI_MODEL_ESP32
        #include <WiFi.h>
        #include <esp_mac.h>
        #include <esp_wifi.h>
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

            struct ProvisioningTokenEntry {
                uint32_t value;
                char mac[18];
                int64_t expires_at;
            };
            LinkedList<ProvisioningTokenEntry*> _tokens;

            char _softAPPassword[13] = {0};

            static void computeSoftAPPassword(uint8_t bssid[6], char out[13]) {
                const char hex[] = "0123456789ABCDEF";
                for (int i = 0; i < 6; i++) {
                    out[i * 2]     = hex[bssid[i] >> 4];
                    out[i * 2 + 1] = hex[bssid[5 - i] & 0xF];
                }
                out[12] = '\0';
            }

            /** Reference to the callback function that will be called when provisioning is changed */
            void (*ptrActiveCallback)();
            void (*ptrInactiveCallback)();

            /** Reference to the callback function that will be called when a rogue client is detected */
            void (*ptrRogueClientCallback)(const char*);

            void isMacAllowed(std::string macToCheck){

                #if CORE_DEBUG_LEVEL >= 4
                    #warning isMacAllowed will be bypassed because CORE_DEBUG_LEVEL >= 4
                    log_i("isMacAllowed will be bypassed because CORE_DEBUG_LEVEL >= 4");
                    return;
                #endif

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

            void _clearTokens(){
                for(int i = 0; i < _tokens.size(); i++){
                    delete _tokens.get(i);
                }
                _tokens.clear();
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
                    uint8_t softapMac[6];
                    esp_read_mac(softapMac, ESP_MAC_WIFI_SOFTAP);
                    computeSoftAPPassword(softapMac, this->_softAPPassword);

                    this->_wifi->softAP("FireFly-Provisioning", this->_softAPPassword, 1, false, 1);
                    esp_wifi_set_max_tx_power(8);  // ~2 dBm; enforces 3-5 ft physical proximity

                    this->_wifi->onEvent(
                        [this](WiFiEvent_t event, WiFiEventInfo_t info ) {
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
                _clearTokens();
                memset(_softAPPassword, 0, sizeof(_softAPPassword));

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


            /// @brief Call from the main loop() to automatically expire the mode and purge expired tokens
            void loop(){

                #ifdef ESP32
                    if(this->_isActive){
                        if(esp_timer_get_time() > this->_disableAtTime){
                            setInactive();
                            return;
                        }

                        int64_t now = esp_timer_get_time();
                        for(int i = _tokens.size() - 1; i >= 0; i--){
                            if(_tokens.get(i)->expires_at < now){
                                delete _tokens.get(i);
                                _tokens.remove(i);
                            }
                        }
                    }
                #endif
            }


            /// @brief Issues a provisioning token bound to the given MAC address, replacing any existing token for that MAC
            /// @param mac MAC address to bind the token to (xx:xx:xx:xx:xx:xx, lowercase)
            /// @return The generated token value
            uint32_t issueToken(const char* mac){
                int64_t now = esp_timer_get_time();

                for(int i = _tokens.size() - 1; i >= 0; i--){
                    if(strcmp(_tokens.get(i)->mac, mac) == 0){
                        delete _tokens.get(i);
                        _tokens.remove(i);
                    }
                }

                ProvisioningTokenEntry* entry = new ProvisioningTokenEntry();
                entry->value = esp_random();
                strncpy(entry->mac, mac, sizeof(entry->mac) - 1);
                entry->mac[sizeof(entry->mac) - 1] = '\0';
                entry->expires_at = now + PROVISIONING_MODE_TTL;
                _tokens.add(entry);
                log_d("Provisioning token issued: %u for mac=%s", entry->value, mac);
                return entry->value;
            }


            /// @brief Validates a provisioning token and extends its expiry on success
            /// @param token The token value to validate
            /// @return True if the token exists and has not expired
            bool validateToken(uint32_t token){
                int64_t now = esp_timer_get_time();
                for(int i = 0; i < _tokens.size(); i++){
                    ProvisioningTokenEntry* entry = _tokens.get(i);
                    if(entry->value == token){
                        if(entry->expires_at < now){
                            log_d("Provisioning token expired: %u for mac=%s", token, entry->mac);
                            return false;
                        }
                        entry->expires_at = now + PROVISIONING_MODE_TTL;
                        log_d("Provisioning token validated: %u for mac=%s", token, entry->mac);
                        return true;
                    }
                }
                log_d("Provisioning token not found: %u", token);
                return false;
            }


            /// @brief Copies the current SoftAP WPA2 password into the provided buffer
            /// @param out Buffer of at least 13 bytes; will be null-terminated
            void getSoftAPPassword(char out[13]){
                memcpy(out, this->_softAPPassword, 13);
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
