#include "hardware.h"


/** Output Manager
 * 
 * Configures outputs on the system using the LED output controllers.
 * 
 * ### Usage
 *  Minimum usage includes `begin()`, which should be placed in the main `setup()` function, and `loop()`, which should be placed in the main `loop()` function.
 * 
 * 
 * ### Callbacks
 *  Two callback functions are supported:
 * - `setCallback_publisher` which will be called when an input changes from its normal status to an abnormal status
 * - `setCallback_failure` which will be called if there is an error during `begin()` or if one of the input controllers falls off the bus after being initialized
 */
class managerOutputs{

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

        enum outputState{
            STATE_HIGH = HIGH,
            STATE_LOW = LOW
        };


        /** Enumeration of the output type, which is either binary or variable (PWM) */
        enum outputType{
            BINARY = 0,
            VARIABLE = 1
        };


        struct outputPin{
            outputState state = STATE_LOW; //Output state will be low on start
            outputType type = BINARY; //Default all outputs are binary for safety
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


        bool _initialized = false;


        void (*ptrPublisherCallback)(void); //TODO: Determine correct signature

        
        void (*ptrFailureCallback)(void); //TODO: Determine correct signature


        /** Marks a given output controller as failed and raises a callback event, if configured 
         * @param outputController The input controller being reported
         * @param failureReason Reason code for the failure
        */
        void failOutputController(outputController *outputController, failureReason failureReason){

            if(outputController->enabled == false){
                return;
            }

            if(failureReason == failureReason::SUCCESS_NO_ERROR){
                #if DEBUG
                    Serial.println(F("[outputs] (failOutputController) Function is being called with no error present."));
                #endif

                return;
            }

            outputController->enabled = false;

            if(this->ptrFailureCallback){
                this->ptrFailureCallback(outputController->address, failureReason);
            }
        }


    public:

        struct healthResult{
            uint8_t count = 0;
            structHealth outputControllers[COUNT_OUTPUT_CONTROLLER];
        };


        void setCallback_publisher(void (*userDefinedCallback)(void)) {
            ptrPublisherCallback = userDefinedCallback; }


        void setCallback_failure(void (*userDefinedCallback)(void)) {
            ptrFailureCallback = userDefinedCallback; }


        void begin(){

            if(this->_initialized == true){
                return;
            }

            const uint8_t addressesOutputController[] = ADDRESSES_OUTPUT_CONTROLLER;

            if(COUNT_OUTPUT_CONTROLLER != sizeof(addressesOutputController)/sizeof(uint8_t)){

                #if DEBUG
                    Serial.println(F("[outputs] (begin) COUNT_OUTPUT_CONTROLLER and the length of ADDRESSES_OUTPUT_CONTROLLER are mismatched in hardware.h; Disabling outputs."));
                #endif

                if(this->ptrFailureCallback){
                    this->ptrFailureCallback();
                }

                return;

            }

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

            this->_initialized = true;

        }

        /** Returns the value of each output controller's bus status */
        healthResult health(){

            healthResult returnValue;

            if(this->_initialized != true){
                return returnValue;
            }

            for(int i = 0; i < COUNT_OUTPUT_CONTROLLER; i++){
                returnValue.outputControllers[i].address = this->outputControllers[i].address;
                returnValue.outputControllers[i].enabled = this->outputControllers[i].enabled;
            }

            returnValue.count = COUNT_OUTPUT_CONTROLLER;

            return returnValue;
        }
};