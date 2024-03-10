#ifndef authorizationToken_h
    #define authorizationToken_h

    #include <LinkedList.h>

    #ifndef VISUAL_AUTH_TOKEN_TTL
        #define VISUAL_AUTH_TOKEN_TTL 60000 //Number of milliseconds the visual token should live, 60 seconds
    #endif

    #ifndef AUTH_TOKEN_TTL
        #define AUTH_TOKEN_TTL 3600000 //Number of milliseconds the token should live, 60 minutes
    #endif

    class authorizationToken{

        public:
            struct token{
                char code[9] = "";
                uint64_t issuedAt = 0; //Time the token was issued
                uint64_t expiresAt = 0; //Time the token will expire

                /**
                 * Percent of time remaining, expressed as a float <= 1.00
                */
                float percentRemaining(){
                    #ifdef ESP32
                        return (1-((float)(esp_timer_get_time()-issuedAt)/(float)(expiresAt-issuedAt)));
                    #else
                        return (1-((float)(millis()-issuedAt)/(float)(expiresAt-issuedAt)));
                    #endif
                };
            };


        private:
            const char* letters = "AaBbCcDdEeFfGgHhiJjKkLMmnPpQqRrSsTtUuVvWwXxYyZz123456789!@#$%&+=*";
            LinkedList<token> _authorizations;
            token _visualToken;
            void (*_ptrVisualTokenChanged)(); //Function to call when the visual token changes
            bool _initialized = false; //Set when the device has been initialized

            void _newVisualToken(){

                for(uint8_t i=0; i < sizeof(this->_visualToken.code) - 1; i++){
                    this->_visualToken.code[i] = letters[random(0, strlen(letters)-1)];
                }

                #ifdef ESP32
                    this->_visualToken.issuedAt = esp_timer_get_time();
                    this->_visualToken.expiresAt = esp_timer_get_time() + (VISUAL_AUTH_TOKEN_TTL * 1000);
                #else
                    this->_visualToken.issuedAt = millis();
                    this->_visualToken.expiresAt = millis() + VISUAL_AUTH_TOKEN_TTL;
                #endif

                if(this->_ptrVisualTokenChanged){
                    this->_ptrVisualTokenChanged();
                }
            }


            /**
             * Adds the current visual token to the list of authorized tokens
             * 
            */
            void _createAuthorization(){

            }


        public:

            token getVisualToken(){
                return _visualToken;
            }

            /**
             * Validates if the given code is authorized
             * 
             * If the given code has already been authorized and is not expired, TRUE will be returned.
             * If the given code is the current visual code, the authorization will be added to the list and TRUE will be returned.
            */
            boolean authorize(char code){
                return false;
            }

            /**
             * Registers a callback function that will be called when the visual token changes
            */
            void setCallback_visualTokenChanged(void (*userDefinedCallback)()) {
                this->_ptrVisualTokenChanged = userDefinedCallback; }


            void begin(){

                _newVisualToken();

                this->_initialized = true;

            }

            void loop(){
                if(this->_initialized == false){
                    return;
                }

                for(uint8_t i=0; i < this->_authorizations.size(); i++){

                    #ifdef ESP32
                        if(_authorizations.get(i).expiresAt < esp_timer_get_time()){
                            _authorizations.remove(i);
                        }
                    #else
                        if(_authorizations.get(i).expiresAt < millis()){
                            _authorizations.remove(i);
                        }
                    #endif
                };

                #ifdef ESP32
                    if(this->_visualToken.expiresAt < esp_timer_get_time()){
                        _newVisualToken();
                    }
                #else
                    if(this->_visualToken.expiresAt < millis()){
                        _newVisualToken();
                    }
                #endif
            }
    };


#endif