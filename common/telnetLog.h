#ifndef telnetLog_h
    #define telnetLog_h

    #if CORE_DEBUG_LEVEL > 0

        #include <WiFiServer.h>
        #include <WiFiClient.h>
        #include <HardwareSerial.h>
        #include <freertos/portmacro.h>

        // Must be declared before TelnetLog so interceptPrintf can call it.
        // Defined here, before #define Serial takes effect, so it always returns
        // the real HardwareSerial regardless of the #define at the bottom.
        inline HardwareSerial& _hw() { return Serial; }

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

            // Shared write path used by both interceptPrintf and TelnetSerial::write.
            // Fills the ring buffer and streams to live clients not currently replaying.
            static void pipe(const uint8_t* buf, size_t n) {
                if (!_instance) return;
                _instance->_writeToRing((const char*)buf, n);
                for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
                    if (_instance->_clients[i] && !_instance->_replaying[i]) {
                        _instance->_clients[i].write(buf, n);
                    }
                }
            }

            // Called by the log_printf macro below to intercept log_d/log_i/log_w/log_e/log_v.
            // Uses _hw() directly for UART so it is independent of the #define Serial below —
            // log_printf can be a function in the Arduino core's own translation unit, so
            // relying on #define Serial alone is not sufficient to intercept it.
            static void interceptPrintf(const char* fmt, ...) {
                char buf[BUF_SIZE];
                va_list args;
                va_start(args, fmt);
                int len = vsnprintf(buf, sizeof(buf), fmt, args);
                va_end(args);
                if (len <= 0) return;
                size_t n = (size_t)len < BUF_SIZE ? (size_t)len : BUF_SIZE - 1;
                _hw().write((const uint8_t*)buf, n);
                pipe((const uint8_t*)buf, n);
            }

            void begin() {
                _instance = this;
                _server.begin();
                _hw().printf("[TelnetLog] begin\r\n");
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
                            _hw().printf("[TelnetLog] client %u accepted bw=%u\r\n",
                                         i, (unsigned)_bytesWritten);
                            _replayHistory(i);
                            placed = true;
                            break;
                        }
                    }
                    if (!placed) {
                        newClient.stop();
                        _hw().printf("[TelnetLog] rejected (no slot)\r\n");
                    }
                }
                for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
                    if (_clients[i] && !_clients[i].connected()) {
                        _hw().printf("[TelnetLog] stopping client %u\r\n", i);
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

        // Thin Serial wrapper: all write() calls go to both the hardware UART (via _hw())
        // and, when TelnetLog is active, the ring buffer and live clients (via pipe()).
        // printf/print/println are inherited from Print and route through write() automatically.
        // This catches all direct Serial.print/printf/write calls in application code.
        struct TelnetSerial : public Print {
            void begin(unsigned long baud, uint32_t config = SERIAL_8N1) { _hw().begin(baud, config); }
            void end()       { _hw().end(); }
            int  available() { return _hw().available(); }
            int  peek()      { return _hw().peek(); }
            int  read()      { return _hw().read(); }
            void flush()     { _hw().flush(); }
            operator bool()  { return (bool)_hw(); }

            size_t write(uint8_t c) override           { return write(&c, 1); }
            size_t write(const uint8_t* buf, size_t n) override {
                _hw().write(buf, n);
                TelnetLog::pipe(buf, n);
                return n;
            }
        };

        inline TelnetSerial _telnetSerial;

        // Replace Serial with TelnetSerial so all explicit Serial calls in application
        // code flow through TelnetSerial::write(), reaching both UART and telnet.
        #define Serial _telnetSerial

        // Intercept Arduino log macros (log_d/log_i/log_w/log_e/log_v), which all expand
        // through log_printf. Must be an explicit macro override — if log_printf is a
        // function in the Arduino core's translation unit, #define Serial alone cannot
        // reach it. This guarantees interception regardless of how the Arduino core
        // defines log_printf.
        #undef log_printf
        #define log_printf(fmt, ...) TelnetLog::interceptPrintf(fmt, ##__VA_ARGS__)

    #endif /* CORE_DEBUG_LEVEL > 0 */

#endif /* telnetLog_h */
