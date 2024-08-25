#ifndef extendedPubSubClient_h
    #define extendedPubSubClient_h

    #include <PubSubClient.h>

    #define WORD_FIREFLY_SLASH "FireFly/"
    #define WORD_LENGTH_FIREFLY 8           //len("FireFly/")
    #define WORD_LENGTH_SET 4               //len("/set")
    #define WORD_LENGTH_STATE 6             //len("/state")
    #define WORD_LENGTH_AVAILABILITY 13     //len("/availability")
    #define WORD_LENGTH_CONFIG 7            //len("/config")
    #define WORD_LENGTH_INTEGRATION 20        //The longest MQTT integration allowed in Home Assistant is len("/alarm_control_panel"), see https://www.home-assistant.io/integrations/mqtt/
    #define WORD_LENGTH_AUTODISCOVERY_ROOT 24 //len(this->auto_discovery_root)
    #define UUID_LENGTH 36                  //len(uuidv4)


    /***************** I/O TOPICS *****************/

    #define MQTT_TOPIC_INPUT_STATE_PATTERN WORD_FIREFLY_SLASH "%s/state"           //%s = Input ID
    #define MQTT_TOPIC_INPUT_STATE_PATTERN_LENGTH WORD_LENGTH_FIREFLY + INPUT_ID_MAX_LENGTH + WORD_LENGTH_STATE
    
    #define MQTT_TOPIC_OUTPUT_SET_PATTERN WORD_FIREFLY_SLASH "%s/set"              //%s = Output ID
    #define MQTT_TOPIC_OUTPUT_SET_LENGTH WORD_LENGTH_FIREFLY + OUTPUT_ID_MAX_LENGTH + WORD_LENGTH_SET    // "FireFly/" + output ID + "/set"
    #define MQTT_TOPIC_OUTPUT_STATE_PATTERN WORD_FIREFLY_SLASH "%s/state"          //%s = Output ID
    #define MQTT_TOPIC_OUTPUT_STATE_LENGTH WORD_LENGTH_FIREFLY + OUTPUT_ID_MAX_LENGTH + WORD_LENGTH_STATE // "FireFly/" + output ID + "/state"
    #define MQTT_TOPIC_OUTPUT_AUTO_DISCOVERY_PATTERN "%s/%s/%s/config"  //%s = Home Assistant root topic (defaults to "homeassistant"), %s = device platform (light/fan/switch/etc), %s = output ID
    #define MQTT_TOPIC_OUTPUT_AUTO_DISCOVERY_LENGTH WORD_LENGTH_AUTODISCOVERY_ROOT + WORD_LENGTH_INTEGRATION + 1 + OUTPUT_ID_MAX_LENGTH + WORD_LENGTH_CONFIG
    #define MQTT_OUTPUT_AUTO_DISCOVERY_UNIQUE_ID_PATTERN "FireFly-%s"       //%s = output ID
    #define MQTT_OUTPUT_AUTO_DISCOVERY_UNIQUE_ID_LENGTH WORD_LENGTH_FIREFLY + OUTPUT_ID_MAX_LENGTH


    /***************** CONTROLLER PERIPHERAL TOPICS *****************/

    #define MQTT_TOPIC_TIME_START_STATE_PATTERN WORD_FIREFLY_SLASH "%s/time-start/state"     //%s = Controller UUID
    #define MQTT_TOPIC_TIME_START_STATE_PATTERN_LENGTH WORD_LENGTH_FIREFLY + UUID_LENGTH + 11 + WORD_LENGTH_STATE
    #define MQTT_TOPIC_TIME_START_AUTO_DISCOVERY_PATTERN "%s/sensor/FireFly-%s-start-time/config"     //%s = Home Assistant root topic (defaults to "homeassistant"), %s = Controller UUID

    #define MQTT_TOPIC_IP_ADDRESS_STATE_PATTERN WORD_FIREFLY_SLASH "%s/ip-address/state"     //%s = Controller UUID
    #define MQTT_TOPIC_IP_ADDRESS_STATE_PATTERN_LENGTH WORD_LENGTH_FIREFLY + UUID_LENGTH + 11 + WORD_LENGTH_STATE //len("/ip-address")
    #define MQTT_TOPIC_IP_ADDRESS_AUTO_DISCOVERY_PATTERN "%s/sensor/FireFly-%s-ip-address/config"     //%s = Home Assistant root topic (defaults to "homeassistant"), %s = Controller UUID

    #define MQTT_TOPIC_ERROR_COUNT_STATE_PATTERN WORD_FIREFLY_SLASH "%s/count-errors/state"      //%s = Controller UUID
    #define MQTT_TOPIC_ERROR_COUNT_STATE_PATTERN_LENGTH WORD_LENGTH_FIREFLY + UUID_LENGTH + 13 + WORD_LENGTH_STATE //len("/count-errors")
    #define MQTT_TOPIC_ERROR_COUNT_AUTO_DISCOVERY_PATTERN "%s/sensor/FireFly-%s-count-errors/config"     //%s = Home Assistant root topic (defaults to "homeassistant"), %s = Controller UUID

    #define MQTT_TOPIC_TEMPERATURE_STATE_PATTERN WORD_FIREFLY_SLASH "%s/temperature/%s/state"        //%s = Controller UUID, %s = location
    #define MQTT_TOPIC_TEMPERATURE_STATE_PATTERN_LENGTH WORD_LENGTH_FIREFLY + UUID_LENGTH + 1 + 12 + WORD_LENGTH_STATE  //len("/temperature")
    #define MQTT_TOPIC_TEMPERATURE_AUTO_DISCOVERY_PATTERN "%s/sensor/FireFly-%s-temperature-%s/config"     //%s = Home Assistant root topic (defaults to "homeassistant"), %s = Controller UUID, %s = location
    #define MQTT_TOPIC_TEMPERATURE_AUTO_DISCOVERY_LENGTH WORD_LENGTH_AUTODISCOVERY_ROOT + 1 + WORD_LENGTH_INTEGRATION + 7 + 1 + UUID_LENGTH + 1 + 11 + 1 + 8 + WORD_LENGTH_CONFIG
    #define MQTT_TEMPERATURE_AUTO_DISCOVERY_UNIQUE_ID_PATTERN "FireFly-%s-temperature-%s"       //%s = Controller UUID, %s = location
    #define MQTT_TEMPERATURE_AUTO_DISCOVERY_UNIQUE_ID_LENGTH 7 + 1 + UUID_LENGTH + 1 + 11 + 1 + 8


    /***************** CONTROLLER FIRMWARE UPDATE TOPICS *****************/

    #define MQTT_TOPIC_UPDATE_STATE_PATTERN WORD_FIREFLY_SLASH "%s/update/state"       //%s = Controller UUID
    #define MQTT_TOPIC_UPDATE_SET_PATTERN WORD_FIREFLY_SLASH "%s/update/set"       //%s = Controller UUID
    #define MQTT_TOPIC_UPDATE_AVAILABILITY_PATTERN WORD_FIREFLY_SLASH "%s/update/availability"     //%s = Controller UUID
    #define MQTT_TOPIC_UPDATE_AUTO_DISCOVERY_PATTERN "%s/update/FireFly-%s/config"     //%s = Home Assistant root topic (defaults to "homeassistant"), %s = Controller UUID


    /***************** CONTROLLER AVAILABILITY TOPICS *****************/

    #define MQTT_TOPIC_CONTROLLER_AVAILABILITY_LENGTH WORD_LENGTH_FIREFLY + UUID_LENGTH + WORD_LENGTH_AVAILABILITY
    #define MQTT_TOPIC_CONTROLLER_AVAILABILITY_PATTERN WORD_FIREFLY_SLASH "%s/availability"    //%s = Controller UUID


    class exPubSubClient : public PubSubClient
    {           

        public:
            using PubSubClient::PubSubClient; /* Inherit the base PubSubClient */
            const char* topic_availability; /* Topic name for availability, which will be used as the last will topic name as well */
            int64_t lastReconnectAttemptTime = 0; /* The time (millis() or equivalent) when the last reconnection was be attempted */
            
            struct autoDiscovery{
                bool sent = false;
                char homeAssistantRoot[WORD_LENGTH_AUTODISCOVERY_ROOT+1] = "homeassistant";
                char deviceName[CONTROLLER_NAME_MAX_LENGTH+1] = "FireFly Controller";
                char suggestedArea[CONTROLLER_AREA_MAX_LENGTH+1];

                /**
                 * Changes the Home Assistant auto discovery root path from the default of "homeassistant" to the value specified
                 * @param value the new root name to use, without a trailing slash
                 */
                void setHomeAssistantRoot(char* value){
                    strcpy(homeAssistantRoot, value);
                }

                void setDeviceName(char* value){
                    strcpy(deviceName, value);
                }

                void setSuggestedArea(char* value){
                    strcpy(suggestedArea, value);
                }
            } autoDiscovery;
    };

#endif
