#include "hardware.h"

class managerTemperatureSensors{

    enum temperatureSensorLocation{
        UNKNOWN = -1,
        CENTER = 0
    };

    struct temperatureSensor{
        uint8_t address = 0; /* I2C address. Default 0.*/

        #if MODEL_TEMPERATURE_SENSOR == ENUM_MODEL_TEMPERATURE_SENSOR_PCT2075
            PCT2075 hardware = PCT2075(0); /* Reference to the hardware. */
        #endif

        float previousRead = 0; /* Previous read temperature. Default 0. */
        unsigned long timePreviousRead = 0; /* Time (millis) when the sensor was last read. Default 0.*/
        temperatureSensorLocation location;
        bool enabled = true; /* Indicates if the sensor is enabled. Default true.*/
    };

    temperatureSensor temperatureSensors[COUNT_TEMPERATURE_SENSOR];

    /** Convert a temperatureSensorLocation to string */
    String locationToString(temperatureSensorLocation value){

        switch(value){

            case temperatureSensorLocation::CENTER:
            return F("CENTER");
            break;

            default:
            return F("UNKNOWN");
            break;
        }
    };

    void (*ptrPublisherCallback)(String, float);
    void (*ptrFailureCallback)(String);

    public:

        void setCallback_publisher(void (*userDefinedCallback)(String, float)) {
                    this->ptrPublisherCallback = userDefinedCallback; }

        void setCallback_failure(void (*userDefinedCallback)(String)) {
                    this->ptrFailureCallback = userDefinedCallback; }
        

        void begin(){

            const uint8_t addressesTemperatureSensor[] = ADDRESSES_TEMPERATURE_SENSOR;

            for(int i = 0; i < COUNT_TEMPERATURE_SENSOR; i++){

                this->temperatureSensors[i].address = addressesTemperatureSensor[i];

                #if MODEL_TEMPERATURE_SENSOR == ENUM_MODEL_TEMPERATURE_SENSOR_PCT2075
                    this->temperatureSensors[i].hardware = PCT2075(this->temperatureSensors[i].address);

                    if(this->temperatureSensors[i].hardware.getConfig() !=0){
                        temperatureSensors[i].enabled = false;

                        if(this->ptrFailureCallback){
                            this->ptrFailureCallback(locationToString(temperatureSensors[i].location));
                        }
                    }

                #endif

                #if PRODUCT_ID == 32322211

                    switch(i){
                        case 0:
                            temperatureSensors[i].location = temperatureSensorLocation::CENTER;
                            break;
                        default:
                            temperatureSensors[i].location = temperatureSensorLocation::UNKNOWN;
                            break;
                    }
                
                #endif
            }
        };


        void loop(){

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
                            this->ptrFailureCallback(locationToString(temperatureSensors[i].location));
                        }
                    #endif
                    
                    //Check if the delta between the two reads is more than the DEGREES_TEMPERATURE_VARIATION_ALLOWED
                    if(abs(currentRead - temperatureSensors[i].previousRead) > DEGREES_TEMPERATURE_VARIATION_ALLOWED){

                        //Store the new temperature reading
                        temperatureSensors[i].previousRead = currentRead;

                        //Publish an event for the change
                        if(this->ptrPublisherCallback){
                            this->ptrPublisherCallback(locationToString(temperatureSensors[i].location), temperatureSensors[i].previousRead);
                        }
                    }      
                }
            }

        };

};