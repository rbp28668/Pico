#include "uart.h"

UART::UART(uart_inst_t* uart, uint baudrate, uint txPin, uint rxPin)
: _uart(uart){
    _baudrate = uart_init(uart, baudrate);

    if(txPin != UNUSED) gpio_set_function(txPin, GPIO_FUNC_UART);
    if(rxPin != UNUSED) gpio_set_function(rxPin, GPIO_FUNC_UART);
}

UART::~UART(){
    uart_deinit(_uart);
}

void UART::setHardwareFlowControl(int ctsPin, int rtsPin ){
    if(ctsPin != UNUSED) gpio_set_function(ctsPin, GPIO_FUNC_UART);
    if(rtsPin != UNUSED) gpio_set_function(rtsPin, GPIO_FUNC_UART);
    uart_set_hw_flow(_uart, ctsPin != UNUSED, rtsPin != unused);
}