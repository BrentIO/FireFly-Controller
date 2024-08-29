#include "hardware.h"

namespace nsOutputs{

    /** Failure reason codes, roughly based on i2c return messages */
    enum failureReason{

        // PCA9685 has specific failure codes built into the library
        #if OUTPUT_CONTROLLER_MODEL == ENUM_OUTPUT_CONTROLLER_MODEL_PCA9685

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

            /// @brief  Inherited from PCA9685 library, generic error
            GENERIC_ERROR = 0xFF,

            /// @brief  Inherited from PCA9685 library, channel out of range
            CHANNEL_OUT_OF_RANGE = 0xFE,

            /// @brief  Inherited from PCA9685 library, invalid mode register chosen
            INVALID_MODE_REGISTER = 0xFD,

            /// @brief  Inherited from PCA9685 library, i2c communication error
            GENERIC_I2C_ERROR = 0xFC,

            /// @brief Unknown/undocumented failure
            UNKNOWN_ERROR = 11
        #endif
    };


    /** Result of setting the port/pin's value */
    enum set_result{

        /// @brief the set request was successful
        SUCCESS = 0,

        /// @brief the set request failed
        FAILED = 1,

        /// @brief request included an invalid output port number
        INVALID_PORT = 10,

        /// @brief the set request will not be fulfilled because the set value and the requested value were the same
        EXCESSIVE = 20,

        /// @brief the controller handling the request is not enabled and request will not be fulfilled
        CONTROLLER_NOT_ENABLED = 30,

        /// @brief the port requested is not enabled and request will not be fulfilled
        PORT_NOT_ENABLED = 40

    };


    class outputController{

        public:

            uint8_t address = 0; /* I2C address. Default 0.*/
            bool enabled = true; /* Indicates if the controller is enabled. Default true */
            void (*failureCallback)(uint8_t, failureReason);

            #if OUTPUT_CONTROLLER_MODEL == ENUM_OUTPUT_CONTROLLER_MODEL_PCA9685
                PCA9685 hardware = PCA9685(0); /* Reference to the hardware. */
            #endif


            /** Marks a given output controller as failed and raises a callback event, if configured 
             * @param reason Reason code for the failure
            */
            void fail(failureReason reason){

                if(enabled == false){
                    return;
                }

                if(reason == failureReason::SUCCESS_NO_ERROR){
                    log_e("Function is being called with no error present");
                    return;
                }

                enabled = false;

                if(this->failureCallback){
                    this->failureCallback(this->address, reason);
                }
            };


            /* Sets a failure using an i2c error response code */
            void fail(uint8_t i2cError){

                switch(i2cError){

                    case 0:
                        this->fail(failureReason::SUCCESS_NO_ERROR);
                        break;

                    case 1:
                        this->fail(failureReason::DATA_TRANSMIT_BUFFER_ERROR);
                        break;

                    case 2:
                        this->fail(failureReason::ADDRESS_OFFLINE);
                        break;

                    case 3:
                        this->fail(failureReason::TRANSMIT_NOT_ACKNOLWEDGED);
                        break;

                    case 4:
                        this->fail(failureReason::OTHER_ERROR);
                        break;

                    case 5:
                        this->fail(failureReason::TIMEOUT);
                        break;

                    case 10:
                        this->fail(failureReason::INVALID_HARDWARE_CONFIGURATION);
                        break;

                    #if OUTPUT_CONTROLLER_MODEL == ENUM_OUTPUT_CONTROLLER_MODEL_PCA9685

                        case 0xFF:
                            this->fail(failureReason::GENERIC_ERROR);
                            break;

                        case 0xFE:
                            this->fail(failureReason::CHANNEL_OUT_OF_RANGE);
                            break;

                        case 0xFD:
                            this->fail(failureReason::INVALID_MODE_REGISTER);
                            break;

                        
                        case 0xFC:
                            this->fail(failureReason::GENERIC_I2C_ERROR);
                            break;
                    #endif

                    default:
                        this->fail(failureReason::UNKNOWN_ERROR);
                }
            };
    };


    class outputPin{

        public:

            /** Enumeration of the output type, which is either binary or variable (PWM) */
            enum outputType{
                BINARY = 0,
                VARIABLE = 1
            };

            outputType type = BINARY; //Default all outputs are binary for safety
            uint8_t port = 0; /* Human-readable port number */
            uint8_t pin = 0; /* Output controller pin which is attached to this output */
            outputController* controller; /* Reference to the output controller */
            boolean enabled = true; /* If the output is enabled */
            char id[OUTPUT_ID_MAX_LENGTH+1]; /* Identifier for the output */

            #if OUTPUT_CONTROLLER_MODEL == ENUM_OUTPUT_CONTROLLER_MODEL_PCA9685
                uint16_t value = 0; /* Expected PWM value for the pin */
            #endif


            /** Sets the value of the pin power output
             * @param value as a percentage of brightness/duty cycle 0-100, inclusive.  For binary output types, any value greater than 0 will be replaced with 100%
             * @returns status on request
            */
            set_result set(int16_t value){

                uint16_t pwmValue = 0;

                if(this->controller->enabled == false){
                    return set_result::CONTROLLER_NOT_ENABLED;
                }

                if(this->enabled == false){
                    return set_result::PORT_NOT_ENABLED;
                }

                value = constrain(value, 0, 100);

                if(value < 5){
                    value = 0;
                }

                if(value > 95){
                    value = 100;
                }

                switch(type){

                    case BINARY:
                        if(value != 0){
                            pwmValue = OUTPUT_CONTROLLER_MAXIMUM_PWM;
                        }else{
                            pwmValue = 0;
                        }
                        break;
                
                    default:
                        pwmValue = int(map(value, 0, 100, 0, OUTPUT_CONTROLLER_MAXIMUM_PWM));
                        break;
                }

                if(pwmValue == this->value){
                    return set_result::EXCESSIVE;
                }

                #if OUTPUT_CONTROLLER_MODEL == ENUM_OUTPUT_CONTROLLER_MODEL_PCA9685
                    this->controller->hardware.setPWM(pin, pwmValue);

                    if(this->controller->hardware.lastError() != 0){
                        this->controller->fail(controller->hardware.lastError());
                        return set_result::FAILED;
                    }

                    this->value = pwmValue;
                #endif

                return set_result::SUCCESS;
            }


            /** Gets the value of the pin power output
             * @returns Percentage brightness/duty cycle for the given pin, 0-100 inclusive
            */
            uint8_t get(){

                #if OUTPUT_CONTROLLER_MODEL == ENUM_OUTPUT_CONTROLLER_MODEL_PCA9685
                    return int(map(this->value, 0, OUTPUT_CONTROLLER_MAXIMUM_PWM,  0, 100));
                #endif
            }
    };


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

        private:

            bool _initialized = false; /* If the class has been initialized */
            outputController outputControllers[OUTPUT_CONTROLLER_COUNT]; /** Stores the output controller details */
            outputPin outputs[OUTPUT_CONTROLLER_COUNT_PINS * OUTPUT_CONTROLLER_COUNT]; /** Stores the output pin details */

            /** Reference to the callback function that will be called when an output controller has failed */
            void (*ptrFailureCallback)(uint8_t, failureReason);


        public:

            struct healthResult{
                const uint8_t count = OUTPUT_CONTROLLER_COUNT;
                structHealth outputControllers[OUTPUT_CONTROLLER_COUNT];
            };


            /**
             * Initializes the class and loads all the controller and output information
            */
            void begin(){

                if(this->_initialized == true){
                    return;
                }

                const uint8_t addressesOutputController[] = OUTPUT_CONTROLLER_ADDRESSES;

                if(OUTPUT_CONTROLLER_COUNT != sizeof(addressesOutputController)/sizeof(uint8_t)){

                    log_e("OUTPUT_CONTROLLER_ADDRESSES and the length of OUTPUT_CONTROLLER_ADDRESSES are mismatched in hardware.h; Disabling outputs");
                    outputController invalid;
                    invalid.address = 0;
                    invalid.fail(failureReason::INVALID_HARDWARE_CONFIGURATION);

                    return;
                }

                uint8_t portPinMap[OUTPUT_CONTROLLER_COUNT_PINS * OUTPUT_CONTROLLER_COUNT] = OUTPUT_CONTROLLER_PORTS;

                for(int i = 0; i < OUTPUT_CONTROLLER_COUNT; i++){
                    this->outputControllers[i].address = addressesOutputController[i];
                    this->outputControllers[i].failureCallback = ptrFailureCallback;
                    
                    for(int j = 0; j < OUTPUT_CONTROLLER_COUNT_PINS; j++){

                        if(portPinMap[j] == 0){
                            log_e("OUTPUT_CONTROLLER_COUNT_PINS and the length of OUTPUT_CONTROLLER_PORTS are mismatched in hardware.h; Disabling outputs");
                            this->outputControllers[i].fail(failureReason::INVALID_HARDWARE_CONFIGURATION);
                            return;
                        }

                        outputs[(OUTPUT_CONTROLLER_COUNT_PINS * i) + j].controller = &this->outputControllers[i];
                        outputs[(OUTPUT_CONTROLLER_COUNT_PINS * i) + j].pin = j;
                        outputs[(OUTPUT_CONTROLLER_COUNT_PINS * i) + j].port = portPinMap[j];
                    }

                    #if OUTPUT_CONTROLLER_MODEL == ENUM_OUTPUT_CONTROLLER_MODEL_PCA9685
                        this->outputControllers[i].hardware = PCA9685(outputControllers[i].address);

                        if(this->outputControllers[i].hardware.begin() == false){
                            this->outputControllers[i].fail(this->outputControllers[i].hardware.lastError());
                        };

                        this->outputControllers[i].hardware.allOFF();
                        
                        if(outputControllers[i].hardware.lastError() != 0){
                            this->outputControllers[i].fail(this->outputControllers[i].hardware.lastError());
                        }

                        this->outputControllers[i].hardware.setFrequency(OUTPUT_CONTROLLER_FREQUENCY_PWM);
                        
                        if(outputControllers[i].hardware.lastError() != 0){
                            this->outputControllers[i].fail(this->outputControllers[i].hardware.lastError());
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

                return returnValue;
            }


            /** Sets the callback function that is called when an output controller failure occurs */
            void setCallback_failure(void (*userDefinedCallback)(uint8_t, failureReason)) {
                ptrFailureCallback = userDefinedCallback; }

            
            /** Gets the port's value (power output/brightness/duty cycle) as a percentage, 0-100 inclusive
             * @param port as the human-readable port number to get
             * @returns Percentage power output/brightness/duty cycle for the given port.  If an invalid port number is provided, 0 will be returned
            */
            uint8_t getPortValue(uint8_t port){

                for(int i = 0; i < OUTPUT_CONTROLLER_COUNT_PINS * OUTPUT_CONTROLLER_COUNT; i++){
                    if(this->outputs[i].port == port){
                        return outputs[i].get();
                    }
                }
                return 0;
            }


            /** Sets the power output/brightness/duty cycle for the given port and value 
             * @param port as the human-readable port number to set
             * @param value percentage power output/brightness/duty cycle for the given port.  If the port is set as binary output, anything >0 will be set to 100, all other numbers will be set to 0
             * @returns enumerated result of the request
            */
            set_result setPortValue(uint8_t port, int8_t value){

                for(int i = 0; i < OUTPUT_CONTROLLER_COUNT_PINS * OUTPUT_CONTROLLER_COUNT; i++){
                    if(outputs[i].port == port){
                        return outputs[i].set(value);
                    }
                }
                return set_result::INVALID_PORT;
            }


            /** Sets the port type (binary, variable)
             * @param port as the human-readable port number to set
             * @param type as the port output type
            */
            void setPortType(uint8_t port, outputPin::outputType type){

                for(int i = 0; i < OUTPUT_CONTROLLER_COUNT_PINS * OUTPUT_CONTROLLER_COUNT; i++){
                    if(this->outputs[i].port == port){
                        this->outputs[i].type = type;
                        return;
                    }
                }
            }

            /** Gets the port type and sets the specified buffer with the value (binary, variable)
             * @param port as the human-readable port number to get
             * @param buffer as a pointer to a buffer where the output type should be set
             * @returns true on success, false if the pin specified was invalid
            */
            boolean getPortType(uint8_t port, outputPin::outputType &buffer){

                for(int i = 0; i < OUTPUT_CONTROLLER_COUNT_PINS * OUTPUT_CONTROLLER_COUNT; i++){
                    if(this->outputs[i].port == port){
                        buffer = this->outputs[i].type;
                        return true;
                    }
                }
                return false;
            }

            /** Enables or disables a port
             * @param port as the human-readable port number to set
             * @param enabled if the port should be enabled or disabled
             * @note When a port is disabled, its output will be set to off prior to being disabled
            */
            void enablePort(uint8_t port, boolean enabled){

                for(int i = 0; i < OUTPUT_CONTROLLER_COUNT_PINS * OUTPUT_CONTROLLER_COUNT; i++){
                    if(this->outputs[i].port == port){
                        if(enabled == false){
                            this->outputs[i].set(0);
                        }
                        this->outputs[i].enabled = enabled;
                        return;
                    }
                }
            }


            /**
             * Sets the port's ID, which is used for MQTT
             * @param port as the physical port number to set
             * @param id as the unique ID used as a reference
             */
            void setPortId(uint8_t port, const char* id){

                for(int i = 0; i < OUTPUT_CONTROLLER_COUNT_PINS * OUTPUT_CONTROLLER_COUNT; i++){
                    if(this->outputs[i].port == port){
                        strlcpy(this->outputs[i].id, id, sizeof(this->outputs[i].id));
                        return;
                    }
                }
            }

            /**
             * Gets the port's ID, which is used for MQTT
             * @param port as the physical port number to get
             * @param id as a pointer to a char* (with size of OUTPUT_ID_MAX_LENGTH + 1) where the unique ID will be written
             */
            void getPortId(uint8_t port, char* &id){

                for(int i = 0; i < OUTPUT_CONTROLLER_COUNT_PINS * OUTPUT_CONTROLLER_COUNT; i++){
                    if(this->outputs[i].port == port){
                        strcpy(id, this->outputs[i].id);
                    }
                }
            }
    };
};