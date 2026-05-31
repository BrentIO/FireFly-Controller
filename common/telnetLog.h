#ifndef telnetLog_h
    #define telnetLog_h

    #if CORE_DEBUG_LEVEL > 0

        #include <WiFiServer.h>
        #include <WiFiClient.h>
        #include <esp_log.h>
        #include <freertos/portmacro.h>

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
            uint32_t            _clientPos[MAX_CLIENTS] = {};
            portMUX_TYPE        _mux = portMUX_INITIALIZER_UNLOCKED;

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
                        _instance->_writeToRing(buf, (size_t)len < BUF_SIZE ? (size_t)len : BUF_SIZE - 1);
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

            void _sendPending(uint8_t i) {
                uint32_t avail = _bytesWritten;
                if (avail - _clientPos[i] > RING_SIZE) {
                    _clientPos[i] = avail - (uint32_t)RING_SIZE;
                }
                while (_clientPos[i] < avail) {
                    size_t idx     = _clientPos[i] % RING_SIZE;
                    size_t toEnd   = RING_SIZE - idx;
                    size_t remain  = (size_t)(avail - _clientPos[i]);
                    size_t chunk   = remain < toEnd ? remain : toEnd;
                    size_t sent    = _clients[i].write((const uint8_t*)(_ringBuf + idx), chunk);
                    _clientPos[i] += (uint32_t)sent;
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
            }

            void loop() {

                WiFiClient newClient = _server.available();

                if (newClient) {
                    bool placed = false;
                    for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
                        if (!_clients[i] || !_clients[i].connected()) {
                            _clients[i] = newClient;
                            _clients[i].print("\r\nFireFly Controller debug log\r\n");
                            uint32_t avail = _bytesWritten;
                            _clientPos[i]  = avail > (uint32_t)RING_SIZE ? avail - (uint32_t)RING_SIZE : 0;
                            placed = true;
                            break;
                        }
                    }
                    if (!placed) {
                        newClient.stop();
                    }
                }

                for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
                    if (_clients[i] && _clients[i].connected()) {
                        _sendPending(i);
                    } else if (_clients[i] && !_clients[i].connected()) {
                        _clients[i].stop();
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
