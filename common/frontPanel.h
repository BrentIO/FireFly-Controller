#include "hardware.h"

class managerFrontPanel{

    public:

        enum status{
            NORMAL = 0, //System is operating nomally
            TROUBLE = 1, //Temporary issue which can be corrected, such as temporary network outage
            FAILURE = 2 //Catastrophic failure that is non-recoverable
        };


    private:

        enum inputState{
            STATE_OPEN = 0,
            STATE_CLOSED = 1
        };

        enum inputType{
            NORMALLY_OPEN = 0,
            NORMALLY_CLOSED = 1,
        };

    private:
        status _ledStatus = TROUBLE;

    public:

        inputState state = STATE_OPEN; /* The state entered at timePreviousChange. Default STATE_OPEN.*/
        inputType type = NORMALLY_OPEN; /* Defines if the input is normally open or normally closed. Default NORMALLY_OPEN.*/

        void (*ptrPublisherCallback)(void);
        void (*ptrStateClosedAtBeginCallback)(void);

        void begin(){
            pinMode(PIN_OLED_BUTTON, INPUT);
            pinMode(PIN_OLED_LED, OUTPUT);

                if(this->ptrStateClosedAtBeginCallback){
                    this->ptrStateClosedAtBeginCallback();
                }
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
                if(ptrPublisherCallback){
                    ptrPublisherCallback();
                }

    };