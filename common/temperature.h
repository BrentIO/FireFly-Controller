#include "hardware.h"

/** Temperature sensor manager
 * 
 * Monitors all temperature sensors in the system.
 * 
 * ### Usage * 
 * Minimum usage includes `begin()`, which should be placed in the main `setup()` function, and `loop()`, which should be placed in the main `loop()` function.
 * 
 * ### Callbacks
 *  Two callback functions are supported:
 * - `setCallback_publisher` which will be called when a sensor has changed temperature more than the programmed threshold since the last published temperature
 * - `setCallback_failure` which will be called if there is an error during `begin()` or if one of the sensors falls off the bus after being initialized
*/
class managerTemperatureSensors{

    public:

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


        /** Represents an individual temperature sensor */
        struct temperatureSensor{
            uint8_t address = 0; /* I2C address. Default 0.*/

            #if TEMPERATURE_SENSOR_MODEL == ENUM_TEMPERATURE_SENSOR_MODEL_PCT2075
                PCT2075 hardware = PCT2075(address); /* Reference to the hardware. */
            #endif

            float previousRead = 0; /* Previous read temperature in degrees celsius. Default 0 */
            unsigned long timePreviousRead = 0; /* Time (millis) when the sensor was last read. Default 0 */
            char* location; /* Physical location of the sensor */
            bool enabled = true; /* Indicates if the sensor is enabled. Default true */
        };


        temperatureSensor temperatureSensors[TEMPERATURE_SENSOR_COUNT];


        bool _initialized = false; /* If the class has been initialized. */


        /** Reference to the callback function that will be called when a temperature change is sensed */
        void (*ptrPublisherCallback)(char*, float);


        /** Reference to the callback function that will be called when a temperature sensor has failed */
        void (*ptrFailureCallback)(char*, failureReason);


        /** Marks a given temperature sensor as failed and raises a callback event, if configured 
         * @param temperatureSensor The temperature sensor being reported
         * @param failureReason Reason code for the failure
        */
        void failTemperatureSensor(temperatureSensor *temperatureSensor, failureReason failureReason){

            if(temperatureSensor->enabled == false){
                return;
            }

            if(failureReason == failureReason::SUCCESS_NO_ERROR){
                #if DEBUG
                    Serial.println(F("[temperature] (failTemperatureSensor) Function is being called with no error present."));
                #endif

                return;
            }

            temperatureSensor->enabled = false;

            if(this->ptrFailureCallback){
                this->ptrFailureCallback(temperatureSensor->location, failureReason);
            }
        }

    public:

        /** Object containing the count of temperature sensors and a list of each sensor's bus status */
        struct healthResult{
            uint8_t count = 0; /** The number of temperature sensors */
            structHealth sensor[TEMPERATURE_SENSOR_COUNT]; /** Array of temperature snsor health*/
        };


        /** Get the health of the temperature sensors 
         * @returns All temperature sensors and their bus state; the count of temperature sensors
        */
        healthResult health(){

            healthResult returnValue;

            if(this->_initialized != true){
                return returnValue;
            }

            for(int i = 0; i < TEMPERATURE_SENSOR_COUNT; i++){
                returnValue.sensor[i].address = this->temperatureSensors[i].address;
                returnValue.sensor[i].enabled = this->temperatureSensors[i].enabled;
            }

            returnValue.count = TEMPERATURE_SENSOR_COUNT;

            return returnValue;
        }


        /** Callback function that is called when a temperature change is observed */
        void setCallback_publisher(void (*userDefinedCallback)(char*, float)) {
                    ptrPublisherCallback = userDefinedCallback; }


        /** Callback function that is called when a temperature sensor failure occurs */
        void setCallback_failure(void (*userDefinedCallback)(char*, failureReason)) {
                    ptrFailureCallback = userDefinedCallback; }
        

        /** Initializes all temperature sensors. If unsuccessful, the failure callback will be called */
        void begin(){
            
            if(this->_initialized == true){
                return;
            }

            if(TEMPERATURE_SENSOR_COUNT == 0){
                return;
            }

            const uint8_t addressesTemperatureSensor[] = TEMPERATURE_SENSOR_ADDRESSES;

            if(TEMPERATURE_SENSOR_COUNT != sizeof(addressesTemperatureSensor)/sizeof(uint8_t)){

                #if DEBUG
                    Serial.println(F("[temperature] (begin) TEMPERATURE_SENSOR_COUNT and the length of TEMPERATURE_SENSOR_ADDRESSES are mismatched in hardware.h; Disabling temperature sensors."));
                #endif

                temperatureSensor invalid;
                strcpy(invalid.location, "INVALID");

                failTemperatureSensor(&invalid, failureReason::INVALID_HARDWARE_CONFIGURATION);
                return;

            }

            char* locations[TEMPERATURE_SENSOR_COUNT] = TEMPERATURE_SENSOR_LOCATIONS;

            for(int i = 0; i < TEMPERATURE_SENSOR_COUNT; i++){

                this->temperatureSensors[i].address = addressesTemperatureSensor[i];
                this->temperatureSensors[i].location = locations[i];
                temperatureSensors[i].enabled = true;

                #if TEMPERATURE_SENSOR_MODEL == ENUM_TEMPERATURE_SENSOR_MODEL_PCT2075
                    this->temperatureSensors[i].hardware = PCT2075(this->temperatureSensors[i].address);

                    //Attempt to get the current temperature to ensure the device is on the bus
                    temperatureSensors[i].hardware.getTempC();

                    if(this->temperatureSensors[i].hardware.i2c_error() !=0){
                        failTemperatureSensor(&this->temperatureSensors[i], i2cResponseToFailureReason(this->temperatureSensors[i].hardware.i2c_error()));
                        continue;
                    }

                #endif
            }

            this->_initialized = true;
        };


        /** Observes changes in temperature with each main loop() cycle based on a defined temperature delta and sleep period */
        void loop(){

            if(this->_initialized != true){
                return;
            }

            //Loop through each temperature sensor on the board
            for(int i = 0; i < TEMPERATURE_SENSOR_COUNT; i++){

                if(temperatureSensors[i].enabled == false){
                    continue;
                }

                //If the timer has expired OR if the temperature sensor has never been read, read it
                if((millis() - temperatureSensors[i].timePreviousRead > TEMPERATURE_SENSOR_SLEEP_DURATION_MILLIS) || (temperatureSensors[i].timePreviousRead == 0)){

                    //Temperatures will be reported in degrees C
                    #if TEMPERATURE_SENSOR_MODEL == ENUM_TEMPERATURE_SENSOR_MODEL_PCT2075
                        float currentRead = temperatureSensors[i].hardware.getTempC();
                    #endif

                    //Set the new read time time
                    temperatureSensors[i].timePreviousRead = millis();

                    //Ensure the hardware is still online
                    #if TEMPERATURE_SENSOR_MODEL == ENUM_TEMPERATURE_SENSOR_MODEL_PCT2075
                        if(temperatureSensors[i].hardware.i2c_error() !=0){

                            failTemperatureSensor(&temperatureSensors[i], i2cResponseToFailureReason(temperatureSensors[i].hardware.i2c_error()));
                            return;
                        }
                    #endif
                    
                    //Check if the delta between the two reads is more than the TEMPERATURE_SENSOR_DEGREES_VARIATION_ALLOWED
                    if(abs(currentRead - temperatureSensors[i].previousRead) > TEMPERATURE_SENSOR_DEGREES_VARIATION_ALLOWED){

                        //Store the new temperature reading
                        temperatureSensors[i].previousRead = currentRead;

                        //Publish an event for the change
                        if(this->ptrPublisherCallback){
                            this->ptrPublisherCallback(temperatureSensors[i].location, temperatureSensors[i].previousRead);
                        }
                    }      
                }
            }
        };
};