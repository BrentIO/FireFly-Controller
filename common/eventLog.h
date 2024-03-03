#include "hardware.h"
#include <LinkedList.h>
#include <NTPClient.h>

#ifndef eventLog_h
    #define eventLog_h

    class EventLog{

        public:

            enum logLevel{
                LOG_LEVEL_INFO = 0, //Event is for informational purposes only
                LOG_LEVEL_NOTIFICATION = 1, //Event is important and will cause the OLED display (if equipped) to be shown
                LOG_LEVEL_ERROR = 2 //Event is an error condition and must be shown on the OLED errors page (if equipped)
            };

            struct eventLogEntry{
                unsigned long timestamp = 0; //Time at which the event occurred in seconds.  If NTP was available at time the event occurred, Epoch time will be used, otherwise elapsed time since boot will be used
                char* text = ""; //Descriptive text of the event
                logLevel level = LOG_LEVEL_INFO; //Severity level of the event
            };

        private:

            LinkedList<eventLogEntry> _eventLog;
            LinkedList<char*> _errors;
            NTPClient* _timeClient;

            void (*_ptrNotificationCallback)(); //Function to call when there is a notification logged
            void (*_ptrErrorCallback)(); //Function to call when there is an error logged
            void (*_ptrResolvedErrorCallback)(); //Function to call when the error is resolved

            /**
             * Logs errors to the error event log, ensuring only one event with that text description is maintained in the log
             * @param text Descriptive text of the error
            */
            void _logError(char* text){

                for(int i=0; i < this->_errors.size(); i++){
                    if(this->_errors.get(i) == text){
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
            };

          
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
            void createEvent(char* text, logLevel level = LOG_LEVEL_INFO){

                eventLogEntry newEvent;

                if(this->_timeClient->isTimeSet()){
                    newEvent.timestamp = this->_timeClient->getEpochTime();
                }else{
                    newEvent.timestamp = esp_timer_get_time()/1000000;
                }

                newEvent.level = level;
                newEvent.text = text;

                if(this->_eventLog.size() >= EVENT_LOG_MAXIMUM_ENTRIES){
                    this->_eventLog.shift();
                }

                this->_eventLog.add(newEvent);

                if(newEvent.level == LOG_LEVEL_NOTIFICATION){
                    if(this->_ptrNotificationCallback){
                        this->_ptrNotificationCallback();
                    }
                }

                if(newEvent.level == LOG_LEVEL_ERROR){
                    this->_logError(text);
                    if(this->_ptrErrorCallback){
                        this->_ptrErrorCallback();
                    }
                }
            }


            /**
             * Creates a new event in the event log
             * @param text Descriptive text about the event
             * @param level Severity level of the event
            */
            void createEvent(const __FlashStringHelper *text, logLevel level = LOG_LEVEL_INFO){
                createEvent((char*)text, level);
            }

            /**
             * Retrieves the list of events from the event log
            */
            LinkedList<eventLogEntry>* getEvents(){
                return &_eventLog;
            }

            /**
             * Retrieves the list of errors from the error log
            */
            LinkedList<char*>* getErrors(){
                return &_errors;
            }

            /** 
             * Resolves an error that was previously logged
             * 
             * @param text Descriptive text of the original error that should be resolved
             * 
            */
            void resolveError(char* text){

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