#ifndef extendedPubSubClient_h
    #define extendedPubSubClient_h

    #include <PubSubClient.h>

    #define WORD_FIREFLY_SLASH "FireFly/"
    #define WORD_LENGTH_FIREFLY 8           //len("FireFly/")
    #define WORD_LENGTH_SET 4               //len("/set")
    #define WORD_LENGTH_STATE 6             //len("/state")
    #define WORD_LENGTH_AVAILABILITY 13     //len("/availability")
    #define WORD_LENGTH_CONFIG 7            //len("/config")
    #define WORD_LENGTH_INPUTS 7             //len("inputs/")
    #define WORD_LENGTH_CIRCUITS 9             //len("circuits/")
    #define WORD_LENGTH_INTEGRATION 20        //The longest MQTT integration allowed in Home Assistant is len("/alarm_control_panel"), see https://www.home-assistant.io/integrations/mqtt/
    #define WORD_LENGTH_AUTODISCOVERY_ROOT 24 //len(this->auto_discovery_root)
    #define UUID_LENGTH 36                  //len(uuidv4)
    #define MQTT_USERNAME_MAX_LENGTH 64
    #define MQTT_PASSWORD_MAX_LENGTH 64


    /***************** I/O TOPICS *****************/

    #define MQTT_TOPIC_INPUT_STATE_PATTERN WORD_FIREFLY_SLASH "inputs/%sC%i/state"           //%s = Port ID, %s = channel number
    #define MQTT_TOPIC_INPUT_STATE_PATTERN_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_INPUTS + PORT_ID_MAX_LENGTH + 1 + 1 + WORD_LENGTH_STATE
    
    #define MQTT_TOPIC_OUTPUT_SET_PATTERN WORD_FIREFLY_SLASH "circuits/%s/set"              //%s = Output ID
    #define MQTT_TOPIC_OUTPUT_SET_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_CIRCUITS + OUTPUT_ID_MAX_LENGTH + WORD_LENGTH_SET    // "FireFly/" + output ID + "/set"
    #define MQTT_TOPIC_OUTPUT_STATE_PATTERN WORD_FIREFLY_SLASH "circuits/%s/state"          //%s = Output ID
    #define MQTT_TOPIC_OUTPUT_STATE_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_CIRCUITS + OUTPUT_ID_MAX_LENGTH + WORD_LENGTH_STATE // "FireFly/" + output ID + "/state"
    #define MQTT_TOPIC_OUTPUT_AUTO_DISCOVERY_PATTERN "%s/%s/%s/config"  //%s = Home Assistant root topic (defaults to "homeassistant"), %s = device platform (light/fan/switch/etc), %s = output ID
    #define MQTT_TOPIC_OUTPUT_AUTO_DISCOVERY_LENGTH WORD_LENGTH_AUTODISCOVERY_ROOT + WORD_LENGTH_INTEGRATION + 1 + OUTPUT_ID_MAX_LENGTH + WORD_LENGTH_CONFIG
    #define MQTT_OUTPUT_AUTO_DISCOVERY_UNIQUE_ID_PATTERN "FireFly-%s"       //%s = output ID
    #define MQTT_OUTPUT_AUTO_DISCOVERY_UNIQUE_ID_LENGTH WORD_LENGTH_FIREFLY + OUTPUT_ID_MAX_LENGTH

    #define MQTT_OUTPUT_DEVICE_NAME_PATTERN "%s %s"     //%s = Output name, %s = Output ID.  If the output has no name field, the function will change this to only %s = Output ID
    #define MQTT_OUTPUT_DEVICE_NAME_LENGTH OUTPUT_NAME_MAX_LENGTH + 1 + OUTPUT_ID_MAX_LENGTH


    /***************** PERIPHERAL TOPICS *****************/

    #define MQTT_TOPIC_TIME_START_STATE_PATTERN WORD_FIREFLY_SLASH "%s/time-start/state"     //%s = Controller UUID
    #define MQTT_TOPIC_TIME_START_STATE_PATTERN_LENGTH WORD_LENGTH_FIREFLY + UUID_LENGTH + 11 + WORD_LENGTH_STATE
    #define MQTT_TOPIC_TIME_START_AUTO_DISCOVERY_PATTERN "%s/sensor/FireFly-%s-time-start/config"     //%s = Home Assistant root topic (defaults to "homeassistant"), %s = Controller UUID
    #define MQTT_TOPIC_TIME_START_AUTO_DISCOVERY_LENGTH WORD_LENGTH_AUTODISCOVERY_ROOT + 1 + WORD_LENGTH_INTEGRATION + 7 + 1 + UUID_LENGTH + 1 + 10 + WORD_LENGTH_CONFIG
    #define MQTT_TIME_START_AUTO_DISCOVERY_UNIQUE_ID_PATTERN "FireFly-%s-time-start"       //%s = Controller UUID
    #define MQTT_TIME_START_AUTO_DISCOVERY_UNIQUE_ID_LENGTH WORD_LENGTH_FIREFLY + UUID_LENGTH + 1 + 10

    #define MQTT_TOPIC_IP_ADDRESS_STATE_PATTERN WORD_FIREFLY_SLASH "%s/ip-address/state"     //%s = Controller UUID
    #define MQTT_TOPIC_IP_ADDRESS_STATE_PATTERN_LENGTH WORD_LENGTH_FIREFLY + UUID_LENGTH + 11 + WORD_LENGTH_STATE //len("/ip-address")
    #define MQTT_TOPIC_IP_ADDRESS_AUTO_DISCOVERY_PATTERN "%s/sensor/FireFly-%s-ip-address/config"     //%s = Home Assistant root topic (defaults to "homeassistant"), %s = Controller UUID
    #define MQTT_TOPIC_IP_ADDRESS_AUTO_DISCOVERY_LENGTH WORD_LENGTH_AUTODISCOVERY_ROOT + 1 + WORD_LENGTH_INTEGRATION + 7 + 1 + UUID_LENGTH + 1 + 10 + WORD_LENGTH_CONFIG
    #define MQTT_IP_ADDRESS_AUTO_DISCOVERY_UNIQUE_ID_PATTERN "FireFly-%s-ip-address"       //%s = Controller UUID
    #define MQTT_IP_ADDRESS_AUTO_DISCOVERY_UNIQUE_ID_LENGTH WORD_LENGTH_FIREFLY + UUID_LENGTH + 1 + 10

    #define MQTT_TOPIC_MAC_ADDRESS_STATE_PATTERN WORD_FIREFLY_SLASH "%s/mac-address/state"     //%s = Controller UUID
    #define MQTT_TOPIC_MAC_ADDRESS_STATE_PATTERN_LENGTH WORD_LENGTH_FIREFLY + UUID_LENGTH + 12 + WORD_LENGTH_STATE //len("/mac-address")
    #define MQTT_TOPIC_MAC_ADDRESS_AUTO_DISCOVERY_PATTERN "%s/sensor/FireFly-%s-mac-address/config"     //%s = Home Assistant root topic (defaults to "homeassistant"), %s = Controller UUID
    #define MQTT_TOPIC_MAC_ADDRESS_AUTO_DISCOVERY_LENGTH WORD_LENGTH_AUTODISCOVERY_ROOT + 1 + WORD_LENGTH_INTEGRATION + 7 + 1 + UUID_LENGTH + 1 + 11 + WORD_LENGTH_CONFIG
    #define MQTT_MAC_ADDRESS_AUTO_DISCOVERY_UNIQUE_ID_PATTERN "FireFly-%s-mac-address"       //%s = Controller UUID
    #define MQTT_MAC_ADDRESS_AUTO_DISCOVERY_UNIQUE_ID_LENGTH WORD_LENGTH_FIREFLY + UUID_LENGTH + 1 + 11

    #define MQTT_TOPIC_COUNT_ERRORS_STATE_PATTERN WORD_FIREFLY_SLASH "%s/count-errors/state"      //%s = Controller UUID
    #define MQTT_TOPIC_COUNT_ERRORS_STATE_PATTERN_LENGTH WORD_LENGTH_FIREFLY + UUID_LENGTH + 13 + WORD_LENGTH_STATE //len("/count-errors")
    #define MQTT_TOPIC_COUNT_ERRORS_AUTO_DISCOVERY_PATTERN "%s/sensor/FireFly-%s-count-errors/config"     //%s = Home Assistant root topic (defaults to "homeassistant"), %s = Controller UUID
    #define MQTT_TOPIC_COUNT_ERRORS_AUTO_DISCOVERY_LENGTH WORD_LENGTH_AUTODISCOVERY_ROOT + 1 + WORD_LENGTH_INTEGRATION + 7 + 1 + UUID_LENGTH + 1 + 12 + WORD_LENGTH_CONFIG
    #define MQTT_COUNT_ERRORS_AUTO_DISCOVERY_UNIQUE_ID_PATTERN "FireFly-%s-count-errors"       //%s = Controller UUID
    #define MQTT_COUNT_ERRORS_AUTO_DISCOVERY_UNIQUE_ID_LENGTH WORD_LENGTH_FIREFLY + UUID_LENGTH + 1 + 12

    #define MQTT_TOPIC_TEMPERATURE_STATE_PATTERN WORD_FIREFLY_SLASH "%s/temperature/%s/state"        //%s = Controller UUID, %s = location
    #define MQTT_TOPIC_TEMPERATURE_STATE_PATTERN_LENGTH WORD_LENGTH_FIREFLY + UUID_LENGTH + 1 + 12 + WORD_LENGTH_STATE  //len("/temperature")
    #define MQTT_TOPIC_TEMPERATURE_AUTO_DISCOVERY_PATTERN "%s/sensor/FireFly-%s-temperature-%s/config"     //%s = Home Assistant root topic (defaults to "homeassistant"), %s = Controller UUID, %s = location
    #define MQTT_TOPIC_TEMPERATURE_AUTO_DISCOVERY_LENGTH WORD_LENGTH_AUTODISCOVERY_ROOT + 1 + WORD_LENGTH_INTEGRATION + 7 + 1 + UUID_LENGTH + 1 + 11 + 1 + 8 + WORD_LENGTH_CONFIG
    #define MQTT_TEMPERATURE_AUTO_DISCOVERY_UNIQUE_ID_PATTERN "FireFly-%s-temperature-%s"       //%s = Controller UUID, %s = location
    #define MQTT_TEMPERATURE_AUTO_DISCOVERY_UNIQUE_ID_LENGTH WORD_LENGTH_FIREFLY + UUID_LENGTH + 1 + 11 + 1 + 8


    /***************** FIRMWARE UPDATE TOPICS *****************/

    #define MQTT_TOPIC_UPDATE_STATE_PATTERN WORD_FIREFLY_SLASH "%s/update/state"       //%s = Controller UUID
    #define MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH WORD_LENGTH_FIREFLY + UUID_LENGTH + 1 + 6 + WORD_LENGTH_STATE  //len("/update")
    #define MQTT_TOPIC_UPDATE_SET_PATTERN WORD_FIREFLY_SLASH "%s/update/set"       //%s = Controller UUID
    #define MQTT_TOPIC_UPDATE_SET_PATTERN_LENGTH WORD_LENGTH_FIREFLY + UUID_LENGTH + 1 + 6 + WORD_LENGTH_SET //len("/update")
    #define MQTT_TOPIC_UPDATE_AUTO_DISCOVERY_PATTERN "%s/update/FireFly-%s-update/config"     //%s = Home Assistant root topic (defaults to "homeassistant"), %s = Controller UUID
    #define MQTT_TOPIC_UPDATE_AUTO_DISCOVERY_PATTERN_LENGTH WORD_LENGTH_AUTODISCOVERY_ROOT + 1 + WORD_LENGTH_INTEGRATION + 7 + 1 + UUID_LENGTH + 1 + 6 + WORD_LENGTH_CONFIG
    #define MQTT_UPDATE_AUTO_DISCOVERY_UNIQUE_ID_PATTERN "FireFly-%s-update"       //%s = Controller UUID
    #define MQTT_UPDATE_AUTO_DISCOVERY_UNIQUE_ID_LENGTH WORD_LENGTH_FIREFLY + UUID_LENGTH + 1 + 6


    /*****************  FIRMWARE UPDATE SERVICE AVAILABILITY TOPICS *****************/

    #define MQTT_TOPIC_UPDATE_AVAILABILITY_PATTERN WORD_FIREFLY_SLASH "%s/update/availability"    //%s = Controller UUID
    #define MQTT_TOPIC_UPDATE_AVAILABILITY_LENGTH WORD_LENGTH_FIREFLY + UUID_LENGTH + 1 + 6 + WORD_LENGTH_AVAILABILITY


    /***************** CONTROLLER AVAILABILITY TOPICS *****************/

    #define MQTT_TOPIC_CONTROLLER_AVAILABILITY_LENGTH WORD_LENGTH_FIREFLY + UUID_LENGTH + WORD_LENGTH_AVAILABILITY
    #define MQTT_TOPIC_CONTROLLER_AVAILABILITY_PATTERN WORD_FIREFLY_SLASH "%s/availability"    //%s = Controller UUID


    class exPubSubClient : public PubSubClient
    {           

        public:
            using PubSubClient::PubSubClient; /* Inherit the base PubSubClient */
            const char* topic_availability; /* Topic name for availability, which will be used as the last will topic name as well */
            int64_t lastReconnectAttemptTime = 0; /* The time (millis() or equivalent) when the last reconnection was be attempted */
            LinkedList<const char*> subscriptions; /* List of MQTT subscriptions */
            char username[MQTT_USERNAME_MAX_LENGTH + 1]; /* MQTT username to use when authenticating */
            char password[MQTT_PASSWORD_MAX_LENGTH + 1]; /* MQTT password to use when authenticating */
            bool enabled = false; /* Enabled only if credentials and configuration exists */

            
            struct autoDiscovery{
                bool sent = false;
                char homeAssistantRoot[WORD_LENGTH_AUTODISCOVERY_ROOT+1] = "homeassistant";
                char deviceName[CONTROLLER_NAME_MAX_LENGTH+1] = "FireFly Controller";
                char suggestedArea[CONTROLLER_AREA_MAX_LENGTH+1];

                /**
                 * Changes the Home Assistant auto discovery root path from the default of "homeassistant" to the value specified
                 * @param value the new root name to use, without a trailing slash
                 */
                void setHomeAssistantRoot(const char* value){
                    strcpy(homeAssistantRoot, value);
                }

                void setDeviceName(const char* value){
                    strcpy(deviceName, value);
                }

                void setSuggestedArea(const char* value){
                    strcpy(suggestedArea, value);
                }
            } autoDiscovery;


            /***
             * Adds a new subscription to the list and automatically subscribes to the topic 
             */
            void addSubscription(const char* topic){
                this->subscriptions.add(topic);
                this->subscribe(topic);
            }


            /***
             * Processes all subscriptions in the list and resubscribes to all of them
             */
            void resubscribe(){
                for(int i=0; i < this->subscriptions.size(); i++){
                    if(!this->subscribe(this->subscriptions.get(i))){
                        log_e("FAILED to subscribe to %s", this->subscriptions.get(i));
                    }
                }
            }


            void setUsername(const char* value){
                strcpy(username, value);
            }


            void setPassword(const char* value){
                strcpy(password, value);
            }

    };

#endif
