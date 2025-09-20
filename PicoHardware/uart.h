#ifndef _UART_H
#define _UART_H

#include <stdint.h>
#include "hardware/uart.h"
#include "hardware/gpio.h"

class UART {
    uart_inst_t* _uart;
    uint _baudrate;

    public:

    const int UNUSED = -1;

    UART(uart_inst_t* uart, uint baudrate, int txPin, int rxPin);
    ~UART() {}


    void setHardwareFlowControl(int ctsPin, int rtsPin );
 
    uint baudrate() const { return _baudrate;}

    uint txDreq() { return uart_get_dreq(_uart, true);}
    uint rxDreq() { return uart_get_dreq(_uart, false);}
    
    char getc() { return uart_getc(_uart);}
    bool isEnabled() { return uart_is_enabled(_uart);}
    bool isReadable() { return uart_is_readable(_uart);}
    bool isReadableWithinUs(uint32_t us) { return uart_is_readable_within_us(_uart, us);}
    bool isWritable() { return uart_is_writable(_uart);}
    void putc(char c) { uart_putc(_uart, c);}
    void putcRaw(char c) { uart_putc_raw(_uart, c);} 
    void puts(const char* s) { uart_puts(_uart, s);}
    void read(uint8_t* dst, size_t len) {uart_read_blocking(_uart, dst, len);}
    uint setBaudrate(uint baudrate) { return _baudrate = uart_set_baudrate(_uart, baudrate);}
    void setBreak(bool on = true) { uart_set_break(_uart, on);}
    void enableFifo(bool on = true) { uart_set_fifo_enabled(_uart, on);}
    void setFormat(uint dataBits, uint stopBits, uart_parity_t 	parity) { uart_set_format(_uart, dataBits, stopBits, parity);}
    void enableHardwareFlowControl(bool cts, bool rts ) { uart_set_hw_flow(_uart, cts, rts)};
    void setInterruptEnables(bool rxHasData, bool txNeedsData){ uart_set_irq_enables(_uart, rxHasData, txNeedsData);}
    void translateCrLf(bool translate = true){ uart_set_translate_crlf(_uart, translate);}
    void waitUntilSent() {uart_tx_wait_blocking(_uart);}
    void write(const uint8_t* src,size_t len) {uart_write_blocking(_uart, src, len);}
};



#endif
