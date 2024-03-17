#include "hardware.h"

class managerExternalEEPROM{

    public:

        struct deviceType{
            char uuid[37]; /**Hardware unique identifier*/
            char product_id[33]; //Product ID
            char key[65]; //Security key
        }data;

        /** Failure reason codes, roughly based on i2c return messages */
        enum failureReason{
            /// @brief Request was successful, no error was returned
            SUCCESS_NO_ERROR = 0, 

            /// @brief Data too long to fit in transmit buffer
            DATA_TRANSMIT_BUFFER_ERROR = 1,

            /// @brief Received NACK on transmit of address
            ADDRESS_OFFLINE = 2,

            /// @brief Received NACK on transmit of data
            TRANSMIT_NOT_ACKNOLWEDGED = 3,

            /// @brief Other error
            OTHER_ERROR = 4,

            /// @brief Timeout
            TIMEOUT = 5,

            /// @brief Invalid hardware configuration
            INVALID_HARDWARE_CONFIGURATION = 10,

            /// @brief Unknown/undocumented failure
            UNKNOWN_ERROR = 11
        };


    private:

        #if EEPROM_EXTERNAL_MODEL == ENUM_EEPROM_EXTERNAL_MODEL_24LCXXX
            I2C_eeprom hardware = I2C_eeprom(0); /* Reference to the hardware. */
        #endif

        bool _initialized = false; /* If the class has been initialized. */
        uint8_t _address = EEPROM_EXTERNAL_ADDRESS;
        void (*ptrFailureCallback)(uint8_t, failureReason);

        void read(){

            //Ensure the hardware is enabled
            if(this->enabled == false){

                log_e("External EEPROM not enabled; read_eeprom() failed");
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

        /** Enumerates the i2c bus failure codes to a failureReason 
         * @param i2cError The value returned from the i2c wire endTransmission() function
         * @returns A failureReason enumeration mapped to the error code passed in
        */
        failureReason i2cResponseToFailureReason(uint8_t i2cError){

            switch(i2cError){

                case 0:
                    return failureReason::SUCCESS_NO_ERROR;
                    break;

                case 1:
                    return failureReason::DATA_TRANSMIT_BUFFER_ERROR;
                    break;

                case 2:
                    return failureReason::ADDRESS_OFFLINE;
                    break;

                case 3:
                    return failureReason::TRANSMIT_NOT_ACKNOLWEDGED;
                    break;

                case 4:
                    return failureReason::OTHER_ERROR;
                    break;

                case 5:
                    return failureReason::TIMEOUT;
                    break;

                case 10:
                    return failureReason::INVALID_HARDWARE_CONFIGURATION;
                    break;

                default:
                    return failureReason::UNKNOWN_ERROR;
            }
        }


    public:

        void setCallback_failure(void (*userDefinedCallback)(uint8_t, failureReason)) {
            this->ptrFailureCallback = userDefinedCallback;
        }

        bool enabled = false; /* Indicates if the device is enabled. Default false.*/
        
        
        void begin(){

            if(this->_initialized == true){
                return;
            }

            #if EEPROM_EXTERNAL_MODEL == ENUM_EEPROM_EXTERNAL_MODEL_24LCXXX
                this->hardware = I2C_eeprom(this->_address,EEPROM_EXTERNAL_SIZE);
                this->hardware.begin();

                //Ensure the hardware is online
                if(this->hardware.isConnected() == false){

                    this->enabled = false;

                    if(this->ptrFailureCallback){
                        this->ptrFailureCallback(this->_address,managerExternalEEPROM::failureReason::ADDRESS_OFFLINE);
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

            returnValue.address = this->_address;
            returnValue.enabled = this->enabled;

            return returnValue;
        }


        /**Writes data to the external EEPROM.  On success, returns true else returns false.*/
        bool write(){

            if(this->_initialized != true){

                log_e("External EEPROM class not initialized");
                return false;
            }

            //Ensure the hardware is enabled
            if(this->enabled == false){

                log_e("External EEPROM not enabled; write() failed");
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

                log_e("EEPROM write failed.  writeResponse returned was %i", writeResponse);

                //Disable the hardware
                this->enabled = false;

                if(this->ptrFailureCallback){
                    this->ptrFailureCallback(this->_address, i2cResponseToFailureReason(writeResponse));
                }

                return false;
            }
        };


        /**Destroys (deletes) data on the external EEPROM.  On success, returns true else returns false.*/
        bool destroy(){

            if(this->_initialized != true){

                log_e("External EEPROM class not initialized");
                return false;
            }

            //Ensure the hardware is enabled
            if(this->enabled == false){

                log_e("External EEPROM not enabled; destroy() failed");
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

            log_e("EEPROM destroy failed.  writeResponse was %i", writeResponse);

            //Disable the hardware
            this->enabled = false;
            
            if(this->ptrFailureCallback){
                this->ptrFailureCallback(this->_address, i2cResponseToFailureReason(writeResponse));
            }
            
            return false;
        };
};