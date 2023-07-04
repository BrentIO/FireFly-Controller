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
                    Serial.println(F("[externalEEPROM] (read_eeprom) External EEPROM not enabled; read_eeprom() failed."));
                #endif

                return;
            }

            #if MODEL_EEPROM_EXTERNAL == ENUM_MODEL_EEPROM_EXTERNAL_24LCXXX
                hardware.readBlock(0, (uint8_t *) &this->data, sizeof(this->data));
            #endif

        };

        bool _initialized = false; /* If the class has been initialized. */
        void (*ptrFailureCallback)(void); //TODO: Define signature


    public:

        void setCallback_failure(void (*userDefinedCallback)(void)) {
            this->ptrFailureCallback = userDefinedCallback;
        }

        bool enabled = false; /* Indicates if the device is enabled. Default false.*/

        
        void begin(){

            if(this->_initialized == true){
                return;
            }

            #if MODEL_EEPROM_EXTERNAL == ENUM_MODEL_EEPROM_EXTERNAL_24LCXXX
                this->hardware = I2C_eeprom(ADDRESS_EEPROM,SIZE_EEPROM);
                this->hardware.begin();

                //Ensure the hardware is online
                if(this->hardware.isConnected() == false){

                    #ifdef DEBUG
                        Serial.println(F("[externalEEPROM] (begin) External EEPROM not connected"));
                    #endif

                    this->enabled = false;

                    if(this->ptrFailureCallback){
                        this->ptrFailureCallback();
                    }

                    return;
                }

                this->enabled = true;

                pinMode(PIN_EEPROM_WP, OUTPUT);

            #endif

            this->_initialized = true;

            //Get the data from the EEPROM
            this->read_eeprom();
        }


        /** Returns the External EEPROM's bus status */
        structHealth health(){

            structHealth returnValue;

            returnValue.address = ADDRESS_EEPROM;
            returnValue.enabled = this->enabled;

            return returnValue;
        }


        /**Writes data to the external EEPROM.  On success, returns true else returns false.*/
        bool write(){

            if(this->_initialized != true){
                #ifdef DEBUG
                    Serial.println(F("[externalEEPROM] (write) External EEPROM class not initialized."));
                #endif

                return false;
            }

            //Ensure the hardware is enabled
            if(this->enabled == false){
                #ifdef DEBUG
                    Serial.println(F("[externalEEPROM] (write) External EEPROM not enabled; write() failed."));
                #endif

                return false;
            }
            
            //Disable write protection
            digitalWrite(PIN_EEPROM_WP, LOW);
            delay(10);

            //Write the deviceInfo object to the external EEPROM
            int writeResponse = this->hardware.writeBlock(0, (uint8_t *) &this->data, sizeof(this->data));

            //Enable write protection
            digitalWrite(PIN_EEPROM_WP, HIGH);

            if(writeResponse == 0){
                return true;

            }else{

                #ifdef DEBUG
                    Serial.println("[externalEEPROM] (write) EEPROM write failed.  writeResponse returned was " + String(writeResponse));
                #endif

                //Disable the hardware
                this->enabled = false;

                if(this->ptrFailureCallback){
                    this->ptrFailureCallback();
                }

                return false;
            }
        };

        /**Destroys (deletes) data on the external EEPROM.  On success, returns true else returns false.*/
        bool destroy(){

            if(this->_initialized != true){
                #ifdef DEBUG
                    Serial.println(F("[externalEEPROM] (destroy) External EEPROM class not initialized."));
                #endif

                return false;
            }

            //Ensure the hardware is enabled
            if(this->enabled == false){
                #ifdef DEBUG
                    Serial.println(F("[externalEEPROM] (destroy) External EEPROM not enabled; destroy() failed."));
                #endif

                return false;
            }

            int writeResponse = 0;

            //Disable write protection
            digitalWrite(PIN_EEPROM_WP, LOW);
            delay(10);

            for (uint32_t address = 0; address < SIZE_EEPROM; address += 128)
            {
                writeResponse += this->hardware.setBlock(address, 0xff, 128);
            }

            //Enable write protection
            digitalWrite(PIN_EEPROM_WP, HIGH);

            if(writeResponse == 0){
                deviceType empty;
                data = empty;

                return true;
            }

            #ifdef DEBUG
                Serial.println("EEPROM destroy failed.  writeResponse was " + String(writeResponse));
            #endif

            //Disable the hardware
            this->enabled = false;
            
            if(this->ptrFailureCallback){
                this->ptrFailureCallback();
            }
            
            return false;
        };
    
};