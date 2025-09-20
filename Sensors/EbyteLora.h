#ifndef _EBYTE_LORA_H
#define _EBYTE_LORA_H

#include "../PicoHardware/uart.h"

class EbyteLora {

    UART* _uart;
    uint _m0Pin;
    uint _m1Pin;
    uint _auxPin;

    public:

    enum Mode {
        NORMAL = 0,
        WAKE_UP,
        POWER_SAVING,
        SLEEP
    };

    class Version{
        uint8_t ver[4];
        public:
            // From datasheet:
            // 32 here means the frequency is 433MHZ, 38 means frequency is 470MHz, 45 means frequency is;
            // 868MHz, 44 means the frequency is 915 MHz, 46 means the frequency is 170MHz
            uint frequency() const { 
                uint f = 0; 
                switch(ver[1]) {
                    case 32: f = 433; break;
                    case 38: f = 470; break;
                    case 45: f = 868; break;
                    case 44: f = 915; break;
                    case 46: f = 170; break;
                }
                return f;
            }
            uint8_t version() const { return ver[2];}
            uint8_t features() const { return ver[3];}
            Version(const uint8_t* data){
                ver[0] = data[0];
                ver[1] = data[1];
                ver[2] = data[2];
                ver[3] = data[3];
           }
    

    };

    class Config {
        uint8_t params[6];
        public:
        enum Parity { NONE, ODD, EVEN};
        enum UartBaud { BAUD_1200, BAUD_2400, BAUD_4800, BAUD_9600, BAUD_19200, BAUD_38400, BAUD_57600, BAUD_115200};
        enum AirBps {BPS_300, BPS_1200, BPS_2400, BPS_4800, BPS_9600, BPS_19200};
        enum WakeupTime {WAKE_250, WAKE_500, WAKE_750, WAKE_1000, WAKE_1250, WAKE_1500, WAKE_1750, WAKE_2000};
        enum Power{ POWER_30DBM, POWER_27DBM, POWER_24DBM, POWER_21DBM };
        
        void setAddr(uint16_t addr){ params[1] = (addr&0xFF00)>>8; params[2] = addr&0x00FF;}
        void setParity(Config::Parity parity)   {params[3] = (params[3] & ~0xC0) | (int(parity) << 6);}
        void setUartBaud(Config::UartBaud baud) {params[3] = (params[3] & ~0x38) | (int(baud) << 3);}
        void setAirBps(Config::AirBps bps)      {params[3] = (params[3] & ~0x07) | int(bps);}
        uint setChannel(uint channel) {params[4] = channel & 0x1F; return 410 + (channel & 0x1F);}
        void setFixedTransmission(bool fixed = true){ params[5] = (params[5] & ~0x80) | (fixed ? (1<<7) : 0);}
        void enablePullups(bool on = true) {params[5] = (params[5] & ~0x40) | (on ? (1<<6) : 0 );}
        void setWakeUpTime(Config::WakeUpTime wut) {params[5] = (params[5] & ~0x38) | (int(wut) << 3);}
        void enableFEC(bool on) {params[5] = (params[5] & ~0x04) | (on ? (1<<2) : 0 );}
        void setTxPower(Config::Power power) {params[5] = (params[5] & ~0x03) | int(power);}

        uint16_t addr(uint16_t addr) const {  uint16_t(params[1]) << 8 | params[2];}
        Config::Parity parity() const      { return static_cast<Parity>((params[3] & 0xC0)>>6);} 
        Config::UartBaud uartBaud() const  { return static_cast<UartBaud>((params[3] & 0x38)>>3);} 
        Config::AirBps airBps() const      { return static_cast<AirBps>(params[3] & 0x07);}
        uint channel() const               { return 410 + (params[4] & 0x1F); }
        bool fixedTransmission() const     { return (params[5] & 0x80) != 0;}
        bool pullupsEnabled() const        { return (params[5] & 0x40) != 0;}
        Config::WakeUpTime wakeUpTime()const { return static_cast<WakeUpTime>((params[5] & 0x38)>>3);}
        bool fecEnabled() const            { return (params[5] & 0x04) != 0;}
        Config::Power txPower() const      { return static_cast<Power>(params[5] & 0x03);}

        Config(){
            params[0] = 0; // C0 or C2 will be sent
            params[1] = 0;
            params[2] = 0;
            params[3] = 0b00011010; // 8N1, 9600baud uart, 2400bps over air.
            params[4] = 0x17h       // 433MHz
            params[5] = 0b01000100; // transparent, pullups, 250mS wakeup, FEC on, 30dBm tx power
        }

        Config(const uint8_t* data){
            params[0] = data[0];
            params[1] = data[1];
            params[2] = data[2];
            params[3] = data[3];
            params[4] = data[4];
            params[5] = data[5];
        }


        const uint8_t* data() {return params;}
    };

    EbyteLora(UART* uart, uint m0Pin, uint m1Pin, uint auxPin);

    bool isBusy();

    Mode getMode();
    void setMode(Mode mode);

    void configure(EbyteLora::Config& config);
    void configureAndSave(EbyteLora::Config& config);
    EbyteLora::Config getConfig();
    EbyteLora::Version getVersion();
    reset();

    void write(const uint8_t* src,size_t len);
    void read(uint8_t* dst, size_t len)

    private:
       void configure(EbyteLora::Config& config, uint8_t cmd);
       void setUartFromConfig(EbyteLora::Config& config);

};

#endif