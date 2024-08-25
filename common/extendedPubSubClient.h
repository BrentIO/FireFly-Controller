#ifndef extendedPubSubClient_h
    #define extendedPubSubClient_h

    #include <PubSubClient.h>

    /***************** CONTROLLER AVAILABILITY TOPICS *****************/

    #define MQTT_TOPIC_CONTROLLER_AVAILABILITY_LENGTH WORD_LENGTH_FIREFLY + UUID_LENGTH + WORD_LENGTH_AVAILABILITY
    #define MQTT_TOPIC_CONTROLLER_AVAILABILITY_PATTERN WORD_FIREFLY_SLASH "%s/availability"    //%s = Controller UUID


    class exPubSubClient : public PubSubClient
    {           

        public:
            using PubSubClient::PubSubClient; /* Inherit the base PubSubClient */
            const char* topic_availability; /* Topic name for availability, which will be used as the last will topic name as well */
            int64_t lastReconnectAttemptTime = 0; /* The time (millis() or equivalent) when the last reconnection was be attempted */
    };

#endif
