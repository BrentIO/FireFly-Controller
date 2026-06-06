#ifndef telnetLog_h
    #define telnetLog_h

    #if CORE_DEBUG_LEVEL > 0

        #include <WiFiServer.h>
        #include <WiFiClient.h>
        #include <HardwareSerial.h>
        #include <freertos/portmacro.h>

        class TelnetLog {

            static constexpr uint16_t PORT        = 23;
            static constexpr uint8_t  MAX_CLIENTS = 3;
            static constexpr size_t   BUF_SIZE    = 512;
            static constexpr size_t   RING_SIZE   = 16384;

            WiFiServer      _server{PORT};
            WiFiClient      _clients[MAX_CLIENTS];
            bool            _replaying[MAX_CLIENTS] = {};

            char                _ringBuf[RING_SIZE];
            volatile uint32_t   _bytesWritten = 0;
            portMUX_TYPE        _mux = portMUX_INITIALIZER_UNLOCKED;

            static TelnetLog* _instance;

            void _writeToRing(const char* buf, size_t len) {
                portENTER_CRITICAL(&_mux);
                for (size_t i = 0; i < len; i++) {
                    _ringBuf[_bytesWritten % RING_SIZE] = buf[i];
                    _bytesWritten++;
                }
                portEXIT_CRITICAL(&_mux);
            }

            void _replayHistory(uint8_t i) {
                _replaying[i] = true;
                uint32_t avail = _bytesWritten;
                uint32_t pos   = avail > (uint32_t)RING_SIZE ? avail - (uint32_t)RING_SIZE : 0;
                while (pos < avail) {
                    size_t idx    = pos % RING_SIZE;
                    size_t toEnd  = RING_SIZE - idx;
                    size_t remain = (size_t)(avail - pos);
                    size_t chunk  = remain < toEnd ? remain : toEnd;
                    size_t sent   = _clients[i].write((const uint8_t*)(_ringBuf + idx), chunk);
                    pos += (uint32_t)sent;
                    if (sent < chunk) break;
                }
                _replaying[i] = false;
            }

        public:

            // Intercepts log_d/log_i/log_w/log_e/log_v via the log_printf macro below.
            // Serial here is the real HardwareSerial — this class body is compiled before
            // #define log_printf takes effect, and there is no #define Serial in this file,
            // so Serial.write() goes directly to the hardware UART.
            static void interceptPrintf(const char* fmt, ...) {
                char buf[BUF_SIZE];
                va_list args;
                va_start(args, fmt);
                int len = vsnprintf(buf, sizeof(buf), fmt, args);
                va_end(args);
                if (len <= 0) return;
                size_t n = (size_t)len < BUF_SIZE ? (size_t)len : BUF_SIZE - 1;
                Serial.write((const uint8_t*)buf, n);
                if (_instance) {
                    _instance->_writeToRing(buf, n);
                    for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
                        if (_instance->_clients[i] && !_instance->_replaying[i]) {
                            _instance->_clients[i].write((const uint8_t*)buf, n);
                        }
                    }
                }
            }

            void begin() {
                _instance = this;
                _server.begin();
                Serial.printf("[TelnetLog] begin\r\n");
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
                            Serial.printf("[TelnetLog] client %u accepted bw=%u\r\n",
                                         i, (unsigned)_bytesWritten);
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
            }

            void end() {
                for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
                    if (_clients[i]) _clients[i].stop();
                }
                _server.end();
                _instance = nullptr;
            }
        };

        inline TelnetLog* TelnetLog::_instance = nullptr;

        // Intercept Arduino log macros (log_d/log_i/log_w/log_e/log_v). All of them expand
        // through log_printf. Redefining it here guarantees interception in every translation
        // unit that includes this header, regardless of whether log_printf is a macro or a
        // function in the Arduino core.
        #undef log_printf
        #define log_printf(fmt, ...) TelnetLog::interceptPrintf(fmt, ##__VA_ARGS__)

    #endif /* CORE_DEBUG_LEVEL > 0 */

#endif /* telnetLog_h */
