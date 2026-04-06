/*
    Ethernet Event Helpers for W5500 (ESP32 Arduino core 3.x)

    Provides drop-in replacements for the ESP32_W5500_* callback API that was
    previously bundled in the BrentIO/AsyncWebServer_ESP32_W5500 library.
    The Arduino ESP32 core 3.x ships a native ETHClass (ETH.h) that drives the
    W5500 directly; these helpers wire its WiFiEvent notifications into the same
    connected/disconnected callback pattern used throughout the project.
*/

#ifndef ethernet_h
#define ethernet_h

    #include <WiFi.h>  /* Required for WiFiEvent_t, WiFiEventInfo_t, and WiFi.onEvent() */

    static volatile bool _eth_connected = false;
    static void (*_eth_cb_connected)()    = nullptr;
    static void (*_eth_cb_disconnected)() = nullptr;

    static void _ethEventHandler(WiFiEvent_t event, WiFiEventInfo_t info)
    {
        switch (event)
        {
            case ARDUINO_EVENT_ETH_GOT_IP:
                _eth_connected = true;
                if (_eth_cb_connected) _eth_cb_connected();
                break;

            case ARDUINO_EVENT_ETH_DISCONNECTED:
            case ARDUINO_EVENT_ETH_STOP:
                _eth_connected = false;
                if (_eth_cb_disconnected) _eth_cb_disconnected();
                break;

            default:
                break;
        }
    }

    inline void ESP32_W5500_onEvent()
    {
        WiFi.onEvent(_ethEventHandler);
    }

    inline void ESP32_W5500_setCallback_connected(void (*cb)())    { _eth_cb_connected    = cb; }
    inline void ESP32_W5500_setCallback_disconnected(void (*cb)()) { _eth_cb_disconnected = cb; }
    inline bool ESP32_W5500_isConnected()                          { return _eth_connected; }

#endif
