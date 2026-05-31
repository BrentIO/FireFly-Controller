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
            portMUX_TYPE        _mux = portMUX_INITIALIZER_UNLOCKED;
            uint32_t            _lastHeartbeat = 0;

            static TelnetLog* _instance;

            static int _vprintf_cb(const char* fmt, va_list args) {

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

                        // Bool operator (not connected()) so a failed _replayHistory write
                        // that internally stops WiFiClient does not block live events.
                        for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
                            if (_instance->_clients[i]) {
                                size_t sent = _instance->_clients[i].write((const uint8_t*)buf, n);
                                if (sent == 0) {
                                    Serial.printf("[TelnetLog] vprintf write=0 client=%u bw=%u\r\n",
                                                  i, (unsigned)_instance->_bytesWritten);
                                }
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
                uint32_t sent_total = 0;
                while (pos < avail) {
                    size_t idx    = pos % RING_SIZE;
                    size_t toEnd  = RING_SIZE - idx;
                    size_t remain = (size_t)(avail - pos);
                    size_t chunk  = remain < toEnd ? remain : toEnd;
                    size_t sent   = _clients[i].write((const uint8_t*)(_ringBuf + idx), chunk);
                    pos += (uint32_t)sent;
                    sent_total += (uint32_t)sent;
                    if (sent < chunk) {
                        Serial.printf("[TelnetLog] replay partial sent=%u chunk=%u total=%u avail=%u\r\n",
                                      (unsigned)sent, (unsigned)chunk, (unsigned)sent_total, (unsigned)avail);
                        break;
                    }
                }
                Serial.printf("[TelnetLog] replay done sent=%u avail=%u client_bool=%d\r\n",
                              (unsigned)sent_total, (unsigned)avail, (bool)_clients[i] ? 1 : 0);
            }

        public:

            void begin() {
                _instance = this;
                _savedVprintf = esp_log_set_vprintf(_vprintf_cb);
                _server.begin();
                Serial.printf("[TelnetLog] begin bw=%u\r\n", (unsigned)_bytesWritten);
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
                            uint32_t avail = _bytesWritten;
                            Serial.printf("[TelnetLog] client %u accepted bw=%u bool=%d conn=%d\r\n",
                                         i, (unsigned)avail,
                                         (bool)_clients[i] ? 1 : 0,
                                         _clients[i].connected() ? 1 : 0);
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

                // Heartbeat: write directly from loop() every 5 s to prove the
                // write path works independently of _vprintf_cb.
                uint32_t now = millis();
                if (now - _lastHeartbeat >= 5000) {
                    _lastHeartbeat = now;
                    char hb[64];
                    int hlen = snprintf(hb, sizeof(hb), "\r\n[TelnetLog] hb bw=%u\r\n",
                                        (unsigned)_bytesWritten);
                    for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
                        if (_clients[i]) {
                            size_t sent = _clients[i].write((const uint8_t*)hb, (size_t)hlen);
                            Serial.printf("[TelnetLog] hb client=%u sent=%u bw=%u bool=%d conn=%d\r\n",
                                         i, (unsigned)sent, (unsigned)_bytesWritten,
                                         (bool)_clients[i] ? 1 : 0,
                                         _clients[i].connected() ? 1 : 0);
                        }
                    }
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
