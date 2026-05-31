#ifndef telnetLog_h
    #define telnetLog_h

    #if CORE_DEBUG_LEVEL > 0

        #include <WiFiServer.h>
        #include <WiFiClient.h>
        #include <esp_log.h>
        #include <freertos/portmacro.h>
        #include <HardwareSerial.h>

        class TelnetLog {

            static constexpr uint16_t PORT        = 23;
            static constexpr uint8_t  MAX_CLIENTS = 3;
            static constexpr size_t   BUF_SIZE    = 512;
            static constexpr size_t   RING_SIZE   = 16384;

            WiFiServer      _server{PORT};
            WiFiClient      _clients[MAX_CLIENTS];
            vprintf_like_t  _savedVprintf = nullptr;

            char                _ringBuf[RING_SIZE];
            volatile uint32_t   _bytesWritten = 0;
            volatile uint32_t   _cbCount      = 0;   // incremented at very top of _vprintf_cb
            portMUX_TYPE        _mux = portMUX_INITIALIZER_UNLOCKED;
            uint32_t            _lastHeartbeat = 0;

            static TelnetLog* _instance;

            static int _vprintf_cb(const char* fmt, va_list args) {

                // Increment unconditionally — before any null checks — so we can
                // tell whether ESP-IDF is calling this hook at all.
                if (_instance) {
                    _instance->_cbCount++;
                }

                int ret = 0;

                if (_instance) {
                    va_list args_copy;
                    va_copy(args_copy, args);

                    if (_instance->_savedVprintf) {
                        ret = _instance->_savedVprintf(fmt, args);
                    }

                    char buf[BUF_SIZE];
                    int len = vsnprintf(buf, sizeof(buf), fmt, args_copy);
                    va_end(args_copy);

                    if (len > 0) {
                        size_t n = (size_t)len < BUF_SIZE ? (size_t)len : BUF_SIZE - 1;
                        _instance->_writeToRing(buf, n);

                        for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
                            if (_instance->_clients[i]) {
                                _instance->_clients[i].write((const uint8_t*)buf, n);
                            }
                        }
                    }
                }

                return ret;
            }

            void _writeToRing(const char* buf, size_t len) {
                portENTER_CRITICAL(&_mux);
                for (size_t i = 0; i < len; i++) {
                    _ringBuf[_bytesWritten % RING_SIZE] = buf[i];
                    _bytesWritten++;
                }
                portEXIT_CRITICAL(&_mux);
            }

            void _replayHistory(uint8_t i) {
                uint32_t avail = _bytesWritten;
                uint32_t pos   = avail > (uint32_t)RING_SIZE ? avail - (uint32_t)RING_SIZE : 0;
                while (pos < avail) {
                    size_t idx    = pos % RING_SIZE;
                    size_t toEnd  = RING_SIZE - idx;
                    size_t remain = (size_t)(avail - pos);
                    size_t chunk  = remain < toEnd ? remain : toEnd;
                    size_t sent   = _clients[i].write((const uint8_t*)(_ringBuf + idx), chunk);
                    pos += (uint32_t)sent;
                    if (sent < chunk) {
                        break;
                    }
                }
            }

        public:

            void begin() {
                _instance = this;
                _savedVprintf = esp_log_set_vprintf(_vprintf_cb);
                _server.begin();
                // savedVprintf=0 means no previous handler was registered (or the call failed).
                // If cbCount stays 0 in heartbeats, ESP-IDF is not calling _vprintf_cb at all.
                Serial.printf("[TelnetLog] begin savedVprintf=%p\r\n", (void*)_savedVprintf);
            }

            void loop() {

                WiFiClient newClient = _server.available();

                if (newClient) {
                    bool placed = false;
                    for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
                        if (!_clients[i] || !_clients[i].connected()) {
                            _clients[i] = newClient;
                            _clients[i].setNoDelay(true);
                            _clients[i].print("\r\nFireFly Controller debug log\r\n");
                            Serial.printf("[TelnetLog] client %u accepted bw=%u cb=%u\r\n",
                                         i, (unsigned)_bytesWritten, (unsigned)_cbCount);
                            _replayHistory(i);
                            placed = true;
                            break;
                        }
                    }
                    if (!placed) {
                        newClient.stop();
                        Serial.printf("[TelnetLog] rejected (no slot)\r\n");
                    }
                }

                for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
                    if (_clients[i] && !_clients[i].connected()) {
                        Serial.printf("[TelnetLog] stopping client %u\r\n", i);
                        _clients[i].stop();
                    }
                }

                uint32_t now = millis();
                if (now - _lastHeartbeat >= 5000) {
                    _lastHeartbeat = now;
                    char hb[80];
                    int hlen = snprintf(hb, sizeof(hb),
                                        "\r\n[TelnetLog] hb bw=%u cb=%u\r\n",
                                        (unsigned)_bytesWritten, (unsigned)_cbCount);
                    for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
                        if (_clients[i]) {
                            _clients[i].write((const uint8_t*)hb, (size_t)hlen);
                        }
                    }
                    Serial.printf("[TelnetLog] hb bw=%u cb=%u\r\n",
                                  (unsigned)_bytesWritten, (unsigned)_cbCount);
                }
            }

            void end() {
                if (_savedVprintf) {
                    esp_log_set_vprintf(_savedVprintf);
                    _savedVprintf = nullptr;
                }
                for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
                    if (_clients[i]) {
                        _clients[i].stop();
                    }
                }
                _server.end();
                _instance = nullptr;
            }
        };

        inline TelnetLog* TelnetLog::_instance = nullptr;

    #endif /* CORE_DEBUG_LEVEL > 0 */

#endif /* telnetLog_h */
