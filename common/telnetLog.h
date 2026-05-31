#ifndef telnetLog_h
    #define telnetLog_h

    #if CORE_DEBUG_LEVEL > 0

        #include <WiFiServer.h>
        #include <WiFiClient.h>
        #include <esp_log.h>

        class TelnetLog {

            static constexpr uint16_t PORT        = 23;
            static constexpr uint8_t  MAX_CLIENTS = 3;
            static constexpr size_t   BUF_SIZE    = 512;

            WiFiServer      _server{PORT};
            WiFiClient      _clients[MAX_CLIENTS];
            vprintf_like_t  _savedVprintf = nullptr;

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
                    vsnprintf(buf, sizeof(buf), fmt, args_copy);
                    va_end(args_copy);

                    size_t len = strlen(buf);
                    for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
                        if (_instance->_clients[i] && _instance->_clients[i].connected()) {
                            _instance->_clients[i].write((const uint8_t*)buf, len);
                        }
                    }
                }

                return ret;
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
                            placed = true;
                            break;
                        }
                    }
                    if (!placed) {
                        newClient.stop();
                    }
                }

                for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
                    if (_clients[i] && !_clients[i].connected()) {
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
