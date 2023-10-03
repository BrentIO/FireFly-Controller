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
 *  One callback function is supported:
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

        /** Enumeration of the output type, which is either binary or variable (PWM) */
        enum outputType{
            BINARY = 0,
            VARIABLE = 1
        };


        struct outputPin{
            uint8_t state = 0; //The state of the output as a percentage from 0 to 100
            outputType type = BINARY; //Default all outputs are binary for safety
            uint8_t port = 0; /* Human-readable port number */
        };


        struct outputController{
            uint8_t address = 0; /* I2C address. Default 0.*/
            outputPin outputs[OUTPUT_CONTROLLER_COUNT_PINS];
            bool enabled = true; /* Indicates if the controller is enabled. Default true */

            #if OUTPUT_CONTROLLER_MODEL == ENUM_OUTPUT_CONTROLLER_MODEL_PCA9685
                PCA9685 hardware = PCA9685(0); /* Reference to the hardware. */
            #endif

        };

        outputController outputControllers[OUTPUT_CONTROLLER_COUNT];


        bool _initialized = false; /* If the class has been initialized. */


        /** Reference to the callback function that will be called when an output controller has failed */
        void (*ptrFailureCallback)(uint8_t, failureReason);


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

        struct healthResult{
            uint8_t count = 0;
            structHealth outputControllers[OUTPUT_CONTROLLER_COUNT];
        };


        /** Callback function that is called when an output controller failure occurs */
        void setCallback_failure(void (*userDefinedCallback)(uint8_t, failureReason)) {
            ptrFailureCallback = userDefinedCallback; }


        void begin(){

            if(this->_initialized == true){
                return;
            }

            const uint8_t addressesOutputController[] = OUTPUT_CONTROLLER_ADDRESSES;

            if(OUTPUT_CONTROLLER_COUNT != sizeof(addressesOutputController)/sizeof(uint8_t)){

                #if DEBUG
                    Serial.println(F("[outputs] (begin) OUTPUT_CONTROLLER_ADDRESSES and the length of OUTPUT_CONTROLLER_ADDRESSES are mismatched in hardware.h; Disabling outputs."));
                #endif

                outputController invalid;
                invalid.address = 0;

                failOutputController(&invalid, failureReason::INVALID_HARDWARE_CONFIGURATION);

                return;
            }

            uint8_t portPinMap[OUTPUT_CONTROLLER_COUNT_PINS * OUTPUT_CONTROLLER_COUNT] = OUTPUT_CONTROLLER_PORTS;

            for(int i = 0; i < OUTPUT_CONTROLLER_COUNT; i++){
                this->outputControllers[i].address = addressesOutputController[i];
                
                for(int j = 0; j < OUTPUT_CONTROLLER_COUNT_PINS; j++){

                    if(portPinMap[j] == 0){

                         #if DEBUG
                            Serial.println(F("[outputs] (begin) OUTPUT_CONTROLLER_COUNT_PINS and the length of OUTPUT_CONTROLLER_PORTS are mismatched in hardware.h; Disabling outputs."));
                        #endif

                        failOutputController(&this->outputControllers[i], failureReason::INVALID_HARDWARE_CONFIGURATION);

                        return;
                    }

                    this->outputControllers[i].outputs[j].port = portPinMap[j];
                }

                #if OUTPUT_CONTROLLER_MODEL == ENUM_OUTPUT_CONTROLLER_MODEL_PCA9685
                    this->outputControllers[i].hardware = PCA9685(outputControllers[i].address);

                    if(this->outputControllers[i].hardware.begin() == false){
                        failOutputController(&this->outputControllers[i], i2cResponseToFailureReason(outputControllers[i].hardware.lastError()));
                    };

                    this->outputControllers[i].hardware.setFrequency(OUTPUT_CONTROLLER_FREQUENCY_PWM);
                    
                    if(outputControllers[i].hardware.lastError() != 0){
                        failOutputController(&this->outputControllers[i], i2cResponseToFailureReason(outputControllers[i].hardware.lastError()));
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

            for(int i = 0; i < OUTPUT_CONTROLLER_COUNT; i++){
                returnValue.outputControllers[i].address = this->outputControllers[i].address;
                returnValue.outputControllers[i].enabled = this->outputControllers[i].enabled;
            }

            returnValue.count = OUTPUT_CONTROLLER_COUNT;

            return returnValue;
        }
};