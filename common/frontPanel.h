#include "hardware.h"

class managerFrontPanelButton{

    public:
        enum status{
            NORMAL = 0, //System is operating nomally
            TROUBLE = 1, //Temporary issue which can be corrected, such as temporary network outage
            FAILURE = 2 //Catastrophic failure that is non-recoverable
        };

        enum inputState{
            STATE_OPEN = LOW,
            STATE_CLOSED = HIGH
        };

        enum inputType{
            NORMALLY_OPEN = 0,
            NORMALLY_CLOSED = 1,
        };

    private:
        managerFrontPanelButton::status _ledStatus = TROUBLE;

    public:

        inputState state = STATE_OPEN; /* The state entered at timePreviousChange. Default STATE_OPEN.*/
        inputType type = NORMALLY_OPEN; /* Defines if the input is normally open or normally closed. Default NORMALLY_OPEN.*/

        void begin(){
            pinMode(PIN_OLED_BUTTON, INPUT);
            pinMode(PIN_OLED_LED, OUTPUT);
        }

        void setStatus(managerFrontPanelButton::status value){

            if(_ledStatus == managerFrontPanelButton::FAILURE) { 
                return; //Prevent the status from being returned to normal
            }

            switch(value){
                case managerFrontPanelButton::status::FAILURE:
                    digitalWrite(PIN_OLED_LED, LOW);
                    break;

                case managerFrontPanelButton::status::NORMAL:
                    digitalWrite(PIN_OLED_LED, HIGH);
                    break;

                case managerFrontPanelButton::status::TROUBLE:
                    digitalWrite(PIN_OLED_LED, LOW);
                    break;                        

                default:
                    digitalWrite(PIN_OLED_LED, LOW);
                    _ledStatus = managerFrontPanelButton::status::TROUBLE;
                    break;
            }

            _ledStatus = value;
        }
    };