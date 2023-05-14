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
            pinMode(PIN_OLED_BUTTON, INPUT_PULLUP);
            pinMode(PIN_OLED_LED, OUTPUT);

            inputState currentState = bitToInputState(digitalRead(PIN_OLED_BUTTON));

            if(currentState == inputState::STATE_CLOSED){

                if(this->ptrStateClosedAtBeginCallback){
                    this->ptrStateClosedAtBeginCallback();
                }
            }

            state = currentState;
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
                    digitalWrite(PIN_OLED_LED, LOW);
                    break;

                case managerFrontPanel::status::NORMAL:
                    digitalWrite(PIN_OLED_LED, HIGH);
                    break;

                case managerFrontPanel::status::TROUBLE:
                    digitalWrite(PIN_OLED_LED, LOW);
                    break;                        

                default:
                    digitalWrite(PIN_OLED_LED, LOW);
                    _ledStatus = managerFrontPanel::status::TROUBLE;
                    break;
            }

            _ledStatus = value;
        }

        inputState getButtonState(){

            return bitToInputState(digitalRead(PIN_OLED_BUTTON));

        }

        void loop(){

            inputState currentState = bitToInputState(digitalRead(PIN_OLED_BUTTON));

            if(currentState == state){
                return;
            }

            //Values are different; Check if we are within the debounce delay and that the status isn't its normal state
            if((millis() - timePreviousChange < DEBOUNCE_DELAY) && currentState != type){
                           
                #ifdef DEBUG
                    #if DEBUG > 50
                        Serial.println("(frontPanel.h->loop) Front Panel Button: DEBOUNCE_DELAY (" + String(DEBOUNCE_DELAY) + ") not satisfied. Time Previous Change: " + String(timePreviousChange) + " Current Time: " + String(millis()) + " Difference: " + String(millis() - timePreviousChange) +  " Current State: " + String(currentState) + " State: " + String(state));
                    #endif
                #endif

                return;
            }

            timePreviousChange = millis();

            //Check if input is in an abnormal state
            if(currentState == inputState::STATE_CLOSED){

                #ifdef DEBUG
                    #if DEBUG > 50
                        Serial.println("(frontPanel.h->loop) Front Panel Button Type: " + String(type) + " New State: " + String(currentState) + " (Abnormal)");
                    #endif
                #endif

                if(ptrPublisherCallback){
                    ptrPublisherCallback();
                }

                //Turn off the front panel LED during a button press
                digitalWrite(PIN_OLED_LED, LOW);

                state = currentState;

                return;
            }

            //Check if input is in normal state
            if(currentState == inputState::STATE_OPEN){
        
                #ifdef DEBUG
                    #if DEBUG > 50
                        Serial.println("(frontPanel.h->loop) Front Panel Button Type: " + String(type) + " New State: " + String(currentState) + " (Normal)");
                    #endif
                #endif

                //Turn on the LED when the button is released
                if(_ledStatus == managerFrontPanel::status::NORMAL){

                    //The status is normal, turn the LED back on
                    digitalWrite(PIN_OLED_LED, HIGH);
                }

                state = currentState;

                return;
            }
        }
    };