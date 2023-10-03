#include "hardware.h"

class managerExternalEEPROM{

    public:

        struct deviceType{
            char uuid[37]; /**Hardware unique identifier*/
            char product_id[33]; //Product ID
            char key[65]; //Security key
        }data;


    private:

        #if EEPROM_EXTERNAL_MODEL == ENUM_EEPROM_EXTERNAL_MODEL_24LCXXX
            I2C_eeprom hardware = I2C_eeprom(0); /* Reference to the hardware. */
        #endif

        bool _initialized = false; /* If the class has been initialized. */
        void (*ptrFailureCallback)(void); //TODO: Define signature

        void read(){

            //Ensure the hardware is enabled
            if(this->enabled == false){
                #ifdef DEBUG
                    Serial.println(F("[externalEEPROM] (read_eeprom) External EEPROM not enabled; read_eeprom() failed."));
                #endif

                return;
            }

            #if EEPROM_EXTERNAL_MODEL == ENUM_EEPROM_EXTERNAL_MODEL_24LCXXX
                hardware.readBlock(0, (uint8_t *) &this->data, sizeof(this->data));
            #endif

            //Do a sanity check to see if the data is printable (new/unformatted EEPROM will not have printable data)
            if(!isPrintable(this->data.uuid[0]) || !isPrintable(this->data.uuid[18]) || !isPrintable(this->data.uuid[35])){
                strcpy(this->data.uuid, "");
                strcpy(this->data.key, "");
                strcpy(this->data.product_id, "");
            }
        };


    public:

        void setCallback_failure(void (*userDefinedCallback)(void)) {
            this->ptrFailureCallback = userDefinedCallback;
        }

        bool enabled = false; /* Indicates if the device is enabled. Default false.*/
        
        
        void begin(){

            if(this->_initialized == true){
                return;
            }

            #if EEPROM_EXTERNAL_MODEL == ENUM_EEPROM_EXTERNAL_MODEL_24LCXXX
                this->hardware = I2C_eeprom(EEPROM_EXTERNAL_ADDRESS,EEPROM_EXTERNAL_SIZE);
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

                pinMode(EEPROM_EXTERNAL_WRITE_PROTECT_PIN, OUTPUT);

            #endif

            this->_initialized = true;

            //Get the data from the EEPROM
            this->read();
        }


        /** Returns the External EEPROM's bus status */
        structHealth health(){

            structHealth returnValue;

            returnValue.address = EEPROM_EXTERNAL_ADDRESS;
            returnValue.enabled = this->enabled;

            return returnValue;
        }


        /**Writes data to the external EEPROM.  On success, returns true else returns false.*/
        bool write(){

            if(this->_initialized != true){
                #ifdef DEBUG
                    Serial.println(F("[externalEEPROM] (write) External EEPROM class not initialized"));
                #endif

                return false;
            }

            //Ensure the hardware is enabled
            if(this->enabled == false){
                #ifdef DEBUG
                    Serial.println(F("[externalEEPROM] (write) External EEPROM not enabled; write() failed"));
                #endif

                return false;
            }
            
            //Disable write protection
            digitalWrite(EEPROM_EXTERNAL_WRITE_PROTECT_PIN, LOW);
            delay(10);

            //Write the deviceInfo object to the external EEPROM
            int writeResponse = this->hardware.writeBlock(0, (uint8_t *) &this->data, sizeof(this->data));

            //Enable write protection
            digitalWrite(EEPROM_EXTERNAL_WRITE_PROTECT_PIN, HIGH);

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
                    Serial.println(F("[externalEEPROM] (destroy) External EEPROM class not initialized"));
                #endif

                return false;
            }

            //Ensure the hardware is enabled
            if(this->enabled == false){
                #ifdef DEBUG
                    Serial.println(F("[externalEEPROM] (destroy) External EEPROM not enabled; destroy() failed"));
                #endif

                return false;
            }

            int writeResponse = 0;

            //Disable write protection
            digitalWrite(EEPROM_EXTERNAL_WRITE_PROTECT_PIN, LOW);
            delay(10);

            for (uint32_t address = 0; address < EEPROM_EXTERNAL_SIZE; address += 128)
            {
                writeResponse += this->hardware.setBlock(address, 0xff, 128);
            }

            //Enable write protection
            digitalWrite(EEPROM_EXTERNAL_WRITE_PROTECT_PIN, HIGH);

            if(writeResponse == 0){
                strcpy(this->data.uuid, "");
                strcpy(this->data.key, "");
                strcpy(this->data.product_id, "");

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