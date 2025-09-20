
#include "EbyteLora.h"
#include "hardware/gpio.h"

EbyteLora::EbyteLora(UART* uart, uint m0Pin, uint m1Pin, uint auxPin)
:_uart(uart)
,_m0Pin(m0Pin)
,_m1Pin(m1Pin)
,_auxPin(auxPin)
{
    gpio_init(m0Pin);
    gpio_init(m1Pin);
    gpio_init(auxPin);

    gpio_set_dir(m0Pin, true);
    gpio_set_dir(m1Pin, true);

    gpio_put(m0Pin, 1);
    gpio_put(m1Pin, 1);

    gpio_pull_up(auxPin); // in case module driving with O/C output.
}

bool EbyteLora::isBusy(){
    return !gpio_get(_auxPin); // aux is low when module busy.
}

EbyteLora::Mode EbyteLora::getMode(){
    bool m0 = gpio_get_out_level(_m0Pin);
    bool m1 = gpio_get_out_level(_m1Pin);
    int val = (m1 ? 2 : 0) + (m0 ? 1 : 0);
    return static_cast<EbyteLora::Mode>(val);
}

void EbyteLora::setMode(EbyteLora::Mode mode){
    int val = int(mode);
    gpio_put(m0Pin, val&1 != 0);
    gpio_put(m1Pin, val&2 != 0);

    _serial.waitUntilSent();

    // Then wait for Lora to finish processing it.
    while(isBusy()) sleep_ms(1);
    sleep_ms(2);
}

static uint8_t* toHex(uint8_t* buff, uint8_t value){
    const char* digits = "0123456789ABCDEF";
    *buff++ = (uint8_t)(digits[ (value & 0xF0)>>4]);
    *buff++ = (uint8_t)(digits[ value & 0x0F]);
    return buff;
}

// Convert 2 hex digits into an 8 bit value.
static uint8_t decode(char msHex, char lsHex){
    int val=0;
    if(msHex >= 'a' && msHex <= 'f') val = (msHex - 'a' + 10) * 16;
    else if(msHex >= 'A' && msHex <= 'F') val = (msHex - 'A' + 10) * 16;
    else if(msHex >= '0' && msHex <= '9') val = (msHex - '0') * 16;
    
    if(lsHex >= 'a' && lsHex <= 'f') val += (lsHex - 'a' + 10);
    else if(lsHex >= 'A' && lsHex <= 'F') val += (lsHex - 'A' + 10);
    else if(lsHex >= '0' && lsHex <= '9') val += (lsHex - '0');

    return (char)val;
}

void EbyteLora::configure(EbyteLora::Config& config){
    configure(config, 0xC2);
}

void EbyteLora::configureAndSave(EbyteLora::Config& config){
    configure(config, 0xC0);
}

EbyteLora::Config config EbyteLora::getConfig(){
    if(getMode() != SLEEP) setMode(SLEEP);
    // module resets to 9600bps, 8N1 format in sleep mode.
    _serial.setBaudrate(9600);
    _serial.setFormat(8, 1, UART_PARITY_NONE); 
    const char* msg = "C1C1C1";
    _serial.write(msg, strlen(msg));

    uint8_t buff[12];
    _serial.read(buff,12);

    buff[0] = decode(buff[0], buff[1]);
    buff[1] = decode(buff[2], buff[3]);
    buff[2] = decode(buff[4], buff[5]);
    buff[3] = decode(buff[6], buff[7]);
    buff[4] = decode(buff[8], buff[9]);
    buff[5] = decode(buff[10], buff[11]);

    Config config(buff);
    return config;
}

EbyteLora::Version EbyteLora::getVersion(){
    if(getMode() != SLEEP) setMode(SLEEP);
    // module resets to 9600bps, 8N1 format in sleep mode.
    _serial.setBaudrate(9600);
    _serial.setFormat(8, 1, UART_PARITY_NONE); 

   const char* msg = "C3C3C3";
   _serial.write(msg, strlen(msg));

    uint8_t buff[8];
    _serial.read(buff,8);

    buff[0] = decode(buff[0], buff[1]);
    buff[1] = decode(buff[2], buff[3]);
    buff[2] = decode(buff[4], buff[5]);
    buff[3] = decode(buff[6], buff[7]);

    return Version version(buff);
}

EbyteLora::reset(){
    if(getMode() != SLEEP) setMode(SLEEP);
    // module resets to 9600bps, 8N1 format in sleep mode.
    _serial.setBaudrate(9600);
    _serial.setFormat(8, 1, UART_PARITY_NONE); 

    const char* msg = "C4C4C4";
    _serial.write(msg, strlen(msg));
    while(isBusy()){
        sleep_ms(1);
    }
}

void EbyteLora::write(const uint8_t* src,size_t len){
    _serial.write(src, len);
}

void EbyteLora::read(uint8_t* dst, size_t len){
    _serial.read(dst, len);
}


void EbyteLora::configure(EbyteLora::Config& config, uint8_t cmd){
    
    if(getMode() != SLEEP) setMode(SLEEP);

    // So send config & wait until it's sent   
    uint8_t buff[12]; 
    uint8_t* ptr = buff;
    const uint8_t* data = config.data();
    ptr = toHex(ptr, cmd);
    ptr = toHex(ptr, data[1]);
    ptr = toHex(ptr, data[2]);
    ptr = toHex(ptr, data[3]);
    ptr = toHex(ptr, data[4]);
    ptr = toHex(ptr, data[5]);
    
    // module resets to 9600bps, 8N1 format in sleep mode.
    _serial.setBaudrate(9600);
    _serial.setFormat(8, 1, UART_PARITY_NONE); 
    
    _serial.write(buff,12);
 
     void setUartFromConfig(EbyteLora::Config& config);
}

void EbyteLora::setUartFromConfig(const EbyteLora::Config& config){
    switch(config.parity()){
        case NONE: _serial.setFormat(8,1,UART_PARITY_NONE); break;
        case ODD:  _serial.setFormat(8,1,UART_PARITY_ODD); break;
        case EVEN: _serial.setFormat(8,1,UART_PARITY_EVEN); break;
    }

    uint baud = 9600;
    switch(config.uartBaud()){
        case BAUD_1200:   baud = 1200; break;
        case BAUD_2400:   baud = 2400; break;
        case BAUD_4800:   baud = 4800; break;
        case BAUD_9600:   baud = 9600; break;
        case BAUD_19200:  baud = 19200; break;
        case BAUD_38400:  baud = 38400; break;
        case BAUD_57600:  baud = 57600; break;
        case BAUD_115200: baud = 115200; break;
    }
    _serial.setBaudrate(baud);
}
