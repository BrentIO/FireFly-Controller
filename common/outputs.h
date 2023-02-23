#include "hardware.h"

class managerOutputs{

    enum outputState{
        STATE_HIGH = HIGH,
        STATE_LOW = LOW
    };


    enum outputType{
        BINARY = 0,
        VARIABLE = 1
    };


    struct outputPin{
        outputState state = STATE_LOW;
        outputType type = BINARY;
    };


    struct outputController{
        uint8_t address = 0; /* I2C address. Default 0.*/
        outputPin outputs[COUNT_PINS_OUTPUT_CONTROLLER];
        bool enabled = true; /* Indicates if the controller is enabled. Default true.*/

        #if MODEL_OUTPUT_CONTROLLER == ENUM_MODEL_OUTPUT_CONTROLLER_PCA9685
            PCA9685 hardware = PCA9685(0); /* Reference to the hardware. */
        #endif

    };

    outputController outputControllers[COUNT_OUTPUT_CONTROLLER];

    void (*ptrPublisherCallback)(void); //TODO: Determine correct signature
    void (*ptrFailureCallback)(void); //TODO: Determine correct signature


    public:

        void setCallback_publisher(void (*userDefinedCallback)(void)) {
            ptrPublisherCallback = userDefinedCallback; }

        void setCallback_failure(void (*userDefinedCallback)(void)) {
            ptrFailureCallback = userDefinedCallback; }

        void begin(){

            const uint8_t addressesOutputController[] = ADDRESSES_OUTPUT_CONTROLLER;

            //Setup output controllers
            for(int i = 0; i < COUNT_OUTPUT_CONTROLLER; i++){
                this->outputControllers[i].address = addressesOutputController[i];

                #if MODEL_OUTPUT_CONTROLLER == ENUM_MODEL_OUTPUT_CONTROLLER_PCA9685
                    this->outputControllers[i].hardware = PCA9685(outputControllers[i].address);
                    this->outputControllers[i].hardware.begin();
                    this->outputControllers[i].hardware.setFrequency(FREQUENCY_PWM);


                    //Ensure the controller is online
                    if(outputControllers[i].hardware.isConnected() == false){    
                        this->outputControllers[i].enabled = false;

                        if(this->ptrFailureCallback){
                            this->ptrFailureCallback();
                        }
                           
                    }

                #endif

            }

        }
};