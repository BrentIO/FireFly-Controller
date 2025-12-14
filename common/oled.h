#include "hardware.h"
#include "Prototype9pt7b.h"
#include <NTPClient.h>
#include "eventLog.h"
#include "authorizationToken.h"

#if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_W5500
    #include <AsyncWebServer_ESP32_W5500.h>
#endif

#if WIFI_MODEL == ENUM_WIFI_MODEL_ESP32
    #include <WiFi.h>
#endif

#ifndef oled_h
    #define oled_h

    #define COUNT_PAGES 6 //The total number of pages without an error and without an auth token
    #define INTRO_DWELL_MS 750 //Number of milliseconds that the intro page should be shown when switching screens

    class managerOled{

        public:

            enum failureReason{
                ADDRESS_OFFLINE = 2, /* Indicates the device was not found on the bus */
                UNABLE_TO_START = 12 /* Indicates the underlying hardware library returned a fault when attempting to begin communications */
            };

            enum pages{
                PAGE_EVENT_LOG = 1,
                PAGE_EVENT_LOG_INTRO = 10,
                PAGE_WIFI = 2,
                PAGE_ETHERNET = 3,
                PAGE_NETWORK_INTRO = 20,
                PAGE_HARDWARE = 4,
                PAGE_HARDWARE_INTRO = 40,
                PAGE_SOFTWARE = 5,
                PAGE_SOFTWARE_INTRO = 50,
                PAGE_ERROR = 6,
                PAGE_ERROR_INTRO = 60,
                PAGE_AUTH_TOKEN = 7,
                PAGE_AUTH_TOKEN_INTRO = 70,
                PAGE_OTA_IN_PROGRESS = 254,
                PAGE_FACTORY_RESET = 255
            };

        private:

            void (*ptrFailureCallback)(uint8_t, failureReason); //Function to call when there is a callback

            bool _initialized = false; //Set when the device has been initialized
            uint8_t _address = OLED_ADDRESS;
            char* _productId;
            char* _uuid;
            char* _name;

            #if WIFI_MODEL == ENUM_WIFI_MODEL_ESP32
                WiFiClass *_wifiInfo;
            #endif

            #if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_W5500
                ESP32_W5500 *_ethernetInfo;
            #endif

            pages _activePage = PAGE_EVENT_LOG;
            boolean _isSleeping = false;
            boolean _isDimmed = false;
            int _factory_reset_value = 0;
            EventLog *_eventLog;
            authorizationToken *_authorizationToken;
            unsigned long _timeLastAction = 0;
            unsigned long _timeIntroShown = 0;

            #define DIM_AFTER_MS 10000
            #define SLEEP_AFTER_MS 15000

            #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32
                Adafruit_SSD1306 hardware;
            #endif


            void _clear(){

                if(this->_initialized != true){
                    return;
                }

                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32
                    this->hardware.clearDisplay();
                    this->hardware.invertDisplay(false);
                #endif
            }


            void _commit(){

                if(this->_initialized != true){
                    return;
                }

                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32
                    this->hardware.display();
                #endif

            }


            void _dim(){

                if(this->_initialized != true){
                    return;
                }

                if(this->_eventLog){
                    if(this->_eventLog->getErrors()->size() > 0){
                        _extendWake();
                        this->_showPage_Error();
                        return;
                    }
                }

                if(this->_activePage == PAGE_AUTH_TOKEN){
                    _extendWake();
                    return;
                }

                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32
                    this->hardware.dim(true);
                #endif

                this->_isDimmed = true;
                this->_isSleeping = false;
            }


            void _sleep(){

                if(this->_initialized != true){
                    return;
                }

                if(this->_eventLog){
                    if(this->_eventLog->getErrors()->size() > 0){
                        _extendWake();
                        this->_showPage_Error();
                        return;
                    }
                }

                if(this->_activePage == PAGE_AUTH_TOKEN){
                    _extendWake();
                    return;
                }

                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32
                    this->hardware.ssd1306_command(SSD1306_DISPLAYOFF);
                #endif

                this->_isSleeping = true;
                this->_isDimmed = false;
            }
            
            
            void _wake(){

                if(this->_initialized != true){
                    return;
                }

                log_v("Woke up OLED");

                if(this->_isDimmed == true){

                    #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32
                        this->hardware.dim(false);
                    #endif

                    this->_isDimmed = false;
                }

                if(this->_isSleeping == true){

                    #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32
                        this->hardware.dim(false);  //Required because without it the display only turns back on to a dimmed display
                        this->hardware.ssd1306_command(SSD1306_DISPLAYON);
                    #endif

                    this->_isSleeping = false;
                }      

                _extendWake();

            }


            void _extendWake(void){

                if(this->_initialized != true){
                    return;
                }

                log_v("Extending OLED wake");

                _timeLastAction = millis();
            }


            void _drawScrollBar(pages page){

                if(this->_initialized != true){
                    return;
                }

                uint8_t total = COUNT_PAGES;

                if(this->_eventLog){
                    if(this->_eventLog->getErrors()->size() > 0){
                        total = total +1;
                    }
                }

                if(this->_authorizationToken){
                    total = total +1;
                }

                int val = map(page, 1, total, 1, OLED_DISPLAY_HEIGHT-OLED_SCROLL_BAR_HEIGHT-1);

                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32
                    this->hardware.setTextColor(SSD1306_WHITE);
                    this->hardware.fillRect((OLED_DISPLAY_WIDTH-OLED_SCROLL_BAR_WIDTH),0,OLED_SCROLL_BAR_WIDTH, OLED_DISPLAY_HEIGHT, SSD1306_WHITE); //Scroll Bar
                    this->hardware.fillRect((OLED_DISPLAY_WIDTH-OLED_SCROLL_BAR_WIDTH),val,OLED_SCROLL_BAR_WIDTH,OLED_SCROLL_BAR_HEIGHT, SSD1306_BLACK); //Scroll position
                #endif      
            }


            void _showPage_Logo(){

                if(this->_initialized != true){
                    return;
                }

                this->_wake();
                this->_clear();

                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32
                    const uint8_t logo_width = 20;
                    const uint8_t logo_height = 10;

                    const uint8_t PROGMEM logo[] = {
                        0b00000110,0b11001001,0b10110000,
                        0b00001110,0b11011011,0b11110000,
                        0b00001101,0b11111011,0b01100000,
                        0b00011111,0b10110110,0b11100000,
                        0b00011011,0b01111110,0b11000000,
                        0b00110111,0b11101101,0b10000000,
                        0b01110110,0b11011111,0b10000000,
                        0b01101101,0b11111011,0b00000000,
                        0b11111101,0b10110111,0b00000000,
                        0b11011001,0b00110110,0b00000000,
                    };

                    this->hardware.drawBitmap(0, (OLED_DISPLAY_HEIGHT - logo_height) / 2, logo, logo_width, logo_height, SSD1306_WHITE);
                    this->hardware.setCursor(logo_width + 2, OLED_DISPLAY_HEIGHT / 2);
                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                    this->hardware.setFont(&Prototype9pt7b);
                    this->hardware.println("P5 Software");
                    this->hardware.setFont();

                #endif

                this->_commit();

            }

        
            void _showPage_Software(){

                if(this->_initialized != true){
                    return;
                }

                this->_activePage = PAGE_SOFTWARE;
                this->_clear();

                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32
                    this->hardware.setCursor(0, 0);
                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text

                    #ifdef APPLICATION_NAME
                        this->hardware.println(APPLICATION_NAME);
                    #endif

                    this->hardware.print("Ver: ");

                    #ifdef VERSION
                        this->hardware.println(VERSION);
                    #else
                        this->hardware.println("UNKNOWN");
                    #endif

                    if(this->_name){
                        this->hardware.println(this->_name);
                    }else{
                        this->hardware.println("No Device Name");
                    }

                    #ifdef COMMIT_HASH
                        this->hardware.println(COMMIT_HASH);
                    #endif

                #endif

                this->_drawScrollBar(PAGE_SOFTWARE);
                this->_commit();
            }


            void _showPage_EventLog_Intro(){

                if(this->_initialized != true){
                    return;
                }

                this->_activePage = PAGE_EVENT_LOG_INTRO;

                this->_wake();
                this->_clear();

                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32

                    const uint8_t logo_width = 20;
                    const uint8_t logo_height = 20;

                    const uint8_t PROGMEM logo[] = {
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00111111,0b11111111,0b11000000,
                        0b00111111,0b11111111,0b11000000,
                        0b00111111,0b11111111,0b11000000,
                        0b00111000,0b00000001,0b11000000,
                        0b00111111,0b11111111,0b11000000,
                        0b00111111,0b11111111,0b11000000,
                        0b00111000,0b00000001,0b11000000,
                        0b00111111,0b11111111,0b11000000,
                        0b00111000,0b00011111,0b11000000,
                        0b00111111,0b11111111,0b11000000,
                        0b00111111,0b11111111,0b11000000,
                        0b00111111,0b11111111,0b11000000,
                        0b00111111,0b11111111,0b00000000,
                        0b00110000,0b00000000,0b00000000,
                        0b00100000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                    };

                    this->hardware.drawBitmap(0, (OLED_DISPLAY_HEIGHT - logo_height) / 2, logo, logo_width, logo_height, SSD1306_WHITE);
                    this->hardware.setCursor(logo_width + 5, OLED_DISPLAY_HEIGHT / 2);
                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                    this->hardware.setFont(&Prototype9pt7b);
                    this->hardware.println("Event Log");
                    this->hardware.setFont();
                    this->_drawScrollBar(PAGE_EVENT_LOG);

                #endif

                _timeIntroShown = millis();
                this->_commit();   
            }


            void _showPage_Network_Intro(){

                if(this->_initialized != true){
                    return;
                }

                this->_activePage = PAGE_NETWORK_INTRO;
                this->_wake();
                this->_clear();

                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32

                    const uint8_t logo_width = 20;
                    const uint8_t logo_height = 20;

                    const uint8_t PROGMEM logo[] = {
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000001,0b11111000,0b00000000,
                        0b00000001,0b11111000,0b00000000,
                        0b00000001,0b11111000,0b00000000,
                        0b00000001,0b11111000,0b00000000,
                        0b00000001,0b11111000,0b00000000,
                        0b00000000,0b01100000,0b00000000,
                        0b00000000,0b01100000,0b00000000,
                        0b00000111,0b11111110,0b00000000,
                        0b00000111,0b11111110,0b00000000,
                        0b00000110,0b00000110,0b00000000,
                        0b00000110,0b00000110,0b00000000,
                        0b00011111,0b10011111,0b10000000,
                        0b00011111,0b10011111,0b10000000,
                        0b00011111,0b10011111,0b10000000,
                        0b00011111,0b10011111,0b10000000,
                        0b00011111,0b10011111,0b10000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                    };

                    this->hardware.drawBitmap(0, (OLED_DISPLAY_HEIGHT - logo_height) / 2, logo, logo_width, logo_height, SSD1306_WHITE);
                    this->hardware.setCursor(logo_width + 5, OLED_DISPLAY_HEIGHT / 2);
                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                    this->hardware.setFont(&Prototype9pt7b);
                    this->hardware.println("Network");
                    this->hardware.setFont();
                    this->_drawScrollBar(PAGE_WIFI);

                #endif

                _timeIntroShown = millis();
                this->_commit();  
            }


            void _showPage_Hardware_Intro(){

                if(this->_initialized != true){
                    return;
                }

                this->_activePage = PAGE_HARDWARE_INTRO;
                this->_wake();
                this->_clear();

                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32

                    const uint8_t logo_width = 20;
                    const uint8_t logo_height = 20;

                    const uint8_t PROGMEM logo[] = {
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00001111,0b11111111,0b00000000,
                        0b00011111,0b11111111,0b10000000,
                        0b00011100,0b11111111,0b10000000,
                        0b00011100,0b11111111,0b10000000,
                        0b00011111,0b11111111,0b10000000,
                        0b00001111,0b11111111,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00001111,0b11111111,0b00000000,
                        0b00011111,0b11111111,0b10000000,
                        0b00011100,0b11111111,0b10000000,
                        0b00011100,0b11111111,0b10000000,
                        0b00011111,0b11111111,0b10000000,
                        0b00001111,0b11111111,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                    };

                    this->hardware.drawBitmap(0, (OLED_DISPLAY_HEIGHT - logo_height) / 2, logo, logo_width, logo_height, SSD1306_WHITE);
                    this->hardware.setCursor(logo_width + 5, OLED_DISPLAY_HEIGHT / 2);
                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                    this->hardware.setFont(&Prototype9pt7b);
                    this->hardware.println("Hardware");
                    this->hardware.setFont();
                    this->_drawScrollBar(PAGE_HARDWARE);
                
                #endif

                _timeIntroShown = millis();
                this->_commit();  

            }


            void _showPage_Software_Intro(){

                if(this->_initialized != true){
                    return;
                }

                this->_activePage = PAGE_SOFTWARE_INTRO;
                this->_wake();
                this->_clear();

                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32

                    const uint8_t logo_width = 20;
                    const uint8_t logo_height = 20;

                    const uint8_t PROGMEM logo[] = {
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00001111,0b11111111,0b00000000,
                        0b00011111,0b11111111,0b10000000,
                        0b00111111,0b11111111,0b11000000,
                        0b00110000,0b00000000,0b11000000,
                        0b00110010,0b00000000,0b11000000,
                        0b00110011,0b00000000,0b11000000,
                        0b00110001,0b10000000,0b11000000,
                        0b00110011,0b00000000,0b11000000,
                        0b00110110,0b00111110,0b11000000,
                        0b00110000,0b00000000,0b11000000,
                        0b00010111,0b11111111,0b10000000,
                        0b00011111,0b11111111,0b10000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                    };

                    this->hardware.drawBitmap(0, (OLED_DISPLAY_HEIGHT - logo_height) / 2, logo, logo_width, logo_height, SSD1306_WHITE);
                    this->hardware.setCursor(logo_width + 5, OLED_DISPLAY_HEIGHT / 2);
                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                    this->hardware.setFont(&Prototype9pt7b);
                    this->hardware.println("Software");
                    this->hardware.setFont();
                    this->_drawScrollBar(PAGE_SOFTWARE);

                #endif

                _timeIntroShown = millis();
                this->_commit();  

            }


            void _showPage_Error_Intro(){

                if(this->_initialized != true){
                    return;
                }

                this->_activePage = PAGE_ERROR_INTRO;
                this->_wake();
                this->_clear();

                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32

                    const uint8_t logo_width = 20;
                    const uint8_t logo_height = 20;

                    const uint8_t PROGMEM logo[] = {
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000011,0b11111100,0b00000000,
                        0b00000111,0b11111110,0b00000000,
                        0b00001111,0b11111111,0b00000000,
                        0b00011111,0b10011111,0b10000000,
                        0b00011111,0b10011111,0b10000000,
                        0b00011111,0b10011111,0b10000000,
                        0b00011111,0b10011111,0b10000000,
                        0b00011111,0b10011111,0b10000000,
                        0b00011111,0b11111111,0b10000000,
                        0b00011111,0b10011111,0b10000000,
                        0b00011111,0b10011111,0b10000000,
                        0b00001111,0b11111111,0b00000000,
                        0b00000111,0b11111110,0b00000000,
                        0b00000011,0b11111100,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                    };

                    this->hardware.drawBitmap(0, (OLED_DISPLAY_HEIGHT - logo_height) / 2, logo, logo_width, logo_height, SSD1306_WHITE);
                    this->hardware.setCursor(logo_width + 5, OLED_DISPLAY_HEIGHT / 2);
                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                    this->hardware.setFont(&Prototype9pt7b);
                    this->hardware.println("Error");
                    this->hardware.setFont();
                    this->_drawScrollBar(PAGE_ERROR);

                #endif

                _timeIntroShown = millis();
                this->_commit();   
            }


            void _showPage_Factory_Reset(){

                if(this->_initialized != true){
                    return;
                }

                this->_activePage = PAGE_FACTORY_RESET;
                this->_wake();
                this->_clear();

                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32

                    if (this->_factory_reset_value % 2 == 0){
                        this->hardware.invertDisplay(false);
                    }else{
                        this->hardware.invertDisplay(true);
                    }

                    this->hardware.setCursor(0, 0);
                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                    this->hardware.println("     Factory Reset    ");
                    this->hardware.setFont(&Prototype9pt7b);
                    this->hardware.setCursor(OLED_DISPLAY_WIDTH / 2, 26);
                    this->hardware.println(this->_factory_reset_value);
                    this->hardware.setFont();

                #endif

                _timeIntroShown = millis();
                this->_commit();   
            }


            void _showPage_Hardware(){

                if(this->_initialized != true){
                    return;
                }

                this->_activePage = PAGE_HARDWARE;
                this->_clear();

                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32
                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                    this->hardware.setCursor(0, 0);

                    if(this->_productId){
                        this->hardware.print("PID: ");
                        this->hardware.println(this->_productId);
                    }else{
                        this->hardware.println("Unknown Product ID");
                    }

                    if(this->_uuid){
                        this->hardware.print("UUID: ");
                        this->hardware.println(this->_uuid);
                    }else{
                        this->hardware.println("Unknown UUID");
                    }
                  
                #endif

                this->_drawScrollBar(PAGE_HARDWARE);
                this->_commit();
            }
        

            void _showPage_WiFi(){

                if(this->_initialized != true){
                    return;
                }

                #if WIFI_MODEL == ENUM_WIFI_MODEL_ESP32

                    if(this->_wifiInfo->getMode() == wifi_mode_t::WIFI_MODE_NULL){
                        _showPage_Ethernet();
                        return;
                    }

                #endif

                this->_activePage = PAGE_WIFI;
                this->_clear();

                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32

                    const uint8_t logo_width = 20;
                    const uint8_t logo_height = 20;

                    const uint8_t PROGMEM wifi_logo[] = {
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000011,0b11111100,0b00000000,
                        0b00001111,0b11111111,0b00000000,
                        0b00111110,0b00000111,0b11000000,
                        0b01110000,0b00000000,0b11100000,
                        0b01100000,0b00000000,0b01100000,
                        0b00000000,0b11110000,0b00000000,
                        0b00000011,0b11111100,0b00000000,
                        0b00001111,0b10011111,0b00000000,
                        0b00000100,0b00000010,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b11110000,0b00000000,
                        0b00000000,0b01100000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                    };

                    #if WIFI_MODEL == ENUM_WIFI_MODEL_ESP32

                        switch(this->_wifiInfo->getMode()){

                            case wifi_mode_t::WIFI_MODE_AP:

                                this->hardware.drawBitmap(0,0, wifi_logo, logo_width, logo_height, SSD1306_WHITE);
                                this->hardware.setCursor(logo_width + 3, 0);
                                this->hardware.println(this->_wifiInfo->softAPSSID()); 
                                this->hardware.setCursor(logo_width + 3, 8);
                                this->hardware.println(this->_wifiInfo->softAPIP());
                                this->hardware.setCursor(logo_width + 3, 16);
                                this->hardware.println(this->_wifiInfo->softAPmacAddress());
                                this->hardware.setCursor(logo_width + 3, 24);
                                this->hardware.println("Clients: " + String(this->_wifiInfo->softAPgetStationNum()));
                                this->hardware.setCursor(4, logo_height+5);
                                this->hardware.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw black text
                                this->hardware.println("AP");
                                this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                                break;

                            case wifi_mode_t::WIFI_MODE_STA:

                                if(this->_wifiInfo->isConnected() == false){
                                    this->hardware.drawBitmap(0,(OLED_DISPLAY_HEIGHT - logo_height) / 2, wifi_logo, logo_width, logo_height, SSD1306_WHITE);
                                    this->hardware.setCursor(logo_width + 3, 0);
                                    this->hardware.println("Disconnected");
                                    this->hardware.setCursor(logo_width + 3, 16);
                                    this->hardware.println(this->_wifiInfo->macAddress());
                                } else {

                                    this->hardware.drawBitmap(0,0, wifi_logo, logo_width, logo_height, SSD1306_WHITE);
                                    this->hardware.setCursor(logo_width + 3, 0);
                                    this->hardware.println(this->_wifiInfo->SSID());
                                    this->hardware.setCursor(logo_width + 3, 8);
                                    this->hardware.println(this->_wifiInfo->localIP());
                                    this->hardware.setCursor(logo_width + 3, 16);
                                    this->hardware.println(this->_wifiInfo->macAddress());
                                    this->hardware.setCursor(2, logo_height+5);
                                    this->hardware.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw black text
                                    this->hardware.println("STA");
                                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                                }
                                
                                break;

                            case wifi_mode_t::WIFI_MODE_NULL:
                                this->hardware.drawBitmap(0,(OLED_DISPLAY_HEIGHT - logo_height) / 2, wifi_logo, logo_width, logo_height, SSD1306_WHITE);
                                this->hardware.setCursor(logo_width + 10, (OLED_DISPLAY_HEIGHT/2)-3);
                                this->hardware.println("WiFi Off");
                                break;

                            default:
                                this->hardware.drawBitmap(0,(OLED_DISPLAY_HEIGHT - logo_height) / 2, wifi_logo, logo_width, logo_height, SSD1306_WHITE);
                                this->hardware.setCursor(logo_width + 20, (OLED_DISPLAY_HEIGHT/2)-3);
                                this->hardware.println("Unknown");
                                break;
                        }
                    #endif

                #endif
                
                this->_drawScrollBar(PAGE_WIFI);
                this->_commit();
            }


            void _showPage_Ethernet(){

                if(this->_initialized != true){
                    return;
                }

                this->_activePage = PAGE_ETHERNET;
                this->_clear();

                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32

                    const uint8_t logo_width = 20;
                    const uint8_t logo_height = 20;

                    const uint8_t PROGMEM ethernet_logo[] = {
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000010,0b00000100,0b00000000,
                        0b00000110,0b00000110,0b00000000,
                        0b00001100,0b00000011,0b00000000,
                        0b00011000,0b00000001,0b10000000,
                        0b00110010,0b01100100,0b11000000,
                        0b00110010,0b01100100,0b11000000,
                        0b00011000,0b00000001,0b10000000,
                        0b00001100,0b00000011,0b00000000,
                        0b00000110,0b00000110,0b00000000,
                        0b00000010,0b00000100,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                    };

                    this->hardware.drawBitmap(0,5, ethernet_logo, logo_width, logo_height, SSD1306_WHITE);
               
                    #if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_W5500
                        this->hardware.setCursor(logo_width + 3, 16);
                        uint8_t ethMac[6];
                        esp_read_mac(ethMac, ESP_MAC_ETH);
                        char macAddress[18] = {0};
                        sprintf(macAddress, "%02X:%02X:%02X:%02X:%02X:%02X", ethMac[0], ethMac[1], ethMac[2], ethMac[3], ethMac[4], ethMac[5]);
                        this->hardware.println(macAddress);

                        if(ESP32_W5500_isConnected()){
                            this->hardware.setCursor(logo_width + 3, 0);
                            this->hardware.println("Connected");
                            this->hardware.setCursor(logo_width + 3, 8);
                            this->hardware.println(this->_ethernetInfo->localIP());
                            this->hardware.setCursor(logo_width + 3, 24);
                            this->hardware.print(ETH.linkSpeed());
                            this->hardware.print("MB ");
                            if(ETH.fullDuplex()){
                                this->hardware.println("FDX");
                            }else{
                                this->hardware.println("HDX");
                            }
                            

                        }else{
                            this->hardware.setCursor(logo_width + 3, 0);
                            this->hardware.println("Disconnected");
                        }

                    #endif

                #endif

                this->_drawScrollBar(PAGE_ETHERNET);
                this->_commit();
            }


            void _showPage_Auth_Token_Intro(){

                if(this->_initialized != true){
                    return;
                }

                if(!this->_authorizationToken){
                    return;
                }

                this->_activePage = PAGE_AUTH_TOKEN_INTRO;
                this->_wake();
                this->_clear();

                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32

                    const uint8_t logo_width = 20;
                    const uint8_t logo_height = 20;

                    const uint8_t PROGMEM logo[] = {
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00010000,0b01000001,0b00000000,
                        0b00010000,0b01000001,0b00000000,
                        0b01111101,0b11110111,0b11000000,
                        0b00111000,0b11100011,0b10000000,
                        0b00101000,0b10100010,0b10000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b01111111,0b11111111,0b11100000,
                        0b01111111,0b11111111,0b11100000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                    };

                    this->hardware.drawBitmap(0, (OLED_DISPLAY_HEIGHT - logo_height) / 2, logo, logo_width, logo_height, SSD1306_WHITE);
                    this->hardware.setCursor(logo_width + 5, OLED_DISPLAY_HEIGHT / 2);
                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                    this->hardware.setFont(&Prototype9pt7b);
                    this->hardware.println("Auth Token");
                    this->hardware.setFont();
                    this->_drawScrollBar(PAGE_AUTH_TOKEN);

                #endif

                _timeIntroShown = millis();
                this->_commit(); 

            }

        
            void _showPage_EventLog(){

                if(this->_initialized != true){
                    return;
                }

                this->_activePage = PAGE_EVENT_LOG;
                this->_clear();

                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32

                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                    this->hardware.setCursor(0, 0);

                    if(this->_eventLog){

                        int8_t iteratorOledStart = 0;

                        if(this->_eventLog->getEvents()->size() > OLED_NUMBER_OF_LINES){ 
                            iteratorOledStart = this->_eventLog->getEvents()->size()-OLED_NUMBER_OF_LINES;
                        }

                        for(int8_t i = iteratorOledStart; i < this->_eventLog->getEvents()->size(); i++){
                            this->hardware.println(this->_eventLog->getEvents()->get(i).text);
                        }
                    }

                #endif

                this->_drawScrollBar(PAGE_EVENT_LOG);
                this->_commit();

            }


            void _showPage_Error(){

                if(this->_initialized != true){
                    return;
                }

                this->_activePage = PAGE_ERROR;
                this->_clear();

                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32
                    this->hardware.setTextColor(SSD1306_BLACK, SSD1306_WHITE); //Inverted text
                    this->hardware.setCursor(0, 0);
                    this->hardware.println("        ERROR        ");
                    this->hardware.setTextColor(SSD1306_WHITE); //Draw white text
                    this->hardware.setCursor(0, 9);

                    if(this->_eventLog){

                        int8_t iteratorOledStart = 0;

                        if(this->_eventLog->getErrors()->size() > OLED_NUMBER_OF_LINES - 1){ 
                            iteratorOledStart = this->_eventLog->getErrors()->size() - (OLED_NUMBER_OF_LINES - 1);
                        }

                        for(int8_t i = iteratorOledStart; i < this->_eventLog->getErrors()->size(); i++){
                            this->hardware.println(this->_eventLog->getErrors()->get(i));
                        }
                    }
                #endif

                this->_drawScrollBar(PAGE_ERROR);
                this->_commit();
            }

        
            void _showPage_Auth_Token(){

                if(this->_initialized != true){
                    return;
                }

                if(!this->_authorizationToken){
                    return;
                }

                this->_activePage = PAGE_AUTH_TOKEN;
                this->_clear();

                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32

                    this->hardware.setCursor(12, 12);
                    this->hardware.setFont(&Prototype9pt7b);
                    this->hardware.println(this->_authorizationToken->getVisualToken().code);
                    this->hardware.setFont();

                #endif
                
                this->_drawScrollBar(PAGE_AUTH_TOKEN);
                this->_commit();
            }


            void _showPage_OTA_In_Progress(){

                if(this->_initialized != true){
                    return;
                }

                this->_activePage = PAGE_OTA_IN_PROGRESS;
                this->_clear();

                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32

                    
                    this->hardware.setCursor(0, 0);
                    this->hardware.println("      OTA update    ");
                    this->hardware.println("    in progress...  ");

                #endif
                
                this->_commit();
            }


        public:

            void setCallback_failure(void (*userDefinedCallback)(uint8_t, failureReason)) {
                this->ptrFailureCallback = userDefinedCallback; }

            void setProductID(char* value){
                this->_productId = value;
            }

            void setUUID(char* value){
                this->_uuid = value;
            }

            void setName(const char* value){
                this->_name = new char[CONTROLLER_NAME_MAX_LENGTH+1];
                strcpy(this->_name, value);
            }

            #if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_W5500

                void setEthernetInfo(ESP32_W5500 *value){
                    this->_ethernetInfo = value;
                }

            #endif

            #if WIFI_MODEL == ENUM_WIFI_MODEL_ESP32

                void setWiFiInfo(WiFiClass *value){

                    this->_wifiInfo = value;

                }

            #endif

            void setFactoryResetValue(int value){
                this->_factory_reset_value = value;
            }


            void begin(){

                //Only allow init once
                if(this->_initialized == true){
                    return;
                }
                    
                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32

                    //Attempt to connect to the display on the bus
                    Wire.beginTransmission(this->_address);

                    if(Wire.endTransmission()!=0){

                        if(this->ptrFailureCallback){
                            this->ptrFailureCallback(this->_address, failureReason::ADDRESS_OFFLINE);
                        }
                        return;
                    }

                    this->hardware = Adafruit_SSD1306(128, 32, &Wire, -1);

                    if(this->hardware.begin(SSD1306_SWITCHCAPVCC, this->_address) == false){
                        if(this->ptrFailureCallback){
                            this->ptrFailureCallback(this->_address, failureReason::UNABLE_TO_START);
                        }
                        return;
                    }

                    this->hardware.cp437(true);
                #endif

                this->_initialized = true;
                this->_clear();
                this->_showPage_Logo();

            }


            /** Returns the OLED's bus status */
            structHealth health(){

                structHealth returnValue;

                returnValue.address = this->_address;
                returnValue.enabled = this->_initialized;

                return returnValue;
            }


            void setEventLog(EventLog *eventLog){
                _eventLog = eventLog;
            }


            void setAuthorizationToken(authorizationToken *token){
                _authorizationToken = token;
            }

            
            void loop(){

                if(this->_initialized != true){
                    return;
                }

                //If the display is off, exit
                if(this->_isSleeping == true){
                    return;
                }

                //See if it is time to turn off
                if(this->_isDimmed == true){
                    if((unsigned long)(millis() - this->_timeLastAction) > SLEEP_AFTER_MS){

                        log_v("Sleeping OLED");
                        this->_sleep();
                    }
                    return;
                }

                if((unsigned long)(millis() - this->_timeLastAction) > DIM_AFTER_MS){

                    log_v("Dimming OLED");
                    this->_dim();
                }

                switch(this->_activePage){

                    case PAGE_AUTH_TOKEN:
                        this->setProgressBar(this->_authorizationToken->getVisualToken().percentRemaining());
                        break;

                    case PAGE_EVENT_LOG_INTRO:

                        if((unsigned long)(millis() - this->_timeIntroShown) > INTRO_DWELL_MS){
                            setPage(PAGE_EVENT_LOG);
                        } 
                        break;


                    case PAGE_NETWORK_INTRO:

                        if((unsigned long)(millis() - this->_timeIntroShown) > INTRO_DWELL_MS){

                            #if WIFI_MODEL != ENUM_WIFI_NONE
                                setPage(PAGE_WIFI);
                                break;
                            #endif

                            #if ETHERNET_MODEL != ENUM_ETHERNET_MODEL_NONE
                                setPage(PAGE_ETHERNET);
                                break;
                            #endif

                        }
                        break;

                    case PAGE_HARDWARE_INTRO:

                        if((unsigned long)(millis() - this->_timeIntroShown) > INTRO_DWELL_MS){
                            setPage(PAGE_HARDWARE);
                        } 
                        break;

                    case PAGE_SOFTWARE_INTRO:

                        if((unsigned long)(millis() - this->_timeIntroShown) > INTRO_DWELL_MS){
                            setPage(PAGE_SOFTWARE);
                        } 
                        break;

                    case PAGE_ERROR_INTRO:
                        
                        if((unsigned long)(millis() - this->_timeIntroShown) > INTRO_DWELL_MS){
                            setPage(PAGE_ERROR);
                        } 
                        break;

                    case PAGE_AUTH_TOKEN_INTRO:
                        
                        if((unsigned long)(millis() - this->_timeIntroShown) > INTRO_DWELL_MS){
                            setPage(PAGE_AUTH_TOKEN);
                        } 
                        break;

                    default:
                        break;
                }
              
            }


            void setPage(pages requestedPage){

                if(this->_initialized != true){
                    return;
                }

                this->_wake();

                switch(requestedPage){

                    case PAGE_EVENT_LOG:
                        _showPage_EventLog();
                        break;

                    case PAGE_EVENT_LOG_INTRO:
                        _showPage_EventLog_Intro();
                        break;

                    case PAGE_NETWORK_INTRO:
                        #if WIFI_MODEL != ENUM_WIFI_MODEL_NONE || ETHERNET_MODEL != ENUM_ETHERNET_MODEL_NONE
                            _showPage_Network_Intro();
                        #endif
                        break;

                    case PAGE_WIFI:
                        #if WIFI_MODEL != ENUM_WIFI_MODEL_NONE
                            _showPage_WiFi();
                        #endif
                        break;

                    case PAGE_ETHERNET:
                        #if ETHERNET_MODEL != ENUM_ETHERNET_MODEL_NONE
                            _showPage_Ethernet();
                        #endif
                        break;

                    case PAGE_HARDWARE_INTRO:
                        _showPage_Hardware_Intro();
                        break;

                    case PAGE_HARDWARE:
                        _showPage_Hardware();
                        break;

                    case PAGE_SOFTWARE_INTRO:
                        _showPage_Software_Intro();
                        break;

                    case PAGE_SOFTWARE:
                        _showPage_Software();
                        break;

                    case PAGE_ERROR_INTRO:
                        _showPage_Error_Intro();
                        break;

                    case PAGE_ERROR:
                        _showPage_Error();
                        break;

                    case PAGE_AUTH_TOKEN_INTRO:
                        _showPage_Auth_Token_Intro();
                        break;

                    case PAGE_AUTH_TOKEN:
                        _showPage_Auth_Token();
                        break;


                    case PAGE_OTA_IN_PROGRESS:
                        _showPage_OTA_In_Progress();
                        break;

                    case PAGE_FACTORY_RESET:
                        _showPage_Factory_Reset();
                        break;
                }
                
            }


            /**
             * Returns the current page being displayed on the OLED
            */
            pages getPage(){
                return _activePage;
            }


            void nextPage(){

                if(this->_initialized != true){
                    return;
                }

                //If the display is asleep, simply turn it on
                if(this->_isSleeping == true){
                    setPage(PAGE_EVENT_LOG_INTRO);
                    return;
                }

                this->_extendWake();

                //Proceed to the next screen
                switch(this->_activePage){

                    case PAGE_EVENT_LOG_INTRO:
                        setPage(PAGE_NETWORK_INTRO);
                        break;

                    case PAGE_EVENT_LOG:
                        setPage(PAGE_NETWORK_INTRO);
                        break;

                    case PAGE_NETWORK_INTRO:
                        setPage(PAGE_HARDWARE_INTRO);
                        break;
                    
                    case PAGE_WIFI:

                        #if ETHERNET_MODEL == ENUM_ETHERNET_MODEL_NONE
                            setPage(PAGE_HARDWARE_INTRO);
                            break;
                        #else
                            setPage(PAGE_ETHERNET);
                            break;
                        #endif

                    case PAGE_ETHERNET:
                        setPage(PAGE_HARDWARE_INTRO);
                        break;

                    case PAGE_HARDWARE_INTRO:
                    case PAGE_HARDWARE:
                        setPage(PAGE_SOFTWARE_INTRO);
                        break;

                    case PAGE_SOFTWARE_INTRO:
                    case PAGE_SOFTWARE:

                        //Only show the error page if there is an error, otherwise show the event log
                        if(this->_eventLog){
                            if(this->_eventLog->getErrors()->size() > 0){
                                setPage(PAGE_ERROR_INTRO);
                                break;
                            }
                        }

                        if(this->_authorizationToken){
                            setPage(PAGE_AUTH_TOKEN_INTRO);
                            break;
                        }

                        setPage(PAGE_EVENT_LOG_INTRO);
                        break;

                    case PAGE_ERROR_INTRO:
                    case PAGE_ERROR:
                        if(this->_authorizationToken){
                            setPage(PAGE_AUTH_TOKEN_INTRO);
                            break;
                        }

                        setPage(PAGE_EVENT_LOG_INTRO);
                        break;

                    case PAGE_AUTH_TOKEN_INTRO:
                        setPage(PAGE_EVENT_LOG_INTRO);
                        break;

                    case PAGE_AUTH_TOKEN:
                        setPage(PAGE_EVENT_LOG_INTRO);
                        break;

                    default:
                        //Do nothing -- could be an OTA update running or other event that we don't want the screen interrupted
                        break;
                }
            }
    

            /**
             * Sets the progress bar value for pages supporting a progress bar
             * @param percentage the percentage complete, where value <= 1.0
            */
            void setProgressBar(float percentage){

                if(_activePage != PAGE_AUTH_TOKEN && _activePage != PAGE_OTA_IN_PROGRESS ){
                    return;
                }

                this->_wake();

                #if OLED_DISPLAY_MODEL == ENUM_OLED_MODEL_SSD1306_128_32

                    const uint8_t PROGRESS_BAR_HEIGHT = 6;
                    const uint8_t PROGRESS_BAR_BORDER = 1;
                    const uint8_t MARGIN_FROM_SCROLLBAR = 3;
                    
                    this->hardware.drawRect(0, (OLED_DISPLAY_HEIGHT-PROGRESS_BAR_HEIGHT), (OLED_DISPLAY_WIDTH - OLED_SCROLL_BAR_WIDTH - MARGIN_FROM_SCROLLBAR), PROGRESS_BAR_HEIGHT, SSD1306_WHITE); //Draw a white border
                    this->hardware.fillRect(PROGRESS_BAR_BORDER, (OLED_DISPLAY_HEIGHT - PROGRESS_BAR_HEIGHT + PROGRESS_BAR_BORDER), (OLED_DISPLAY_WIDTH - OLED_SCROLL_BAR_WIDTH - (PROGRESS_BAR_BORDER * 2) - MARGIN_FROM_SCROLLBAR), PROGRESS_BAR_HEIGHT - (PROGRESS_BAR_BORDER * 2), SSD1306_BLACK); //Draw progress bar at 0%
                    this->hardware.fillRect(PROGRESS_BAR_BORDER, (OLED_DISPLAY_HEIGHT - PROGRESS_BAR_HEIGHT + PROGRESS_BAR_BORDER), round((float)(OLED_DISPLAY_WIDTH - OLED_SCROLL_BAR_WIDTH - PROGRESS_BAR_BORDER - MARGIN_FROM_SCROLLBAR) * percentage),  PROGRESS_BAR_HEIGHT - (PROGRESS_BAR_BORDER * 2), SSD1306_WHITE); //Fill progress bar with correct percentage

                #endif

                this->_commit();
            }


            /*
            * Retrieves the status if the OLED is sleeping
            */
            boolean isSleeping(){
                return this->_isSleeping;
            }
    };

#endif