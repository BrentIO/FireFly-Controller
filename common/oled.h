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
            boolean _isInitialized = 0;
            boolean _isSleeping = false;
            boolean _isDimmed = false;
            char* _errorText = "";
            char events[NUMBER_OF_LINES][CHARACTERS_PER_LINE];
            unsigned long _timeLastAction = 0;

            #define DIM_AFTER_MS 10000
            #define SLEEP_AFTER_MS 15000

            #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32
                Adafruit_SSD1306 _display;
            #endif


            void _clear(){

                if(this->_initialized != true){
                    return;
                }

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32
                    this->_display.clearDisplay();
                    this->_display.invertDisplay(false);
                #endif
            }


            void _commit(){

                if(this->_initialized != true){
                    return;
                }

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32
                    this->_display.display();

                #endif

            }

            void _dim(){

                if(this->_initialized != true){
                    return;
                }

                this->_display.dim(true);
                this->_isDimmed = true;
            }

            void _sleep(){
                this->_display.ssd1306_command(SSD1306_DISPLAYOFF);

                if(this->_initialized != true){
                    return;
                }

                this->_isSleeping = true;
                this->_isDimmed = false;
            }
            
            
            void _wake(){

                if(this->_initialized != true){
                    return;
                }

                #if DEBUG > 400
                    Serial.println("[oled] (_wake) Woke up");
                #endif

                this->_display.dim(false);
                
                if(this->_isSleeping == true){
                    this->_display.ssd1306_command(SSD1306_DISPLAYON);
                }      

                this->_isDimmed = false;
                this->_isSleeping = false;

                _extendWake();

            }


            void _extendWake(void){

                if(this->_initialized != true){
                    return;
                }

                #if DEBUG > 400
                    Serial.println("[oled] (_extendWake) Extending Wake");
                #endif

                _timeLastAction = millis();
            }


            void _drawScrollBar(byte page){

                if(this->_initialized != true){
                    return;
                }

                byte total = COUNT_PAGES;

                if(_errorText != ""){
                    total = total +1;
                }

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32

                    int val = map(page, 1, total, 1, DISPLAY_HEIGHT-SCROLL_BAR_HEIGHT-1);
                    this->_display.setTextColor(SSD1306_WHITE);
                    this->_display.fillRect((DISPLAY_WIDTH-SCROLL_BAR_WIDTH),0,SCROLL_BAR_WIDTH, DISPLAY_HEIGHT, SSD1306_WHITE); //Scroll Bar
                    this->_display.fillRect((DISPLAY_WIDTH-SCROLL_BAR_WIDTH),val,SCROLL_BAR_WIDTH,SCROLL_BAR_HEIGHT, SSD1306_BLACK); //Scroll position

                #endif      
            }


            void _showPage_Logo(){

                if(this->_initialized != true){
                    return;
                }

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32
                    #define LOGO_WIDTH  128
                    #define LOGO_HEIGHT 15

                    const uint8_t PROGMEM p5software_logo[] = {
                        0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,
                        0b00000000,0b01101101,0b10110110,0b01111111,0b10001111,0b11111000,0b00000011,0b11111000,0b00000000,0b00000111,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,
                        0b00000000,0b11011011,0b11110110,0b01111111,0b11001111,0b11111000,0b00000111,0b11111000,0b00000000,0b00001111,0b00010000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,
                        0b00000001,0b11111111,0b01101100,0b01100000,0b11001100,0b00000000,0b00000110,0b00000000,0b00000000,0b00001100,0b00010000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,
                        0b00000011,0b10110110,0b11011000,0b01100000,0b11001100,0b00000000,0b00000110,0b00000000,0b00000000,0b00011100,0b00111000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,
                        0b00000011,0b01101101,0b11111000,0b01100000,0b11001100,0b00000000,0b00000110,0b00000000,0b01111111,0b00011110,0b01111100,0b11001111,0b00010001,0b11111100,0b01111000,0b11111100,
                        0b00000110,0b11111111,0b10110000,0b01100000,0b11001111,0b11110000,0b00000111,0b11110000,0b11100011,0b10011100,0b00111000,0b11001111,0b00010000,0b00001100,0b01100001,0b10000110,
                        0b00000000,0b00000000,0b00000000,0b01111111,0b11001111,0b11111000,0b00000011,0b11111100,0b11000001,0b10001000,0b00010000,0b11001101,0b10010000,0b00000100,0b01000011,0b00000010,
                        0b00001101,0b10110110,0b11000000,0b01111111,0b10000000,0b00011100,0b00000000,0b00011100,0b11000001,0b10001000,0b00010000,0b01001101,0b10110000,0b00001100,0b01000011,0b11111110,
                        0b00000000,0b00000000,0b00000000,0b01100000,0b00000000,0b00011100,0b00000000,0b00001100,0b11000001,0b10001000,0b00010000,0b01001101,0b10110001,0b11111100,0b01000011,0b11111110,
                        0b00110111,0b11111101,0b10000000,0b01100000,0b00000000,0b00011100,0b00000000,0b00001100,0b11000001,0b10001000,0b00010000,0b01101001,0b10110001,0b00000100,0b01000011,0b00000000,
                        0b01111110,0b11011011,0b00000000,0b01100000,0b00000000,0b00011000,0b00000000,0b00001100,0b11000001,0b10001000,0b00010000,0b01111001,0b11110001,0b00000100,0b01000011,0b00000000,
                        0b01101101,0b10110111,0b00000000,0b01100000,0b00001111,0b11111000,0b00000111,0b11111100,0b11111111,0b10001000,0b00011100,0b01111001,0b11100001,0b11111100,0b01000001,0b11111100,
                        0b11001001,0b00110110,0b00000000,0b01100000,0b00001111,0b11110000,0b00000111,0b11111000,0b01111111,0b00001000,0b00011100,0b01111000,0b11100001,0b11111100,0b01000000,0b11111100,
                        0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,
                    };

                    this->_display.clearDisplay();
                    this->_display.drawBitmap((DISPLAY_WIDTH - LOGO_WIDTH) / 2, (DISPLAY_HEIGHT - LOGO_HEIGHT) / 2, p5software_logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                #endif

                this->_commit();

            }

        
            void _showPage_Software(){

                if(this->_initialized != true){
                    return;
                }

                this->_clear();

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32
                    this->_display.setCursor(0, 0);
                    this->_display.setTextColor(SSD1306_WHITE); // Draw white text

                    this->_display.print("Ver: ");
                    this->_display.println(VERSION);                    
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
                    this->_display.drawBitmap(0, (DISPLAY_HEIGHT - LOGO_HEIGHT) / 2, logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                    this->_display.setCursor(LOGO_WIDTH + 5, DISPLAY_HEIGHT / 2);
                    this->_display.setTextColor(SSD1306_WHITE); // Draw white text
                    this->_display.setFont(&Prototype9pt7b);
                    this->_display.println("Event Log");
                    this->_display.setFont();
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
                    this->_display.drawBitmap(0, (DISPLAY_HEIGHT - LOGO_HEIGHT) / 2, logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                    this->_display.setCursor(LOGO_WIDTH + 5, DISPLAY_HEIGHT / 2);
                    this->_display.setTextColor(SSD1306_WHITE); // Draw white text
                    this->_display.setFont(&Prototype9pt7b);
                    this->_display.println("Network");
                    this->_display.setFont();
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
                    this->_display.drawBitmap(0, (DISPLAY_HEIGHT - LOGO_HEIGHT) / 2, logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                    this->_display.setCursor(LOGO_WIDTH + 5, DISPLAY_HEIGHT / 2);
                    this->_display.setTextColor(SSD1306_WHITE); // Draw white text
                    this->_display.setFont(&Prototype9pt7b);
                    this->_display.println("Hardware");
                    this->_display.setFont();
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
                    this->_display.drawBitmap(0, (DISPLAY_HEIGHT - LOGO_HEIGHT) / 2, logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                    this->_display.setCursor(LOGO_WIDTH + 5, DISPLAY_HEIGHT / 2);
                    this->_display.setTextColor(SSD1306_WHITE); // Draw white text
                    this->_display.setFont(&Prototype9pt7b);
                    this->_display.println("Software");
                    this->_display.setFont();
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
                    this->_display.drawBitmap(0, (DISPLAY_HEIGHT - LOGO_HEIGHT) / 2, logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                    this->_display.setCursor(LOGO_WIDTH + 5, DISPLAY_HEIGHT / 2);
                    this->_display.setTextColor(SSD1306_WHITE); // Draw white text
                    this->_display.setFont(&Prototype9pt7b);
                    this->_display.println("Status");
                    this->_display.setFont();
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
                    this->_display.drawBitmap(0, (DISPLAY_HEIGHT - LOGO_HEIGHT) / 2, logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                    this->_display.setCursor(LOGO_WIDTH + 5, DISPLAY_HEIGHT / 2);
                    this->_display.setTextColor(SSD1306_WHITE); // Draw white text
                    this->_display.setFont(&Prototype9pt7b);
                    this->_display.println("Error");
                    this->_display.setFont();
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
                        this->_display.invertDisplay(false);
                    }else{
                        this->_display.invertDisplay(true);
                    }

                    this->_display.setCursor(0, 0);
                    this->_display.setTextColor(SSD1306_WHITE); // Draw white text
                    this->_display.println("     Factory Reset    ");
                    this->_display.setFont(&Prototype9pt7b);
                    this->_display.setCursor(DISPLAY_WIDTH / 2, 26);
                    this->_display.println(this->_factory_reset_value);
                    this->_display.setFont();

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
                    this->_display.setTextColor(SSD1306_WHITE); // Draw white text
                    this->_display.setCursor(0, 0);

                    if(this->_productId){
                        this->_display.print("PID: ");
                        this->_display.println(this->_productId);
                    }else{
                        this->_display.println("Unknown Product ID");
                    }

                    if(this->_uuid){
                        this->_display.print("UUID: ");
                        this->_display.println(this->_uuid);
                    }else{
                        this->_display.println("Unknown UUID");
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
                    #define LOGO_HEIGHT 15

                    const uint8_t PROGMEM wifi_logo[] = {
                        0b00000011,0b11111100,0b00000000,
                        0b00011111,0b11111111,0b10000000,
                        0b00111100,0b00000011,0b11000000,
                        0b11110000,0b00000000,0b11110000,
                        0b11000011,0b11111100,0b01110000,
                        0b00001111,0b11111111,0b00000000,
                        0b00011110,0b00000111,0b10000000,
                        0b00011000,0b00000001,0b10000000,
                        0b00000011,0b11111100,0b10000000,
                        0b00000111,0b11111110,0b00000000,
                        0b00000100,0b00000010,0b00000000,
                        0b00000000,0b01100000,0b00000000,
                        0b00000000,0b11110000,0b00000000,
                        0b00000000,0b11110000,0b00000000,
                        0b00000000,0b01100000,0b00000000,
                    };

                    this->_display.clearDisplay();
                    
                    if(this->_wifiInfo->isConnected() == false){
                        this->_display.drawBitmap(0,(DISPLAY_HEIGHT - LOGO_HEIGHT) / 2, wifi_logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                        this->_display.setCursor(LOGO_WIDTH + 16, (DISPLAY_HEIGHT/2)-3);
                        this->_display.println("Disconnected");
                    }else{
                        this->_display.drawBitmap(0,0, wifi_logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                        this->_display.setCursor(LOGO_WIDTH + 3, 0);
                        this->_display.println(this->_wifiInfo->SSID());
                        this->_display.setCursor(LOGO_WIDTH + 3, 8);
                        this->_display.println(this->_wifiInfo->localIP());
                        this->_display.setCursor(LOGO_WIDTH + 3, 16);
                        this->_display.println(this->_wifiInfo->macAddress());

                        this->_display.setCursor(2, LOGO_HEIGHT+5);
                        if(this->_wifiInfo->getMode() == WIFI_MODE_AP){
                            this->_display.println("AP");
                        }
                        if(this->_wifiInfo->getMode() == WIFI_MODE_STA){
                            this->_display.println("STA");
                        }
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
                        0b01111111,0b11111111,0b11100000,
                        0b11000000,0b00000000,0b00110000,
                        0b10000000,0b00000000,0b00010000,
                        0b10111011,0b01101101,0b11010000,
                        0b10111011,0b01101101,0b11010000,
                        0b10111011,0b01101101,0b11010000,
                        0b10111111,0b11111111,0b11010000,
                        0b10111111,0b11111111,0b11010000,
                        0b10111111,0b11111111,0b11010000,
                        0b10111111,0b11111111,0b11010000,
                        0b10111111,0b11111111,0b11010000,
                        0b10111111,0b11111111,0b11010000,
                        0b10011111,0b11111111,0b10010000,
                        0b10000001,0b11111000,0b00010000,
                        0b10000001,0b11111000,0b00010000,
                        0b10000000,0b00000000,0b00010000,
                        0b10000000,0b00000000,0b00010000,
                        0b10000000,0b00000000,0b00010000,
                        0b11000000,0b00000000,0b00110000,
                        0b01111111,0b11111111,0b11100000,
                    };

                        this->_display.clearDisplay();
                        this->_display.drawBitmap(0,0, ethernet_logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
                #endif

                this->_drawScrollBar(PAGE_ETHERNET);
                this->_commit();

            }

        
            void _showPage_EventLog(){

                if(this->_initialized != true){
                    return;
                }

                this->_display.clearDisplay();

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32
                    this->_display.setTextColor(SSD1306_WHITE); // Draw white text
                    this->_display.setCursor(0, 0);
                    for(unsigned int i=0; i < 4; i++){
                        this->_display.println(events[i]);
                    }
                #endif

                this->_drawScrollBar(PAGE_EVENT_LOG);
                this->_commit();

            }


            void _showPage_Status(){

                if(this->_initialized != true){
                    return;
                }

                this->_display.clearDisplay();

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32
                    this->_display.setTextColor(SSD1306_WHITE); // Draw white text
                    this->_display.setCursor(0, 0); 
                    this->_display.println("STATUS STUB");
                #endif

                this->_drawScrollBar(PAGE_STATUS);
                this->_commit();

            }


            void _showPage_Error(){

                if(this->_initialized != true){
                    return;
                }

                this->_display.clearDisplay();

                #if MODEL_OLED_DISPLAY == ENUM_MODEL_OLED_SSD1306_128_32
                    this->_display.setTextColor(SSD1306_WHITE); // Draw white text
                    this->_display.setCursor(0, 0); 
                    this->_display.println("Error STUB");
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

                    this->_display = Adafruit_SSD1306(128, 32, &Wire, -1);

                    if(this->_display.begin(SSD1306_SWITCHCAPVCC, ADDRESS_OLED) == false){
                        if(this->ptrFailureCallback){
                            this->ptrFailureCallback(failureCode::UNABLE_TO_START);
                        }
                        return;
                    }

                    this->_display.cp437(true);
                #endif

                this->_initialized = true;
                this->_clear();
                this->_showPage_Logo();

            }


            void logEvent(const char* eventData, logLevel type){
     
                //Copy the elements down one element
                for(int i = 2; i >= 0; i--){

                    strcpy(events[i+1], events[i]);
                    
                }

                //Write the requested chars to the first element in the array
                strcpy(events[0], eventData);
                
                //Turn on the display for notification and error messages
                if(type == LOG_LEVEL_NOTIFICATION || type == LOG_LEVEL_ERROR){
                    this->_wake();
                    showPage(PAGE_EVENT_LOG);
                }

                if(type == LOG_LEVEL_NOTIFICATION){

                    //Clear the error condition
                    this->_errorText = "";

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
                if(_isSleeping == true){
                    return;
                }

                //See if it is time to turn off
                if(_isDimmed == true){
                    if((unsigned long)(millis() - _timeLastAction) > SLEEP_AFTER_MS){

                        #if DEBUG > 400
                            Serial.println("[oled] (loop) Turning off OLED");
                        #endif

                        this->_sleep();
                    }
                    return;
                }

                if((unsigned long)(millis() - _timeLastAction) > DIM_AFTER_MS){

                    #if DEBUG > 400
                        Serial.println("[oled] (loop) Dimming OLED");
                    #endif

                    this->_dim();
                }

                switch(_activePage){

                    case PAGE_EVENT_LOG_INTRO:

                        if((unsigned long)(millis() - _timeIntroShown) > INTRO_DWELL_MS){
                            showPage(PAGE_EVENT_LOG);
                        } 
                        break;


                    case PAGE_NETWORK_INTRO:

                        if((unsigned long)(millis() - _timeIntroShown) > INTRO_DWELL_MS){
                            showPage(PAGE_WIFI);
                        } 
                        break;

                    case PAGE_STATUS_INTRO:

                        if((unsigned long)(millis() - _timeIntroShown) > INTRO_DWELL_MS){
                            showPage(PAGE_STATUS);
                        } 
                        break;

                    case PAGE_HARDWARE_INTRO:

                        if((unsigned long)(millis() - _timeIntroShown) > INTRO_DWELL_MS){
                            showPage(PAGE_HARDWARE);
                        } 
                        break;

                    case PAGE_SOFTWARE_INTRO:

                        if((unsigned long)(millis() - _timeIntroShown) > INTRO_DWELL_MS){
                            showPage(PAGE_SOFTWARE);
                        } 
                        break;

                    case PAGE_ERROR_INTRO:
                        
                        if((unsigned long)(millis() - _timeIntroShown) > INTRO_DWELL_MS){
                            showPage(PAGE_ERROR);
                        } 
                        break;
                }
              
            }

            void showError(char* text){

                //Log the error text
                //logEvent(text, info);

                //Set the errorText
                _errorText = text;

                //Show the error screen
                showPage(PAGE_ERROR);
            }

            void clearError(){
                _errorText = "";
            }


            void showPage(pages requestedPage){

                if(this->_initialized != true){
                    return;
                }

                this->_wake();

                switch(requestedPage){

                    case PAGE_EVENT_LOG:
                        _activePage = PAGE_EVENT_LOG;
                        _showPage_EventLog();
                        break;

                    case PAGE_EVENT_LOG_INTRO:
                        _activePage = PAGE_EVENT_LOG_INTRO;
                        _showPage_EventLog_Intro();
                        break;
                    
                    case PAGE_WIFI:
                        _activePage = PAGE_WIFI;
                        _showPage_WiFi();
                        break;

                    case PAGE_ETHERNET:
                        _activePage = PAGE_ETHERNET;
                        _showPage_Ethernet();
                        break;

                    case PAGE_NETWORK_INTRO:
                        _activePage = PAGE_NETWORK_INTRO;
                        _showPage_Network_Intro();
                        break;

                    case PAGE_STATUS:
                        _activePage = PAGE_STATUS;
                        _showPage_Status();
                        break;

                    case PAGE_STATUS_INTRO:
                        _activePage = PAGE_STATUS_INTRO;
                        _showPage_Status_Intro();
                        break;

                    case PAGE_HARDWARE:
                        _activePage = PAGE_HARDWARE;
                        _showPage_Hardware();
                        break;

                    case PAGE_HARDWARE_INTRO:
                        _activePage = PAGE_HARDWARE_INTRO;
                        _showPage_Hardware_Intro();
                        break;

                    case PAGE_SOFTWARE:
                        _activePage = PAGE_SOFTWARE;
                        _showPage_Software();
                        break;

                    case PAGE_SOFTWARE_INTRO:
                        _activePage = PAGE_SOFTWARE_INTRO;
                        _showPage_Software_Intro();
                        break;

                    case PAGE_ERROR:
                        _activePage = PAGE_ERROR;
                        _showPage_Error();
                        break;

                    case PAGE_ERROR_INTRO:
                        _activePage = PAGE_ERROR_INTRO;
                        _showPage_Error_Intro();
                        break;

                    case PAGE_FACTORY_RESET:
                        _activePage = PAGE_FACTORY_RESET;
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
                switch(_activePage){

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
                        if(_errorText != ""){
                            showPage(PAGE_ERROR_INTRO);
                            return;
                        }
                        showPage(PAGE_EVENT_LOG_INTRO);
                        break;
                }
            }
    };

#endif