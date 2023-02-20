#include "hardware.h"

class managerExternalEEPROM{

    public:

        struct deviceType{
            char uuid[37]; /**Hardware unique identifier*/
            char product_id[33]; //Product ID
            char key[65]; //Security key
        }data;


    private:

        #if MODEL_EEPROM_EXTERNAL == ENUM_MODEL_EEPROM_EXTERNAL_24LCXXX
            I2C_eeprom hardware = I2C_eeprom(0); /* Reference to the hardware. */
        #endif

        void read_eeprom(){

            //Ensure the hardware is enabled
            if(this->enabled == false){
                #ifdef DEBUG
                    Serial.println("External EEPROM not enabled; read_eeprom() failed.");
                #endif

                return;
            }

            #if MODEL_EEPROM_EXTERNAL == ENUM_MODEL_EEPROM_EXTERNAL_24LCXXX
                hardware.readBlock(0, (uint8_t *) &this->data, sizeof(this->data));
            #endif

        };


        void (*ptrFailureCallback)(void); //TODO: Define signature


    public:

        void setCallback_failure(void (*userDefinedCallback)(void)) {
            this->ptrFailureCallback = userDefinedCallback; }


        bool enabled = true; /* Indicates if the device is enabled. Default true.*/

        
        void begin(){

            #if MODEL_EEPROM_EXTERNAL == ENUM_MODEL_EEPROM_EXTERNAL_24LCXXX
                this->hardware = I2C_eeprom(ADDRESS_EEPROM,SIZE_EEPROM);
                this->hardware.begin();

                //Ensure the hardware is online
                if(this->hardware.isConnected() == false){    
                    this->enabled = false;
                    this->ptrFailureCallback();   
                }

                pinMode(PIN_EEPROM_WP, OUTPUT);

            #endif

            //Get the data from the EEPROM
            this->read_eeprom();
        }

        /**Writes data to the external EEPROM.  On success, returns true else returns false.*/
        bool write(){

            //Ensure the hardware is enabled
            if(this->enabled == false){
                #ifdef DEBUG
                    Serial.println("External EEPROM not enabled; write() failed.");
                #endif

                return false;
            }

            #ifdef DEBUG
                Serial.println("Ready to write EEPROM");
            #endif
            
            //Disable write protection
            digitalWrite(PIN_EEPROM_WP, LOW);
            delay(10);

            //Write the deviceInfo object to the external EEPROM
            int writeResponse = this->hardware.writeBlock(0, (uint8_t *) &this->data, sizeof(this->data));

            //Enable write protection
            digitalWrite(PIN_EEPROM_WP, HIGH);

            if(writeResponse == 0){

                #ifdef DEBUG
                    Serial.println("EEPROM write success");
                #endif

                return true;

            }else{

                #ifdef DEBUG
                    Serial.println("EEPROM write failed.  writeResponse returned was " + String(writeResponse));
                #endif

                //Disable the hardware
                this->enabled = false;

                return false;
            }
        };

        /**Destroys (deletes) data on the external EEPROM.  On success, returns true else returns false.*/
        bool destroy(){

            //Ensure the hardware is enabled
            if(this->enabled == false){
                #ifdef DEBUG
                    Serial.println("External EEPROM not enabled; destroy() failed.");
                #endif

                return false;
            }

            int writeResponse = 0;

            //Disable write protection
            digitalWrite(PIN_EEPROM_WP, LOW);
            delay(10);

            #ifdef DEBUG
                Serial.println("Ready to destroy EEPROM");
            #endif

            for (uint32_t address = 0; address < SIZE_EEPROM; address += 128)
            {
                writeResponse += this->hardware.setBlock(address, 0xff, 128);
            }

            //Enable write protection
            digitalWrite(PIN_EEPROM_WP, HIGH);

            if(writeResponse == 0){
                #ifdef DEBUG
                    Serial.println("EEPROM destroyed success");
                #endif

                deviceType empty;
                data = empty;

                return true;
            }

            #ifdef DEBUG
                Serial.println("EEPROM destroy failed.  writeResponse was " + String(writeResponse));
            #endif

            //Disable the hardware
            this->enabled = false;
            
            return false;
        };
    
};