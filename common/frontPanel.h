#include "hardware.h"

class managerFrontPanel{

    public:

        enum status{
            NORMAL = 0, //System is operating nomally
            TROUBLE = 1, //Temporary issue which can be corrected, such as temporary network outage
            FAILURE = 2 //Catastrophic failure that is non-recoverable
        };

        enum inputState{
            STATE_OPEN = 0,
            STATE_CLOSED = 1
        };


    private:

        enum inputType{
            NORMALLY_OPEN = 0,
            NORMALLY_CLOSED = 1,
        };

        status _ledStatus = TROUBLE;

        /** Convert a bit to inputState */
        inputState bitToInputState(boolean value){

            if(value == LOW){
                return inputState::STATE_CLOSED;
            }else{
                return inputState::STATE_OPEN;
            }

        }

        unsigned long timePreviousChange = 0; /* Time (millis) when the input state last changed.  Value is set to 0 when the state returns to its input type. Default 0.*/
        inputState state = STATE_OPEN; /* The state entered at timePreviousChange. Default STATE_OPEN.*/
        inputType type = NORMALLY_OPEN; /* Defines if the input is normally open or normally closed. Default NORMALLY_OPEN.*/

        void (*ptrPublisherCallback)(void);
        void (*ptrStateClosedAtBeginCallback)(void);

    public:

        void begin(){
            pinMode(OLED_BUTTON_PIN, INPUT_PULLUP);
            pinMode(OLED_LED_PIN, OUTPUT);

            inputState currentState = bitToInputState(digitalRead(OLED_BUTTON_PIN));

            if(currentState == inputState::STATE_CLOSED){

                if(this->ptrStateClosedAtBeginCallback){
                    this->ptrStateClosedAtBeginCallback();
                }
            }

            state = currentState;

            this->setStatus(managerFrontPanel::status::NORMAL);
        }

        void setCallback_publisher(void (*userDefinedCallback)(void)) {
            ptrPublisherCallback = userDefinedCallback; }

        void setCallback_state_closed_at_begin(void (*userDefinedCallback)(void)) {
            ptrStateClosedAtBeginCallback = userDefinedCallback; }

        void setStatus(managerFrontPanel::status value){

            if(_ledStatus == managerFrontPanel::FAILURE) { 
                return; //Prevent the status from being returned to normal
            }

            switch(value){
                case managerFrontPanel::status::FAILURE:
                    digitalWrite(OLED_LED_PIN, LOW);
                    break;

                case managerFrontPanel::status::NORMAL:
                    digitalWrite(OLED_LED_PIN, HIGH);
                    break;

                case managerFrontPanel::status::TROUBLE:
                    digitalWrite(OLED_LED_PIN, LOW);
                    break;                        

                default:
                    digitalWrite(OLED_LED_PIN, LOW);
                    _ledStatus = managerFrontPanel::status::TROUBLE;
                    break;
            }

            _ledStatus = value;
        }

        inputState getButtonState(){

            return bitToInputState(digitalRead(OLED_BUTTON_PIN));

        }

        void loop(){

            inputState currentState = bitToInputState(digitalRead(OLED_BUTTON_PIN));

            if(currentState == state){
                return;
            }

            //Values are different; Check if we are within the debounce delay and that the status isn't its normal state
            if((millis() - timePreviousChange < DEBOUNCE_DELAY) && ((type == NORMALLY_OPEN && currentState != STATE_OPEN) || (type == NORMALLY_CLOSED && currentState != STATE_CLOSED))){

                log_v("Front Panel Button: DEBOUNCE_DELAY (%i) not satisfied. Time Previous Change: %i Current Time: %i Difference: %i Current State: %s State: %s", DEBOUNCE_DELAY, timePreviousChange, millis(), (millis() - timePreviousChange), String(currentState), String(state));
                return;
            }

            timePreviousChange = millis();

            //Check if input is in an abnormal state
            if(currentState == inputState::STATE_CLOSED){

                log_v("Front Panel Button Type: %s New State: %s (Abnormal)", String(type), String(currentState));

                if(ptrPublisherCallback){
                    ptrPublisherCallback();
                }

                //Turn off the front panel LED during a button press
                digitalWrite(OLED_LED_PIN, LOW);

                state = currentState;

                return;
            }

            //Check if input is in normal state
            if(currentState == inputState::STATE_OPEN){

                log_v("Front Panel Button Type: %s New State: %s (Normal)", String(type), String(currentState));
        
                //Turn on the LED when the button is released
                if(_ledStatus == managerFrontPanel::status::NORMAL){

                    //The status is normal, turn the LED back on
                    digitalWrite(OLED_LED_PIN, HIGH);
                }

                state = currentState;

                return;
            }
        }
    };