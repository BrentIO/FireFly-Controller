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


    struct temperatureSensor{
        uint8_t address = 0; /* I2C address. Default 0.*/

        #if MODEL_TEMPERATURE_SENSOR == ENUM_MODEL_TEMPERATURE_SENSOR_PCT2075
            PCT2075 hardware = PCT2075(0); /* Reference to the hardware. */
        #endif

        float previousRead = 0; /* Previous read temperature. Default 0. */
        unsigned long timePreviousRead = 0; /* Time (millis) when the sensor was last read. Default 0.*/
        bool enabled = false; /* Indicates if the sensor is enabled. Default false.*/
    };
            char* location; /* Physical location of the sensor */

    bool _initialized = false; /* If the class has been initialized. */





    void (*ptrPublisherCallback)(String, float);
    void (*ptrFailureCallback)(String);

    public:

        struct healthResult{
            uint8_t count = 0;
            structHealth sensor[COUNT_TEMPERATURE_SENSOR];
        };

        void setCallback_publisher(void (*userDefinedCallback)(String, float)) {
                    ptrPublisherCallback = userDefinedCallback; }

        void setCallback_failure(void (*userDefinedCallback)(String)) {
                    ptrFailureCallback = userDefinedCallback; }
        

        void begin(){
            
            if(this->_initialized == true){
                return;
            }

            if(COUNT_TEMPERATURE_SENSOR == 0){
                return;
            }

            const uint8_t addressesTemperatureSensor[] = ADDRESSES_TEMPERATURE_SENSOR;

            if(COUNT_TEMPERATURE_SENSOR != sizeof(addressesTemperatureSensor)/sizeof(uint8_t)){

                #if DEBUG
                    Serial.println(F("[temperature] (begin) COUNT_TEMPERATURE_SENSOR and the length of ADDRESSES_TEMPERATURE_SENSOR are mismatched in hardware.h; Disabling temperature sensors."));
                #endif

                if(this->ptrFailureCallback){
                    this->ptrFailureCallback(locationToString(UNKNOWN));
                }

                return;

            }

            for(int i = 0; i < COUNT_TEMPERATURE_SENSOR; i++){

                this->temperatureSensors[i].address = addressesTemperatureSensor[i];
                this->temperatureSensors[i].location = locations[i];

                #if MODEL_TEMPERATURE_SENSOR == ENUM_MODEL_TEMPERATURE_SENSOR_PCT2075
                    this->temperatureSensors[i].hardware = PCT2075(this->temperatureSensors[i].address);

                    if(this->temperatureSensors[i].hardware.getConfig() !=0){
                        temperatureSensors[i].enabled = false;

                        if(this->ptrFailureCallback){
                            this->ptrFailureCallback(temperatureSensors[i].location);
                        }

                        continue;
                    }

                    temperatureSensors[i].enabled = true;

                #endif
            }

            this->_initialized = true;
        };


        /** Returns the value of each temperature sensor's bus status */
        healthResult health(){

            healthResult returnValue;

            if(this->_initialized != true){
                return returnValue;
            }

            for(int i = 0; i < COUNT_TEMPERATURE_SENSOR; i++){
                returnValue.sensor[i].address = this->temperatureSensors[i].address;
                returnValue.sensor[i].enabled = this->temperatureSensors[i].enabled;
            }

            returnValue.count = COUNT_TEMPERATURE_SENSOR;

            return returnValue;
        }


        void loop(){

            if(this->_initialized != true){
                return;
            }

            //Loop through each temperature sensor on the board
            for(int i = 0; i < COUNT_TEMPERATURE_SENSOR; i++){

                if(temperatureSensors[i].enabled == false){
                    continue;
                }

                //If the timer has expired OR if the temperature sensor has never been read, read it
                if((millis() - temperatureSensors[i].timePreviousRead > MILLS_TEMPERATURE_SLEEP_DURATION) || (temperatureSensors[i].timePreviousRead == 0)){

                    //Temperatures will be reported in degrees C
                    #if MODEL_TEMPERATURE_SENSOR == ENUM_MODEL_TEMPERATURE_SENSOR_PCT2075
                        float currentRead = temperatureSensors[i].hardware.getTempC();
                    #endif

                    //Set the new read time time
                    temperatureSensors[i].timePreviousRead = millis();

                    //Ensure the hardware is still online
                    #if MODEL_TEMPERATURE_SENSOR == ENUM_MODEL_TEMPERATURE_SENSOR_PCT2075
                        if(this->temperatureSensors[i].hardware.getConfig() !=0){
                            temperatureSensors[i].enabled = false;
                            this->ptrFailureCallback(temperatureSensors[i].location);
                        }
                    #endif
                    
                    //Check if the delta between the two reads is more than the DEGREES_TEMPERATURE_VARIATION_ALLOWED
                    if(abs(currentRead - temperatureSensors[i].previousRead) > DEGREES_TEMPERATURE_VARIATION_ALLOWED){

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