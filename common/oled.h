#include "hardware.h"
#include <WiFi.h>
#include "Prototype9pt7b.h"

#ifndef oled_h
    #define oled_h

    #define COUNT_PAGES 6 //The total number of pages without an error
    #define INTRO_DWELL_MS 750 //Number of milliseconds that the intro page should be shown when switching screens

    class managerOled{

        public:

            enum failureCode{
                NOT_ON_BUS = 0, /* Indicates the device was not found on the bus */
                UNABLE_TO_START = 1 /* Indicates the underlying hardware library returned a fault when attempting to begin communications */
            };

            enum pages{
                PAGE_EVENT_LOG = 1,
                PAGE_EVENT_LOG_INTRO = 10,
                PAGE_WIFI = 2,
                PAGE_ETHERNET = 3,
                PAGE_NETWORK_INTRO = 20,
                PAGE_STATUS = 4,
                PAGE_STATUS_INTRO = 40,
                PAGE_HARDWARE = 5,
                PAGE_HARDWARE_INTRO = 50,
                PAGE_SOFTWARE = 6,
                PAGE_SOFTWARE_INTRO = 60,
                PAGE_ERROR = 7,
                PAGE_ERROR_INTRO = 70,
                PAGE_FACTORY_RESET = 1000
            };

            enum logLevel{
                LOG_LEVEL_INFO = 0,
                LOG_LEVEL_NOTIFICATION = 1,
                LOG_LEVEL_ERROR = 2
            };

        private:

            void (*ptrFailureCallback)(failureCode);

            bool _initialized = false;
            char* _productId;
            char* _uuid;
            WiFiClass *_wifiInfo;
            IPAddress _ethernetIp;
            pages _activePage = PAGE_EVENT_LOG;
            boolean _isSleeping = false;
            boolean _isDimmed = false;
            char _errorText[CHARACTERS_PER_LINE * 2];
            int _factory_reset_value = 0;
            char events[NUMBER_OF_LINES][CHARACTERS_PER_LINE + 1];
            unsigned long _timeLastAction = 0;
            unsigned long _timeIntroShown = 0;

            #define DIM_AFTER_MS 10000
            #define SLEEP_AFTER_MS 15000

            #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32
                Adafruit_SSD1306 hardware;
            #endif


            void _clear(){

                if(this->_initialized != true){
                    return;
                }

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32
                    this->hardware.clearDisplay();
                    this->hardware.invertDisplay(false);
                #endif
            }


            void _commit(){

                if(this->_initialized != true){
                    return;
                }

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32
                    this->hardware.display();
                #endif

            }


            void _dim(){

                if(this->_initialized != true){
                    return;
                }

                if(strlen(this->_errorText)>0){
                    _extendWake();
                    return;
                }

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32
                    this->hardware.dim(true);
                #endif

                this->_isDimmed = true;
                this->_isSleeping = false;
            }


            void _sleep(){

                if(this->_initialized != true){
                    return;
                }

                if(strlen(this->_errorText)>0){
                    _extendWake();
                    return;
                }

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32
                    this->hardware.ssd1306_command(SSD1306_DISPLAYOFF);
                #endif

                this->_isSleeping = true;
                this->_isDimmed = false;
            }
            
            
            void _wake(){

                if(this->_initialized != true){
                    return;
                }

                #if DEBUG > 4000
                    Serial.println(F("[oled] (_wake) Woke up"));
                #endif

                if(this->_isDimmed == true){

                    #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32
                        this->hardware.dim(false);
                    #endif

                    this->_isDimmed = false;
                }

                if(this->_isSleeping == true){

                    #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32
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

                #if DEBUG > 4000
                    Serial.println(F("[oled] (_extendWake) Extending Wake"));
                #endif

                _timeLastAction = millis();
            }


            void _drawScrollBar(byte page){

                if(this->_initialized != true){
                    return;
                }

                byte total = COUNT_PAGES;

                if(strlen(this->_errorText)>0){
                    total = total +1;
                }

                int val = map(page, 1, total, 1, DISPLAY_HEIGHT-SCROLL_BAR_HEIGHT-1);

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32
                    this->hardware.setTextColor(SSD1306_WHITE);
                    this->hardware.fillRect((DISPLAY_WIDTH-SCROLL_BAR_WIDTH),0,SCROLL_BAR_WIDTH, DISPLAY_HEIGHT, SSD1306_WHITE); //Scroll Bar
                    this->hardware.fillRect((DISPLAY_WIDTH-SCROLL_BAR_WIDTH),val,SCROLL_BAR_WIDTH,SCROLL_BAR_HEIGHT, SSD1306_BLACK); //Scroll position
                #endif      
            }


            void _showPage_Logo(){

                if(this->_initialized != true){
                    return;
                }

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32
                    #define LOGO_WIDTH  20
                    #define LOGO_HEIGHT 10

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

                    this->_wake();
                    this->_clear();
                    this->hardware.drawBitmap(0, (DISPLAY_HEIGHT - LOGO_HEIGHT) / 2, logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                    this->hardware.setCursor(LOGO_WIDTH + 2, DISPLAY_HEIGHT / 2);
                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                    this->hardware.setFont(&Prototype9pt7b);
                    this->hardware.println(F("P5 Software"));
                    this->hardware.setFont();

                #endif

                this->_commit();

            }

        
            void _showPage_Software(){

                if(this->_initialized != true){
                    return;
                }

                this->_clear();

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32
                    this->hardware.setCursor(0, 0);
                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text

                    this->hardware.print(F("Ver: "));
                    this->hardware.println(VERSION);                    
                #endif

                this->_drawScrollBar(PAGE_SOFTWARE);
                this->_commit();
            }


            void _showPage_EventLog_Intro(){

                if(this->_initialized != true){
                    return;
                }

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32

                    #define LOGO_WIDTH  20
                    #define LOGO_HEIGHT 20

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

                    this->_wake();
                    this->_clear();
                    this->hardware.drawBitmap(0, (DISPLAY_HEIGHT - LOGO_HEIGHT) / 2, logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                    this->hardware.setCursor(LOGO_WIDTH + 5, DISPLAY_HEIGHT / 2);
                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                    this->hardware.setFont(&Prototype9pt7b);
                    this->hardware.println(F("Event Log"));
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

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32

                    #define LOGO_WIDTH  20
                    #define LOGO_HEIGHT 20

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

                    this->_wake();
                    this->_clear();
                    this->hardware.drawBitmap(0, (DISPLAY_HEIGHT - LOGO_HEIGHT) / 2, logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                    this->hardware.setCursor(LOGO_WIDTH + 5, DISPLAY_HEIGHT / 2);
                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                    this->hardware.setFont(&Prototype9pt7b);
                    this->hardware.println(F("Network"));
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

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32

                    #define LOGO_WIDTH  20
                    #define LOGO_HEIGHT 20

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

                    this->_wake();
                    this->_clear();
                    this->hardware.drawBitmap(0, (DISPLAY_HEIGHT - LOGO_HEIGHT) / 2, logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                    this->hardware.setCursor(LOGO_WIDTH + 5, DISPLAY_HEIGHT / 2);
                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                    this->hardware.setFont(&Prototype9pt7b);
                    this->hardware.println(F("Hardware"));
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

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32

                    #define LOGO_WIDTH  20
                    #define LOGO_HEIGHT 20

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

                    this->_wake();
                    this->_clear();
                    this->hardware.drawBitmap(0, (DISPLAY_HEIGHT - LOGO_HEIGHT) / 2, logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                    this->hardware.setCursor(LOGO_WIDTH + 5, DISPLAY_HEIGHT / 2);
                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                    this->hardware.setFont(&Prototype9pt7b);
                    this->hardware.println(F("Software"));
                    this->hardware.setFont();
                    this->_drawScrollBar(PAGE_SOFTWARE);

                #endif

                _timeIntroShown = millis();
                this->_commit();  

            }


            void _showPage_Status_Intro(){

                if(this->_initialized != true){
                    return;
                }

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32

                    #define LOGO_WIDTH  20
                    #define LOGO_HEIGHT 20

                    const uint8_t PROGMEM logo[] = {
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b11110000,0b00000000,
                        0b00000011,0b11111100,0b00000000,
                        0b00000111,0b11111110,0b00000000,
                        0b00001111,0b11111111,0b00000000,
                        0b00011111,0b11111111,0b10000000,
                        0b00011111,0b11111011,0b10000000,
                        0b00111111,0b11110011,0b11000000,
                        0b00111111,0b11100111,0b11000000,
                        0b00111110,0b01001111,0b11000000,
                        0b00111111,0b00011111,0b11000000,
                        0b00011111,0b10111111,0b10000000,
                        0b00011111,0b11111111,0b10000000,
                        0b00001111,0b11111111,0b00000000,
                        0b00000111,0b11111110,0b00000000,
                        0b00000011,0b11111100,0b00000000,
                        0b00000000,0b11110000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                        0b00000000,0b00000000,0b00000000,
                    };

                    this->_wake();
                    this->_clear();
                    this->hardware.drawBitmap(0, (DISPLAY_HEIGHT - LOGO_HEIGHT) / 2, logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                    this->hardware.setCursor(LOGO_WIDTH + 5, DISPLAY_HEIGHT / 2);
                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                    this->hardware.setFont(&Prototype9pt7b);
                    this->hardware.println(F("Status"));
                    this->hardware.setFont();
                    this->_drawScrollBar(PAGE_STATUS);

                #endif

                _timeIntroShown = millis();
                this->_commit();   
            }


            void _showPage_Error_Intro(){

                if(this->_initialized != true){
                    return;
                }

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32

                    #define LOGO_WIDTH  20
                    #define LOGO_HEIGHT 20

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

                    this->_wake();
                    this->_clear();
                    this->hardware.drawBitmap(0, (DISPLAY_HEIGHT - LOGO_HEIGHT) / 2, logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                    this->hardware.setCursor(LOGO_WIDTH + 5, DISPLAY_HEIGHT / 2);
                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                    this->hardware.setFont(&Prototype9pt7b);
                    this->hardware.println(F("Error"));
                    this->hardware.setFont();
                    this->_drawScrollBar(PAGE_STATUS);

                #endif

                _timeIntroShown = millis();
                this->_commit();   
            }

            void _showPage_Factory_Reset(){

                if(this->_initialized != true){
                    return;
                }

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32

                    this->_wake();
                    this->_clear();

                    if (this->_factory_reset_value % 2 == 0){
                        this->hardware.invertDisplay(false);
                    }else{
                        this->hardware.invertDisplay(true);
                    }

                    this->hardware.setCursor(0, 0);
                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                    this->hardware.println(F("     Factory Reset    "));
                    this->hardware.setFont(&Prototype9pt7b);
                    this->hardware.setCursor(DISPLAY_WIDTH / 2, 26);
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

                this->_clear();

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32
                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                    this->hardware.setCursor(0, 0);

                    if(this->_productId){
                        this->hardware.print(F("PID: "));
                        this->hardware.println(this->_productId);
                    }else{
                        this->hardware.println(F("Unknown Product ID"));
                    }

                    if(this->_uuid){
                        this->hardware.print(F("UUID: "));
                        this->hardware.println(this->_uuid);
                    }else{
                        this->hardware.println(F("Unknown UUID"));
                    }
                  
                #endif

                this->_drawScrollBar(PAGE_HARDWARE);
                this->_commit();
            }
        

            void _showPage_WiFi(){

                if(this->_initialized != true){
                    return;
                }

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32

                    #define LOGO_WIDTH  20
                    #define LOGO_HEIGHT 20

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

                    this->_clear();

                    switch(this->_wifiInfo->getMode()){

                        case wifi_mode_t::WIFI_MODE_AP:

                            this->hardware.drawBitmap(0,0, wifi_logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                            this->hardware.setCursor(LOGO_WIDTH + 3, 0);
                            this->hardware.println(this->_wifiInfo->softAPSSID()); 
                            this->hardware.setCursor(LOGO_WIDTH + 3, 8);
                            this->hardware.println(this->_wifiInfo->softAPIP());
                            this->hardware.setCursor(LOGO_WIDTH + 3, 16);
                            this->hardware.println(this->_wifiInfo->softAPmacAddress());
                            this->hardware.setCursor(LOGO_WIDTH + 3, 24);
                            this->hardware.println("Clients: " + String(this->_wifiInfo->softAPgetStationNum()));
                            this->hardware.setCursor(4, LOGO_HEIGHT+5);
                            this->hardware.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw black text
                            this->hardware.println(F("AP"));
                            this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                            break;

                        case wifi_mode_t::WIFI_MODE_STA:

                            if(this->_wifiInfo->isConnected() == false){
                                this->hardware.drawBitmap(0,(DISPLAY_HEIGHT - LOGO_HEIGHT) / 2, wifi_logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                                this->hardware.setCursor(LOGO_WIDTH + 16, (DISPLAY_HEIGHT/2)-3);
                                this->hardware.println(F("Disconnected"));
                            } else {

                                this->hardware.drawBitmap(0,0, wifi_logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                                this->hardware.setCursor(LOGO_WIDTH + 3, 0);
                                this->hardware.println(this->_wifiInfo->SSID());
                                this->hardware.setCursor(LOGO_WIDTH + 3, 8);
                                this->hardware.println(this->_wifiInfo->localIP());
                                this->hardware.setCursor(LOGO_WIDTH + 3, 16);
                                this->hardware.println(this->_wifiInfo->macAddress());
                                this->hardware.setCursor(2, LOGO_HEIGHT+5);
                                this->hardware.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw black text
                                this->hardware.println(F("STA"));
                                this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                            }
                            
                            break;

                        case wifi_mode_t::WIFI_MODE_NULL:
                            this->hardware.drawBitmap(0,(DISPLAY_HEIGHT - LOGO_HEIGHT) / 2, wifi_logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                            this->hardware.setCursor(LOGO_WIDTH + 10, (DISPLAY_HEIGHT/2)-3);
                            this->hardware.println(F("Not Initialized"));
                            break;

                        default:
                            this->hardware.drawBitmap(0,(DISPLAY_HEIGHT - LOGO_HEIGHT) / 2, wifi_logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                            this->hardware.setCursor(LOGO_WIDTH + 20, (DISPLAY_HEIGHT/2)-3);
                            this->hardware.println(F("Unknown"));
                            break;
                    }

                #endif
                
                this->_drawScrollBar(PAGE_WIFI);
                this->_commit();
            }


            void _showPage_Ethernet(){

                if(this->_initialized != true){
                    return;
                }

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32

                    #define LOGO_WIDTH  20
                    #define LOGO_HEIGHT 20

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

                    this->_clear();
                    this->hardware.drawBitmap(0,0, ethernet_logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                #endif

                this->_drawScrollBar(PAGE_ETHERNET);
                this->_commit();

            }

        
            void _showPage_EventLog(){

                if(this->_initialized != true){
                    return;
                }

                this->_clear();

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32

                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                    this->hardware.setCursor(0, 0);
                    for(unsigned int i=0; i < 4; i++){
                        this->hardware.println(events[i]);
                    }
                #endif

                this->_drawScrollBar(PAGE_EVENT_LOG);
                this->_commit();

            }


            void _showPage_Status(){

                if(this->_initialized != true){
                    return;
                }

                this->_clear();

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32
                    this->hardware.setTextColor(SSD1306_WHITE); // Draw white text
                    this->hardware.setCursor(0, 0); 
                    this->hardware.println("STATUS STUB");
                #endif

                this->_drawScrollBar(PAGE_STATUS);
                this->_commit();

            }


            void _showPage_Error(){

                if(this->_initialized != true){
                    return;
                }

                this->_clear();

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32
                    this->hardware.setTextColor(SSD1306_BLACK, SSD1306_WHITE); //Inverted text
                    this->hardware.setCursor(0, 0);
                    this->hardware.println(F("        ERROR        "));
                    this->hardware.setTextColor(SSD1306_WHITE); //Draw white text
                    this->hardware.setCursor(0, 9); 
                    this->hardware.println(this->_errorText);
                #endif

                this->_drawScrollBar(PAGE_ERROR);
                this->_commit();
            }

        public:

            void setCallback_failure(void (*userDefinedCallback)(failureCode)) {
                this->ptrFailureCallback = userDefinedCallback; }

            void setProductID(char* value){
                this->_productId = value;
            }

            void setUUID(char* value){
                this->_uuid = value;
            }

            void setEthernetIPAddress(IPAddress value){
                this->_ethernetIp = value;
            }

            void setWiFiInfo(WiFiClass *value){

                this->_wifiInfo = value;

                if(this->_wifiInfo->getMode() == wifi_mode_t::WIFI_MODE_NULL){
                    #ifdef DEBUG
                        Serial.println(F("[oled] (setWiFiInfo) Attempted to set WiFi it has not been initialized (WIFI_MODE_NULL)"));
                    #endif
                    return;
                }

            }

            void setFactoryResetValue(int value){
                this->_factory_reset_value = value;
            }


            void begin(){

                //Only allow init once
                if(this->_initialized == true){
                    return;
                }
                    
                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32

                    //Attempt to connect to the display on the bus
                    Wire.beginTransmission(ADDRESS_OLED);

                    if(Wire.endTransmission()!=0){

                        if(this->ptrFailureCallback){
                            this->ptrFailureCallback(failureCode::NOT_ON_BUS);
                        }
                        return;
                    }

                    this->hardware = Adafruit_SSD1306(128, 32, &Wire, -1);

                    if(this->hardware.begin(SSD1306_SWITCHCAPVCC, ADDRESS_OLED) == false){
                        if(this->ptrFailureCallback){
                            this->ptrFailureCallback(failureCode::UNABLE_TO_START);
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

                returnValue.address = ADDRESS_OLED;
                returnValue.enabled = this->_initialized;

                return returnValue;
            }


            void logEvent(const char* text, logLevel type){
     
                //Copy the elements down one element
                for(int i = 2; i >= 0; i--){

                    strncpy(this->events[i+1], this->events[i], CHARACTERS_PER_LINE);
                    
                }

                //Write the requested chars to the first element in the array
                strncpy(this->events[0], text, CHARACTERS_PER_LINE);
                
                //Turn on the display for notification
                if(type == LOG_LEVEL_NOTIFICATION){

                    this->_wake();
                    showPage(PAGE_EVENT_LOG);
                    return;

                }

                //Don't show the event log if we are sleeping because the event isn't important enough
                if(_isSleeping == true){
                    return;
                }

                //Update the page if we are already on the event log page
                if(_activePage == PAGE_EVENT_LOG){
                    showPage(PAGE_EVENT_LOG);
                }
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

                        #if DEBUG > 4000
                            Serial.println(F("[oled] (loop) Sleeping OLED"));
                        #endif

                        this->_sleep();
                    }
                    return;
                }

                if((unsigned long)(millis() - this->_timeLastAction) > DIM_AFTER_MS){

                    #if DEBUG > 4000
                        Serial.println(F("[oled] (loop) Dimming OLED"));
                    #endif

                    this->_dim();
                }

                switch(this->_activePage){

                    case PAGE_EVENT_LOG_INTRO:

                        if((unsigned long)(millis() - this->_timeIntroShown) > INTRO_DWELL_MS){
                            showPage(PAGE_EVENT_LOG);
                        } 
                        break;


                    case PAGE_NETWORK_INTRO:

                        if((unsigned long)(millis() - this->_timeIntroShown) > INTRO_DWELL_MS){
                            showPage(PAGE_WIFI);
                        } 
                        break;

                    case PAGE_STATUS_INTRO:

                        if((unsigned long)(millis() - this->_timeIntroShown) > INTRO_DWELL_MS){
                            showPage(PAGE_STATUS);
                        } 
                        break;

                    case PAGE_HARDWARE_INTRO:

                        if((unsigned long)(millis() - this->_timeIntroShown) > INTRO_DWELL_MS){
                            showPage(PAGE_HARDWARE);
                        } 
                        break;

                    case PAGE_SOFTWARE_INTRO:

                        if((unsigned long)(millis() - this->_timeIntroShown) > INTRO_DWELL_MS){
                            showPage(PAGE_SOFTWARE);
                        } 
                        break;

                    case PAGE_ERROR_INTRO:
                        
                        if((unsigned long)(millis() - this->_timeIntroShown) > INTRO_DWELL_MS){
                            showPage(PAGE_ERROR);
                        } 
                        break;
                }
              
            }

            void showError(const char* text){

                //Log the error text
                logEvent(text, this->LOG_LEVEL_ERROR);

                //Set the errorText
                strncpy(this->_errorText, text, sizeof(_errorText));

                //Show the error screen
                showPage(PAGE_ERROR);

            }


            void clearError(){
                strcpy(this->_errorText, "");
                showPage(PAGE_EVENT_LOG);
            }


            void showPage(pages requestedPage){

                if(this->_initialized != true){
                    return;
                }

                this->_wake();

                switch(requestedPage){

                    case PAGE_EVENT_LOG:
                        this->_activePage = PAGE_EVENT_LOG;
                        _showPage_EventLog();
                        break;

                    case PAGE_EVENT_LOG_INTRO:
                        this->_activePage = PAGE_EVENT_LOG_INTRO;
                        _showPage_EventLog_Intro();
                        break;
                    
                    case PAGE_WIFI:
                        this->_activePage = PAGE_WIFI;
                        _showPage_WiFi();
                        break;

                    case PAGE_ETHERNET:
                        this->_activePage = PAGE_ETHERNET;
                        _showPage_Ethernet();
                        break;

                    case PAGE_NETWORK_INTRO:
                        this->_activePage = PAGE_NETWORK_INTRO;
                        _showPage_Network_Intro();
                        break;

                    case PAGE_STATUS:
                        this->_activePage = PAGE_STATUS;
                        _showPage_Status();
                        break;

                    case PAGE_STATUS_INTRO:
                        this->_activePage = PAGE_STATUS_INTRO;
                        _showPage_Status_Intro();
                        break;

                    case PAGE_HARDWARE:
                        this->_activePage = PAGE_HARDWARE;
                        _showPage_Hardware();
                        break;

                    case PAGE_HARDWARE_INTRO:
                        this->_activePage = PAGE_HARDWARE_INTRO;
                        _showPage_Hardware_Intro();
                        break;

                    case PAGE_SOFTWARE:
                        this->_activePage = PAGE_SOFTWARE;
                        _showPage_Software();
                        break;

                    case PAGE_SOFTWARE_INTRO:
                        this->_activePage = PAGE_SOFTWARE_INTRO;
                        _showPage_Software_Intro();
                        break;

                    case PAGE_ERROR:
                        this->_activePage = PAGE_ERROR;
                        _showPage_Error();
                        break;

                    case PAGE_ERROR_INTRO:
                        this->_activePage = PAGE_ERROR_INTRO;
                        _showPage_Error_Intro();
                        break;

                    case PAGE_FACTORY_RESET:
                        this->_activePage = PAGE_FACTORY_RESET;
                        _showPage_Factory_Reset();
                        break;
                }
                
            }


            void nextPage(){

                if(this->_initialized != true){
                    return;
                }

                //If the display is asleep, simply turn it on
                if(this->_isSleeping == true){
                    showPage(PAGE_EVENT_LOG_INTRO);
                    return;
                }

                this->_extendWake();

                //Proceed to the next screen
                switch(this->_activePage){

                    case PAGE_EVENT_LOG:
                        showPage(PAGE_NETWORK_INTRO);
                        break;

                    case PAGE_EVENT_LOG_INTRO:
                        showPage(PAGE_NETWORK_INTRO);
                        break;
                    
                    case PAGE_WIFI:
                        showPage(PAGE_ETHERNET);
                        break;

                    case PAGE_ETHERNET:
                    case PAGE_NETWORK_INTRO:
                        showPage(PAGE_STATUS_INTRO);
                        break;

                    case PAGE_STATUS:
                    case PAGE_STATUS_INTRO:
                        showPage(PAGE_HARDWARE_INTRO);
                        break;

                    case PAGE_HARDWARE:
                    case PAGE_HARDWARE_INTRO:
                        showPage(PAGE_SOFTWARE_INTRO);
                        break;

                    case PAGE_SOFTWARE:
                    case PAGE_SOFTWARE_INTRO:

                        //Only show the error page if there is an error, otherwise show the event log
                        if(strlen(this->_errorText)>0){
                            showPage(PAGE_ERROR_INTRO);
                            return;
                        }
                        showPage(PAGE_EVENT_LOG_INTRO);
                        break;

                    case PAGE_ERROR:
                    case PAGE_ERROR_INTRO:
                        showPage(PAGE_EVENT_LOG_INTRO);
                        break;

                }
            }
    };

#endif