#include "hardware.h"
#include <LinkedList.h>
#include <NTPClient.h>

#ifndef eventLog_h
    #define eventLog_h

    #ifndef EVENT_LOG_ENTRY_MAX_LENGTH
        #define EVENT_LOG_ENTRY_MAX_LENGTH 21
    #endif


    class EventLog{

        public:

            enum logLevel{
                LOG_LEVEL_INFO = 0, //Event is for informational purposes only
                LOG_LEVEL_NOTIFICATION = 1, //Event is important and will cause the OLED display (if equipped) to be shown
                LOG_LEVEL_ERROR = 2 //Event is an error condition and must be shown on the OLED errors page (if equipped)
            };

            struct eventLogEntry{
                unsigned long timestamp = 0; //Time at which the event occurred in seconds.  If NTP was available at time the event occurred, Epoch time will be used, otherwise elapsed time since boot will be used
                char text[EVENT_LOG_ENTRY_MAX_LENGTH + 1]; //Descriptive text of the event
                logLevel level = LOG_LEVEL_INFO; //Severity level of the event
            };

        private:

            eventLogEntry* _buffer; /* PSRAM-backed circular buffer for event log entries; falls back to heap if PSRAM unavailable */
            uint16_t _head = 0; /* Index of the oldest entry in the circular buffer */
            uint16_t _count = 0; /* Number of valid entries currently stored */

            LinkedList<const char*> _errors;
            NTPClient* _timeClient;

            void (*_ptrInfoCallback)(); //Function to call when there is an info logged
            void (*_ptrNotificationCallback)(); //Function to call when there is a notification logged
            void (*_ptrErrorCallback)(); //Function to call when there is an error logged
            void (*_ptrResolvedErrorCallback)(); //Function to call when the error is resolved

            /**
             * Logs errors to the error event log, ensuring only one event with that text description is maintained in the log
             * @param text Descriptive text of the error
            */
            void _logError(const char* text){

                for(int i=0; i < this->_errors.size(); i++){
                    if(strcmp(this->_errors.get(i), text) == 0){
                        return;
                    }
                }

                if(this->_errors.size() >= EVENT_LOG_MAXIMUM_ENTRIES){
                    this->_errors.shift();
                }

                this->_errors.add(text);
            }

        public:

            /**
             * Creates a new event log handler
             *
             * @param timeClient An NTP client that can be used when logging events
            */
            EventLog(NTPClient *timeClient){
                _timeClient = timeClient;
                _buffer = (eventLogEntry*)ps_malloc(EVENT_LOG_MAXIMUM_ENTRIES * sizeof(eventLogEntry));
            };

            /**
             * Sets a callback function when a new info event is entered into the event log
            */
            void setCallback_info(void (*userDefinedCallback)()) {
                this->_ptrInfoCallback = userDefinedCallback; }


            /**
             * Sets a callback function when a new notification event is entered into the event log
            */
            void setCallback_notification(void (*userDefinedCallback)()) {
                this->_ptrNotificationCallback = userDefinedCallback; }


            /**
             * Sets a callback function when a new error event is entered into the event log
            */
            void setCallback_error(void (*userDefinedCallback)()) {
                this->_ptrErrorCallback = userDefinedCallback; }


            /**
             * Sets a callback function when an error is resolved from the event log
            */
            void setCallback_resolveError(void (*userDefinedCallback)()) {
                this->_ptrResolvedErrorCallback = userDefinedCallback; }


            /**
             * Creates a new event in the event log
             * @param text Descriptive text about the event
             * @param level Severity level of the event, defaulting to LOG_LEVEL_INFO if not specified
            */
            void createEvent(const char* text, logLevel level = LOG_LEVEL_INFO){

                eventLogEntry newEvent;

                if(this->_timeClient->isTimeSet()){
                    newEvent.timestamp = this->_timeClient->getEpochTime();
                }else{
                    newEvent.timestamp = esp_timer_get_time()/1000000;
                }

                newEvent.level = level;
                strcpy(newEvent.text, text);

                if(newEvent.level == LOG_LEVEL_ERROR){
                    log_e("New event log entry: [%s]", newEvent.text);
                }else{
                    log_i("New event log entry: [%s]", newEvent.text);
                }

                uint16_t insertIndex = (_head + _count) % EVENT_LOG_MAXIMUM_ENTRIES;

                if(_count < EVENT_LOG_MAXIMUM_ENTRIES){
                    _buffer[insertIndex] = newEvent;
                    _count++;
                } else {
                    /* Buffer full: overwrite oldest entry and advance head */
                    _buffer[_head] = newEvent;
                    _head = (_head + 1) % EVENT_LOG_MAXIMUM_ENTRIES;
                }

                switch(newEvent.level){

                    case LOG_LEVEL_INFO:
                        if(this->_ptrInfoCallback){
                            this->_ptrInfoCallback();
                        }
                        break;

                    case LOG_LEVEL_NOTIFICATION:
                        if(this->_ptrNotificationCallback){
                            this->_ptrNotificationCallback();
                        }
                        break;

                    case LOG_LEVEL_ERROR:
                        this->_logError(text);
                        if(this->_ptrErrorCallback){
                            this->_ptrErrorCallback();
                        }
                        break;

                    default:
                        break;
                }
            }


            /**
             * Creates a new event in the event log
             * @param text Descriptive text about the event
             * @param level Severity level of the event
            */
            void createEvent(const __FlashStringHelper *text, logLevel level = LOG_LEVEL_INFO){
                createEvent((const char*)text, level);
            }

            /**
             * Returns the number of events currently stored in the event log
            */
            uint16_t getEventCount(){
                return _count;
            }

            /**
             * Returns the event at logical index i (0 = oldest, count-1 = newest)
            */
            eventLogEntry getEvent(uint16_t i){
                return _buffer[(_head + i) % EVENT_LOG_MAXIMUM_ENTRIES];
            }

            /**
             * Retrieves the list of errors from the error log
            */
            LinkedList<const char*>* getErrors(){
                return &_errors;
            }

            /**
             * Resolves an error that was previously logged
             *
             * @param text Descriptive text of the original error that should be resolved
             *
            */
            void resolveError(const char* text){

                for(int i=0; i < this->_errors.size(); i++){

                    if(strcmp(this->_errors.get(i), text) == 0){
                        this->_errors.remove(i);

                        if(this->_ptrResolvedErrorCallback){
                            this->_ptrResolvedErrorCallback();
                        }

                        return;
                    }
                }
            }

            /**
             * Resolves an error that was previously logged
             *
             * @param text Descriptive text of the original error that should be resolved
             *
            */
            void resolveError(const __FlashStringHelper *text){
                resolveError((char*)text);
            }
    };

#endif
