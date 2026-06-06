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

            // Called from TelnetSerial::write() on every Serial write after begin().
            // Fills the ring buffer and live-streams to connected clients.
            static void pipe(const uint8_t* buf, size_t n) {
                if (!_instance) return;
                _instance->_writeToRing((const char*)buf, n);
                for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
                    if (_instance->_clients[i] && !_instance->_replaying[i]) {
                        _instance->_clients[i].write(buf, n);
                    }
                }
            }

            void begin() {
                _instance = this;
                _server.begin();
                // Serial here is the real HardwareSerial — class body is compiled before
                // #define Serial _telnetSerial takes effect below, so this is UART-only.
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

        // Capture the real HardwareSerial before #define Serial takes effect below.
        inline HardwareSerial& _hw() { return Serial; }

        // Thin Serial wrapper: all write() calls go to both the hardware UART and, when
        // TelnetLog is active, the ring buffer and live telnet clients. printf/print/println
        // are inherited from Print and route through write() automatically.
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

        // Replace Serial with TelnetSerial. _hw() above captures the real HardwareSerial
        // first. All code compiled after this point — including log_d/log_i/log_w/log_e
        // (which expand through Serial.printf) and all explicit Serial calls — flow through
        // TelnetSerial::write(), reaching both UART and telnet.
        #define Serial _telnetSerial

    #endif /* CORE_DEBUG_LEVEL > 0 */

#endif /* telnetLog_h */
