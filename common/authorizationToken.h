#ifndef authorizationToken_h
    #define authorizationToken_h

    #include <LinkedList.h>

    #ifndef VISUAL_AUTH_TOKEN_TTL
        #define VISUAL_AUTH_TOKEN_TTL 60000 //Number of milliseconds the visual token should live, 60 seconds
    #endif

    #ifndef AUTH_TOKEN_TTL
        #define AUTH_TOKEN_TTL 3600000 //Number of milliseconds the token should live, 60 minutes
    #endif

    #ifndef AUTH_TOKEN_LENGTH
        #define AUTH_TOKEN_LENGTH 9 //Length of the token, including null terminator
    #endif

    class authorizationToken{

        public:
            struct token{
                char code[AUTH_TOKEN_LENGTH] = "";
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
            const char* letters = "AaBbCcDdEeFfGgHhiJjKkLMmnPpQqRrSsTtUuVvWwXxYyZz123456789!@#$%&+=*"; //List of letters that can be used when generating visual tokens
            LinkedList<token> _authorizations; //List of authorizations used for long-time use
            token _visualToken; //The current visual token
            void (*_ptrVisualTokenChanged)(); //Function to call when the visual token changes
            bool _initialized = false; //Set when the device has been initialized

            /**
             * Creates a new visual token that can be displayed on the display.  If configured, raises a callback event
            */
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
             * Adds the current visual token to the list of authorized tokens.  If the visual token already exists, it will not be re-added or updated
            */
            void _createAuthorization(){

                for(uint8_t i=0; i < this->_authorizations.size(); i++){
                    if(strncmp(this->_visualToken.code, this->_authorizations.get(i).code, strlen(this->_visualToken.code)) == 0){
                        return;
                    }
                }

                token newAuthorization;

                strcpy(newAuthorization.code, _visualToken.code);
                newAuthorization.issuedAt = _visualToken.issuedAt;

                #ifdef ESP32
                    newAuthorization.expiresAt = esp_timer_get_time() + (AUTH_TOKEN_TTL * 1000);
                #else
                    newAuthorization.expiresAt = millis() + AUTH_TOKEN_TTL;
                #endif

                this->_authorizations.add(newAuthorization);
            }


        public:

            /**
             * Retrives the current visual token that is displayed
            */
            token getVisualToken(){
                return _visualToken;
            }


            /**
             * Validates if the given code is authenticated
             * 
             * Returns TRUE if:
             *  - The code is the current visual code
             *  - The code has already been authenticated for long-term use and is not expired
             * 
             * @param code The code being attempted for authentication
             * @param retain Optional, default false, if the authentication should be retained for long-term use
            */
            boolean authenticate(const char* code, boolean retain = false){

                if(strncmp(this->_visualToken.code, code, strlen(this->_visualToken.code)) == 0){
                    if(retain){
                        _createAuthorization();
                    }
                    return true;
                }

                for(uint8_t i=0; i < this->_authorizations.size(); i++){
                    if(strncmp(code, this->_authorizations.get(i).code, strlen(this->_visualToken.code)) == 0){
                        return true;
                    }
                }
                
                return false;
            }


            /**
             * Registers a callback function that will be called when the visual token changes
            */
            void setCallback_visualTokenChanged(void (*userDefinedCallback)()) {
                this->_ptrVisualTokenChanged = userDefinedCallback; }


            /** 
             * Starts the authentication token service and creates a new visual token and should be placed in setup()
            */
            void begin(){

                _newVisualToken();
                this->_initialized = true;

            }


            /**
             * Manages the authentication token service and should be placed in loop()
            */
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