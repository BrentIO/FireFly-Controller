#ifndef extendedPubSubClient_h
    #define extendedPubSubClient_h

    #include <PubSubClient.h>

    #define WORD_FIREFLY_SLASH "FireFly/"                           //String literal

    #define WORD_LENGTH_SLASH 1                                     //len("/")
    #define WORD_LENGTH_DASH 1                                      //len("-")
    #define WORD_LENGTH_DOT 1                                       //len(".")
    #define WORD_LENGTH_FIREFLY 7                                   //len("FireFly")
    #define WORD_LENGTH_SET 3                                       //len("set")
    #define WORD_LENGTH_STATE 5                                     //len("state")
    #define WORD_LENGTH_AVAILABILITY 12                             //len("availability")
    #define WORD_LENGTH_CONFIG 6                                    //len("config")
    #define WORD_LENGTH_INPUTS 6                                    //len("inputs")
    #define WORD_LENGTH_CIRCUITS 8                                  //len("circuits")
    #define WORD_LENGTH_CHANNELS 8                                  //len("channels")
    #define WORD_LENGTH_INTEGRATION 19                              //The longest MQTT integration allowed in Home Assistant is len("alarm_control_panel"), see https://www.home-assistant.io/integrations/mqtt/
    #define WORD_LENGTH_AUTODISCOVERY_ROOT 13                       //len(this->auto_discovery_root, len("homeassistant"))
    #define WORD_LENGTH_TEMPERATURE 11                              //len("temperature")
    #define WORD_LENGTH_HTTP_DASH_SERVER 11                         //len("http-server")
    #define WORD_LENGTH_IP_DASH_ADDRESS 10                          //len("ip-address")
    #define WORD_LENGTH_TIME_DASH_START 10                          //len("time-start")
    #define WORD_LENGTH_MAC_DASH_ADDRESS 11                         //len("mac-address")
    #define WORD_LENGTH_COUNT_DASH_ERRORS 12                        //len("count-errors")
    #define WORD_LENGTH_UPDATE 6                                    //len("update")
    #define WORD_LENGTH_HEAP_DASH_FREE 9                            //len("heap-free")
    #define WORD_LENGTH_HEAP_DASH_LARGEST_DASH_FREE_DASH_BLOCK 23   //len("heap-largest-free-block")

    #define UUID_LENGTH 36                  //len(uuidv4)
    #define MQTT_USERNAME_MAX_LENGTH 64
    #define MQTT_PASSWORD_MAX_LENGTH 64
    #define CHANNEL_NUMBER_MAX_LENGTH 1


    /***************** I/O TOPICS *****************/

    //Ex: FireFly/inputs/12345678/channels/6/state
    #define MQTT_TOPIC_INPUT_STATE_PATTERN WORD_FIREFLY_SLASH "inputs/%s/channels/%i/state"           //%s = Port ID, %i = channel number       
    #define MQTT_TOPIC_INPUT_STATE_PATTERN_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_SLASH + WORD_LENGTH_INPUTS + WORD_LENGTH_SLASH  + PORT_ID_MAX_LENGTH + WORD_LENGTH_SLASH + WORD_LENGTH_CHANNELS + WORD_LENGTH_SLASH  + CHANNEL_NUMBER_MAX_LENGTH + WORD_LENGTH_SLASH + WORD_LENGTH_STATE
    
    //Ex: FireFly/circuits/12345678/set
    #define MQTT_TOPIC_OUTPUT_SET_PATTERN WORD_FIREFLY_SLASH "circuits/%s/set"              //%s = Output ID
    #define MQTT_TOPIC_OUTPUT_SET_REGEX "^FireFly\/circuits\/([A-Za-z0-9~!@#$%^&*()_+-=|]+)\/set$"
    #define MQTT_TOPIC_OUTPUT_SET_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_SLASH + WORD_LENGTH_CIRCUITS + WORD_LENGTH_SLASH + OUTPUT_ID_MAX_LENGTH + WORD_LENGTH_SLASH + WORD_LENGTH_SET
    
    //Ex: FireFly/circuits/12345678/state
    #define MQTT_TOPIC_OUTPUT_STATE_PATTERN WORD_FIREFLY_SLASH "circuits/%s/state"          //%s = Output ID
    #define MQTT_TOPIC_OUTPUT_STATE_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_SLASH + WORD_LENGTH_CIRCUITS + WORD_LENGTH_SLASH + OUTPUT_ID_MAX_LENGTH + WORD_LENGTH_SLASH + WORD_LENGTH_STATE
    
    //Ex: homeassistant/alarm_control_panel/12345678/config
    #define MQTT_TOPIC_OUTPUT_AUTO_DISCOVERY_PATTERN "%s/%s/%s/config"  //%s = Home Assistant root topic (defaults to "homeassistant"), %s = device platform (light/fan/switch/etc), %s = Output ID
    #define MQTT_TOPIC_OUTPUT_AUTO_DISCOVERY_LENGTH WORD_LENGTH_AUTODISCOVERY_ROOT + WORD_LENGTH_SLASH + WORD_LENGTH_INTEGRATION + WORD_LENGTH_SLASH + OUTPUT_ID_MAX_LENGTH + WORD_LENGTH_SLASH + WORD_LENGTH_CONFIG

    //Ex: alarm_control_panel.FireFly-circuits-12345678
    #define MQTT_OUTPUT_DEFAULT_ENTITY_ID_PATTERN "%s.FireFly-circuits-%s"   //%s = device platform (light/fan/switch/etc), %s = Output ID
    #define MQTT_OUTPUT_DEFAULT_ENTITY_ID_LENGTH WORD_LENGTH_INTEGRATION + WORD_LENGTH_DOT + WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + WORD_LENGTH_CIRCUITS + WORD_LENGTH_DASH + OUTPUT_ID_MAX_LENGTH
    
    //Ex: FireFly-12345678
    #define MQTT_OUTPUT_AUTO_DISCOVERY_UNIQUE_ID_PATTERN "FireFly-%s"       //%s = output ID
    #define MQTT_OUTPUT_AUTO_DISCOVERY_UNIQUE_ID_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH +  OUTPUT_ID_MAX_LENGTH


    /***************** PERIPHERAL TOPICS *****************/

    //Ex: FireFly/00000000-0000-4000-0000-000000000000/time-start/state
    #define MQTT_TOPIC_TIME_START_STATE_PATTERN WORD_FIREFLY_SLASH "%s/time-start/state"     //%s = Controller UUID
    #define MQTT_TOPIC_TIME_START_STATE_PATTERN_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_SLASH + UUID_LENGTH + WORD_LENGTH_SLASH + WORD_LENGTH_TIME_DASH_START + WORD_LENGTH_SLASH + WORD_LENGTH_STATE 

    //Ex: homeassistant/sensor/FireFly-00000000-0000-4000-0000-000000000000-time-start/config
    #define MQTT_TOPIC_TIME_START_AUTO_DISCOVERY_PATTERN "%s/sensor/FireFly-%s-time-start/config"     //%s = Home Assistant root topic (defaults to "homeassistant"), %s = Controller UUID
    #define MQTT_TOPIC_TIME_START_AUTO_DISCOVERY_LENGTH WORD_LENGTH_AUTODISCOVERY_ROOT + WORD_LENGTH_SLASH + WORD_LENGTH_INTEGRATION + WORD_LENGTH_SLASH + WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_DASH + WORD_LENGTH_TIME_DASH_START + WORD_LENGTH_SLASH + WORD_LENGTH_CONFIG
    
    //Ex: FireFly-00000000-0000-4000-0000-000000000000-time-start
    #define MQTT_TIME_START_AUTO_DISCOVERY_UNIQUE_ID_PATTERN "FireFly-%s-time-start"       //%s = Controller UUID
    #define MQTT_TIME_START_AUTO_DISCOVERY_UNIQUE_ID_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_DASH + WORD_LENGTH_TIME_DASH_START

    //Ex: sensor.FireFly-00000000-0000-4000-0000-000000000000-time-start
    #define MQTT_TIME_START_DEFAULT_ENTITY_ID_PATTERN "sensor.FireFly-%s-time-start"       //%s = Controller UUID
    #define MQTT_TIME_START_DEFAULT_ENTITY_ID_LENGTH WORD_LENGTH_INTEGRATION + WORD_LENGTH_DOT + WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_TIME_DASH_START

    //Ex: FireFly/00000000-0000-4000-0000-000000000000/ip-address/state
    #define MQTT_TOPIC_IP_ADDRESS_STATE_PATTERN WORD_FIREFLY_SLASH "%s/ip-address/state"     //%s = Controller UUID
    #define MQTT_TOPIC_IP_ADDRESS_STATE_PATTERN_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_SLASH + UUID_LENGTH + WORD_LENGTH_SLASH + WORD_LENGTH_IP_DASH_ADDRESS + WORD_LENGTH_SLASH + WORD_LENGTH_STATE

    //Ex: homeassistant/sensor/FireFly-00000000-0000-4000-0000-000000000000-ip-address/config
    #define MQTT_TOPIC_IP_ADDRESS_AUTO_DISCOVERY_PATTERN "%s/sensor/FireFly-%s-ip-address/config"     //%s = Home Assistant root topic (defaults to "homeassistant"), %s = Controller UUID
    #define MQTT_TOPIC_IP_ADDRESS_AUTO_DISCOVERY_LENGTH WORD_LENGTH_AUTODISCOVERY_ROOT + WORD_LENGTH_SLASH + WORD_LENGTH_INTEGRATION + WORD_LENGTH_SLASH + WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_DASH + WORD_LENGTH_IP_DASH_ADDRESS + WORD_LENGTH_SLASH + WORD_LENGTH_CONFIG

    //Ex: FireFly-00000000-0000-4000-0000-000000000000-ip-address
    #define MQTT_IP_ADDRESS_AUTO_DISCOVERY_UNIQUE_ID_PATTERN "FireFly-%s-ip-address"       //%s = Controller UUID
    #define MQTT_IP_ADDRESS_AUTO_DISCOVERY_UNIQUE_ID_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_DASH + WORD_LENGTH_IP_DASH_ADDRESS

    //Ex: sensor.FireFly-00000000-0000-4000-0000-000000000000-ip-address
    #define MQTT_IP_ADDRESS_DEFAULT_ENTITY_ID_PATTERN "sensor.FireFly-%s-ip-address"       //%s = Controller UUID
    #define MQTT_IP_ADDRESS_DEFAULT_ENTITY_ID_LENGTH WORD_LENGTH_INTEGRATION + WORD_LENGTH_DOT + WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_IP_DASH_ADDRESS

    //Ex: FireFly/00000000-0000-4000-0000-000000000000/mac-address/state
    #define MQTT_TOPIC_MAC_ADDRESS_STATE_PATTERN WORD_FIREFLY_SLASH "%s/mac-address/state"     //%s = Controller UUID
    #define MQTT_TOPIC_MAC_ADDRESS_STATE_PATTERN_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_SLASH + UUID_LENGTH + WORD_LENGTH_SLASH + WORD_LENGTH_MAC_DASH_ADDRESS + WORD_LENGTH_SLASH + WORD_LENGTH_STATE

    //Ex: homeassistant/sensor/FireFly-00000000-0000-4000-0000-000000000000-mac-address/config
    #define MQTT_TOPIC_MAC_ADDRESS_AUTO_DISCOVERY_PATTERN "%s/sensor/FireFly-%s-mac-address/config"     //%s = Home Assistant root topic (defaults to "homeassistant"), %s = Controller UUID
    #define MQTT_TOPIC_MAC_ADDRESS_AUTO_DISCOVERY_LENGTH WORD_LENGTH_AUTODISCOVERY_ROOT + WORD_LENGTH_SLASH + WORD_LENGTH_INTEGRATION + WORD_LENGTH_SLASH + WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_DASH + WORD_LENGTH_MAC_DASH_ADDRESS + WORD_LENGTH_SLASH + WORD_LENGTH_CONFIG

    //Ex: FireFly-00000000-0000-4000-0000-000000000000-mac-address
    #define MQTT_MAC_ADDRESS_AUTO_DISCOVERY_UNIQUE_ID_PATTERN "FireFly-%s-mac-address"       //%s = Controller UUID
    #define MQTT_MAC_ADDRESS_AUTO_DISCOVERY_UNIQUE_ID_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_DASH + WORD_LENGTH_MAC_DASH_ADDRESS

    //Ex: sensor.FireFly-00000000-0000-4000-0000-000000000000-mac-address
    #define MQTT_MAC_ADDRESS_DEFAULT_ENTITY_ID_PATTERN "sensor.FireFly-%s-mac-address"       //%s = Controller UUID
    #define MQTT_MAC_ADDRESS_DEFAULT_ENTITY_ID_LENGTH WORD_LENGTH_INTEGRATION + WORD_LENGTH_DOT + WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_MAC_DASH_ADDRESS

    //Ex: FireFly/00000000-0000-4000-0000-000000000000/count-errors/state
    #define MQTT_TOPIC_COUNT_ERRORS_STATE_PATTERN WORD_FIREFLY_SLASH "%s/count-errors/state"      //%s = Controller UUID
    #define MQTT_TOPIC_COUNT_ERRORS_STATE_PATTERN_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_SLASH + UUID_LENGTH + WORD_LENGTH_SLASH + WORD_LENGTH_COUNT_DASH_ERRORS + WORD_LENGTH_SLASH + WORD_LENGTH_STATE

    //Ex: homeassistant/sensor/FireFly-00000000-0000-4000-0000-000000000000-count-errors/config
    #define MQTT_TOPIC_COUNT_ERRORS_AUTO_DISCOVERY_PATTERN "%s/sensor/FireFly-%s-count-errors/config"     //%s = Home Assistant root topic (defaults to "homeassistant"), %s = Controller UUID
    #define MQTT_TOPIC_COUNT_ERRORS_AUTO_DISCOVERY_LENGTH WORD_LENGTH_AUTODISCOVERY_ROOT + WORD_LENGTH_SLASH + WORD_LENGTH_INTEGRATION + WORD_LENGTH_SLASH + WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_DASH + WORD_LENGTH_COUNT_DASH_ERRORS + WORD_LENGTH_SLASH + WORD_LENGTH_CONFIG

    //Ex: FireFly-00000000-0000-4000-0000-000000000000-count-errors
    #define MQTT_COUNT_ERRORS_AUTO_DISCOVERY_UNIQUE_ID_PATTERN "FireFly-%s-count-errors"       //%s = Controller UUID
    #define MQTT_COUNT_ERRORS_AUTO_DISCOVERY_UNIQUE_ID_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_DASH + WORD_LENGTH_COUNT_DASH_ERRORS

     //Ex: sensor.FireFly-00000000-0000-4000-0000-000000000000-count-errors
    #define MQTT_COUNT_ERRORS_DEFAULT_ENTITY_ID_PATTERN "sensor.FireFly-%s-count-errors"       //%s = Controller UUID
    #define MQTT_COUNT_ERRORS_DEFAULT_ENTITY_ID_LENGTH WORD_LENGTH_INTEGRATION + WORD_LENGTH_DOT + WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_COUNT_DASH_ERRORS

    //Ex: FireFly/00000000-0000-4000-0000-000000000000/temperature/12345678/state
    #define MQTT_TOPIC_TEMPERATURE_STATE_PATTERN WORD_FIREFLY_SLASH "%s/temperature/%s/state"        //%s = Controller UUID, %s = location
    #define MQTT_TOPIC_TEMPERATURE_STATE_PATTERN_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_SLASH + UUID_LENGTH + WORD_LENGTH_SLASH + WORD_LENGTH_TEMPERATURE + WORD_LENGTH_SLASH + TEMPERATURE_SENSOR_LOCATION_MAX_LENGTH + WORD_LENGTH_SLASH + WORD_LENGTH_STATE

    //Ex: homeassistant/sensor/FireFly-00000000-0000-4000-0000-000000000000-temperature-12345678/config
    #define MQTT_TOPIC_TEMPERATURE_AUTO_DISCOVERY_PATTERN "%s/sensor/FireFly-%s-temperature-%s/config"     //%s = Home Assistant root topic (defaults to "homeassistant"), %s = Controller UUID, %s = location
    #define MQTT_TOPIC_TEMPERATURE_AUTO_DISCOVERY_LENGTH WORD_LENGTH_AUTODISCOVERY_ROOT + WORD_LENGTH_SLASH + WORD_LENGTH_INTEGRATION + WORD_LENGTH_SLASH + WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_DASH + WORD_LENGTH_TEMPERATURE + WORD_LENGTH_DASH + TEMPERATURE_SENSOR_LOCATION_MAX_LENGTH + WORD_LENGTH_SLASH + WORD_LENGTH_CONFIG

    //Ex: FireFly-00000000-0000-4000-0000-000000000000-temperature-12345678
    #define MQTT_TEMPERATURE_AUTO_DISCOVERY_UNIQUE_ID_PATTERN "FireFly-%s-temperature-%s"       //%s = Controller UUID, %s = location
    #define MQTT_TEMPERATURE_AUTO_DISCOVERY_UNIQUE_ID_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_DASH + WORD_LENGTH_TEMPERATURE + WORD_LENGTH_DASH + TEMPERATURE_SENSOR_LOCATION_MAX_LENGTH

    //Ex: alarm_control_panel.FireFly-00000000-0000-4000-0000-000000000000-temperature-12345678
    #define MQTT_TEMPERATURE_DEFAULT_ENTITY_ID_PATTERN "sensor.FireFly-%s-temperature-%s"       //%s = Controller UUID, %s = location
    #define MQTT_TEMPERATURE_DEFAULT_ENTITY_ID_LENGTH WORD_LENGTH_INTEGRATION + WORD_LENGTH_DOT + WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_DASH + WORD_LENGTH_TEMPERATURE + WORD_LENGTH_DASH + TEMPERATURE_SENSOR_LOCATION_MAX_LENGTH

    //Ex: FireFly/00000000-0000-4000-0000-000000000000/http-server/state
    #define MQTT_TOPIC_HTTP_SERVER_STATE_PATTERN WORD_FIREFLY_SLASH "%s/http-server/state"     //%s = Controller UUID
    #define MQTT_TOPIC_HTTP_SERVER_STATE_PATTERN_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_SLASH + UUID_LENGTH + WORD_LENGTH_SLASH + WORD_LENGTH_HTTP_DASH_SERVER + WORD_LENGTH_SLASH + WORD_LENGTH_STATE

    //Ex: homeassistant/switch/FireFly-00000000-0000-4000-0000-000000000000-http-server/config
    #define MQTT_TOPIC_HTTP_SERVER_AUTO_DISCOVERY_PATTERN "%s/switch/FireFly-%s-http-server/config"     //%s = Home Assistant root topic (defaults to "homeassistant"), %s = Controller UUID
    #define MQTT_TOPIC_HTTP_SERVER_AUTO_DISCOVERY_LENGTH WORD_LENGTH_AUTODISCOVERY_ROOT + WORD_LENGTH_SLASH + WORD_LENGTH_INTEGRATION + WORD_LENGTH_SLASH + WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_DASH + WORD_LENGTH_HTTP_DASH_SERVER + WORD_LENGTH_SLASH + WORD_LENGTH_CONFIG

    //Ex: FireFly-00000000-0000-4000-0000-000000000000-http-server
    #define MQTT_HTTP_SERVER_AUTO_DISCOVERY_UNIQUE_ID_PATTERN "FireFly-%s-http-server"       //%s = Controller UUID
    #define MQTT_HTTP_SERVER_AUTO_DISCOVERY_UNIQUE_ID_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_DASH + WORD_LENGTH_HTTP_DASH_SERVER

    //Ex: FireFly/00000000-0000-4000-0000-000000000000/http-server/set
    #define MQTT_TOPIC_HTTP_SERVER_SET_PATTERN WORD_FIREFLY_SLASH "%s/http-server/set"       //%s = Controller UUID
    #define MQTT_TOPIC_HTTP_SERVER_SET_REGEX "^FireFly\/[0-9a-f-]+\/http[-]server\/set$"
    #define MQTT_TOPIC_HTTP_SERVER_SET_PATTERN_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_SLASH + UUID_LENGTH + WORD_LENGTH_SLASH + WORD_LENGTH_HTTP_DASH_SERVER + WORD_LENGTH_DASH + WORD_LENGTH_SET

    //Ex: switch.FireFly-00000000-0000-4000-0000-000000000000-http_server
    #define MQTT_HTTP_SERVER_DEFAULT_ENTITY_ID_PATTERN "switch.FireFly-%s-http_server"       //%s = Controller UUID
    #define MQTT_HTTP_SERVER_DEFAULT_ENTITY_ID_LENGTH WORD_LENGTH_INTEGRATION + WORD_LENGTH_DOT + WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_HTTP_DASH_SERVER


    /***************** FIRMWARE UPDATE TOPICS *****************/

    //Ex: FireFly/00000000-0000-4000-0000-000000000000/update/state
    #define MQTT_TOPIC_UPDATE_STATE_PATTERN WORD_FIREFLY_SLASH "%s/update/state"       //%s = Controller UUID
    #define MQTT_TOPIC_UPDATE_STATE_PATTERN_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_SLASH + UUID_LENGTH + WORD_LENGTH_SLASH + WORD_LENGTH_UPDATE + WORD_LENGTH_SLASH + WORD_LENGTH_STATE

    //Ex: FireFly/00000000-0000-4000-0000-000000000000/update/set
    #define MQTT_TOPIC_UPDATE_SET_PATTERN WORD_FIREFLY_SLASH "%s/update/set"       //%s = Controller UUID
    #define MQTT_TOPIC_UPDATE_SET_REGEX "^FireFly\/[0-9a-f-]+\/update\/set$"
    #define MQTT_TOPIC_UPDATE_SET_PATTERN_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_SLASH + UUID_LENGTH + WORD_LENGTH_SLASH + WORD_LENGTH_UPDATE + WORD_LENGTH_SLASH + WORD_LENGTH_SET

    //Ex: homeassistant/update/FireFly-00000000-0000-4000-0000-000000000000-update/config
    #define MQTT_TOPIC_UPDATE_AUTO_DISCOVERY_PATTERN "%s/update/FireFly-%s-update/config"     //%s = Home Assistant root topic (defaults to "homeassistant"), %s = Controller UUID
    #define MQTT_TOPIC_UPDATE_AUTO_DISCOVERY_PATTERN_LENGTH WORD_LENGTH_AUTODISCOVERY_ROOT + WORD_LENGTH_SLASH + WORD_LENGTH_INTEGRATION + WORD_LENGTH_SLASH + WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_DASH + WORD_LENGTH_UPDATE + WORD_LENGTH_SLASH + WORD_LENGTH_CONFIG

    //Ex: FireFly-00000000-0000-4000-0000-000000000000-update
    #define MQTT_UPDATE_AUTO_DISCOVERY_UNIQUE_ID_PATTERN "FireFly-%s-update"       //%s = Controller UUID
    #define MQTT_UPDATE_AUTO_DISCOVERY_UNIQUE_ID_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_DASH + WORD_LENGTH_UPDATE

    //Ex: update.FireFly-00000000-0000-4000-0000-000000000000-update
    #define MQTT_UPDATE_DEFAULT_ENTITY_ID_PATTERN "update.FireFly-%s-update"       //%s = Controller UUID
    #define MQTT_UPDATE_DEFAULT_ENTITY_ID_LENGTH WORD_LENGTH_INTEGRATION + WORD_LENGTH_DOT + WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_UPDATE


    /*****************  FIRMWARE UPDATE SERVICE AVAILABILITY TOPICS *****************/

    //Ex: FireFly/00000000-0000-4000-0000-000000000000/update/availability
    #define MQTT_TOPIC_UPDATE_AVAILABILITY_PATTERN WORD_FIREFLY_SLASH "%s/update/availability"    //%s = Controller UUID
    #define MQTT_TOPIC_UPDATE_AVAILABILITY_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_SLASH + UUID_LENGTH + WORD_LENGTH_SLASH + WORD_LENGTH_UPDATE + WORD_LENGTH_SLASH + WORD_LENGTH_AVAILABILITY


    /***************** CONTROLLER AVAILABILITY TOPICS *****************/

    //Ex: FireFly/00000000-0000-4000-0000-000000000000/availability
    #define MQTT_TOPIC_CONTROLLER_AVAILABILITY_PATTERN WORD_FIREFLY_SLASH "%s/availability"    //%s = Controller UUID
    #define MQTT_TOPIC_CONTROLLER_AVAILABILITY_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_SLASH + UUID_LENGTH + WORD_LENGTH_SLASH + WORD_LENGTH_AVAILABILITY


    /***************** MEMORY MANAGEMENT TOPICS *****************/

    //Ex: FireFly/00000000-0000-4000-0000-000000000000/heap-free/state
    #define MQTT_TOPIC_HEAP_FREE_STATE_PATTERN WORD_FIREFLY_SLASH "%s/heap-free/state"       //%s = Controller UUID
    #define MQTT_TOPIC_HEAP_FREE_STATE_PATTERN_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_SLASH + UUID_LENGTH + WORD_LENGTH_SLASH + WORD_LENGTH_HEAP_DASH_FREE + WORD_LENGTH_SLASH + WORD_LENGTH_STATE

    //Ex: homeassistant/sensor/FireFly-00000000-0000-4000-0000-000000000000-heap-free/config
    #define MQTT_TOPIC_HEAP_FREE_AUTO_DISCOVERY_PATTERN "%s/sensor/FireFly-%s-heap-free/config"     //%s = Home Assistant root topic (defaults to "homeassistant"), %s = Controller UUID
    #define MQTT_TOPIC_HEAP_FREE_AUTO_DISCOVERY_LENGTH WORD_LENGTH_AUTODISCOVERY_ROOT + WORD_LENGTH_SLASH + WORD_LENGTH_INTEGRATION + WORD_LENGTH_SLASH + WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_DASH + WORD_LENGTH_HEAP_DASH_FREE + WORD_LENGTH_SLASH + WORD_LENGTH_CONFIG

    //Ex: FireFly-00000000-0000-4000-0000-000000000000-heap-free
    #define MQTT_HEAP_FREE_AUTO_DISCOVERY_UNIQUE_ID_PATTERN "FireFly-%s-heap-free"       //%s = Controller UUID
    #define MQTT_HEAP_FREE_AUTO_DISCOVERY_UNIQUE_ID_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_DASH + WORD_LENGTH_HEAP_DASH_FREE

    //Ex: sensor.FireFly-00000000-0000-4000-0000-000000000000-heap-free
    #define MQTT_HEAP_FREE_DEFAULT_ENTITY_ID_PATTERN "sensor.FireFly-%s-heap-free"       //%s = Controller UUID
    #define MQTT_HEAP_FREE_DEFAULT_ENTITY_ID_LENGTH WORD_LENGTH_INTEGRATION + WORD_LENGTH_DOT + WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_HEAP_DASH_FREE

    //Ex: FireFly/00000000-0000-4000-0000-000000000000/heap-largest-block/state
    #define MQTT_TOPIC_HEAP_LARGEST_FREE_BLOCK_STATE_PATTERN WORD_FIREFLY_SLASH "%s/heap-largest-free-block/state"       //%s = Controller UUID
    #define MQTT_TOPIC_HEAP_LARGEST_FREE_BLOCK_STATE_PATTERN_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_SLASH + UUID_LENGTH + WORD_LENGTH_SLASH + WORD_LENGTH_HEAP_DASH_LARGEST_DASH_FREE_DASH_BLOCK + WORD_LENGTH_SLASH + WORD_LENGTH_STATE

    //Ex: homeassistant/sensor/FireFly-00000000-0000-4000-0000-000000000000-heap-largest-block/config
    #define MQTT_TOPIC_HEAP_LARGEST_FREE_BLOCK_AUTO_DISCOVERY_PATTERN "%s/sensor/FireFly-%s-heap-largest-free-block/config"     //%s = Home Assistant root topic (defaults to "homeassistant"), %s = Controller UUID
    #define MQTT_TOPIC_HEAP_LARGEST_FREE_BLOCK_AUTO_DISCOVERY_LENGTH WORD_LENGTH_AUTODISCOVERY_ROOT + WORD_LENGTH_SLASH + WORD_LENGTH_INTEGRATION + WORD_LENGTH_SLASH + WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_DASH + WORD_LENGTH_HEAP_DASH_LARGEST_DASH_FREE_DASH_BLOCK + WORD_LENGTH_SLASH + WORD_LENGTH_CONFIG

    //Ex: FireFly-00000000-0000-4000-0000-000000000000-heap-largest-block
    #define MQTT_HEAP_LARGEST_FREE_BLOCK_AUTO_DISCOVERY_UNIQUE_ID_PATTERN "FireFly-%s-heap-largest-free-block"       //%s = Controller UUID
    #define MQTT_HEAP_LARGEST_FREE_BLOCK_AUTO_DISCOVERY_UNIQUE_ID_LENGTH WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_DASH + WORD_LENGTH_HEAP_DASH_LARGEST_DASH_FREE_DASH_BLOCK

    //Ex: sensor.FireFly-00000000-0000-4000-0000-000000000000-heap-largest-free-block
    #define MQTT_HEAP_LARGEST_FREE_BLOCK_DEFAULT_ENTITY_ID_PATTERN "sensor.FireFly-%s-heap-largest-free-block"       //%s = Controller UUID
    #define MQTT_HEAP_LARGEST_FREE_BLOCK_DEFAULT_ENTITY_ID_LENGTH WORD_LENGTH_INTEGRATION + WORD_LENGTH_DOT + WORD_LENGTH_FIREFLY + WORD_LENGTH_DASH + UUID_LENGTH + WORD_LENGTH_HEAP_DASH_LARGEST_DASH_FREE_DASH_BLOCK


    class exPubSubClient : public PubSubClient
    {           

        public:
            using PubSubClient::PubSubClient; /* Inherit the base PubSubClient */
            char topic_availability[MQTT_TOPIC_CONTROLLER_AVAILABILITY_LENGTH + 1]; /* Topic name for availability, which will be used as the last will topic name as well */
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
                char* topic_to_add = strdup(topic);
                this->subscriptions.add(topic_to_add);
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
