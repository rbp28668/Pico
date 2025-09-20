#include <stdio.h>
#include <assert.h>
#include "pico/stdlib.h"
#include "pico/sync.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
//#include "hardware/structs/iobank0.h"
#include "telemetry.h"

/// \tag::telemetry

#define UART_ID uart0
#define BAUD_RATE 38400
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 0
#define UART_RX_PIN 1

// A few outputs to help diagnosis with a scope.
#define USE_SCOPE_PINS 1

#ifdef USE_SCOPE_PINS
#define SCOPE_PIN 2
#define SCOPE_WAIT 3
#define SCOPE_TRIG 4
#endif

static Slots slots;

typedef enum {
    state_wait_for_address,
    state_wait_receive_data_1, 
    state_wait_receive_data_2,
    state_wait_receive_data_3,
    state_receive_complete,
    state_wait_send_1,
    state_wait_send_2,
    state_wait_send_3,
    state_wait_send_complete,
    state_transmit_complete,
 } state_t;
static state_t state = state_wait_for_address;

typedef enum {
    event_char_received,
    event_char_transmitted,
    event_tx_wait_timeout,  // timeout of waiting to transmit
    event_slot_end_timeout  // timeout of receiving payload of another device's telemetry.
} event_t;




static uint8_t slot_index;
static int32_t received_data; // All 3 bytes. Can decode if we want.

static critical_section_t state_machine_critsec;

const uint LED_PIN = 25;


// Enables or disables the outpout pad for the UART TX
static inline void get_bus(uint gpio, bool enabled) {
    gpio_set_function(gpio, enabled ? GPIO_FUNC_UART : GPIO_FUNC_NULL);
}

// Forward
extern "C" void handle_event(event_t event, uint8_t ch); 


// Called from timer to mark time between receiving an address byte we're looking for
// and transmitting the first response byte.
// end up in sync.
extern "C" int64_t on_pre_transmit_timeout(alarm_id_t id, void *user_data) {
    handle_event(event_tx_wait_timeout, 0);
    return 0;
}

// Called from timer after we could have received any data bytes on the
// bus.  
extern "C" int64_t on_slot_end_timeout(alarm_id_t id, void *user_data) {
    handle_event(event_slot_end_timeout,0);
    return 0;
}

// UART interrupt handler.  Called for uart has data or uart needs data.
// Note there is a chance of spurious transmit interrupts if a character is
// received whilst transmit buffer is empty - but only if the buffer is empty.
extern "C" void on_uart_event() {
    irq_clear(UART0_IRQ);
    uint8_t ch = 0;
    if(uart_is_readable(UART_ID)) {
        ch = uart_getc(UART_ID); // waits for data to become readable but as we've just checked....
        handle_event(event_char_received, ch);
    }
    if(uart_is_writable(UART_ID)) {
        handle_event(event_char_transmitted,0);
    }
    return;

}

// Call this at the end of a processing cycle.
static inline void wait_for_next_frame(){
     state = state_wait_for_address; // go back to waiting for address
}


// State machine to manage MPX protocol.
// Frame timing: 
// 1 byte takes c. 260 uS to transmit or receive.
// Receive 1 byte:  260 uS
// delay before response 300
// send or receive 3 bytes: 3*260 = 800 approx
// total 1350uS
extern "C" void handle_event(event_t event, uint8_t ch) {

    critical_section_enter_blocking(&state_machine_critsec);

    // Slot end timeout always goes back to wait for next address.
    if(event == event_slot_end_timeout) {
        #ifdef USE_SCOPE_PINS
        gpio_put(SCOPE_WAIT,0);
        gpio_put(SCOPE_TRIG,0);
        #endif

        // Release bus 
        get_bus(UART_TX_PIN, false);
        state = state_wait_for_address;
    }

    switch(state){
        // Wait for initial address byte
        case state_wait_for_address: 
            if(event == event_char_received) {  // address byte receive (hopefully)
                // Set overall slot timer
                alarm_id_t aid = add_alarm_in_us (4500, on_slot_end_timeout, 0, false);
                if(aid <= 0){
                    printf("Failed to set slot end timeout\n");
                }
 
                slot_index = ch & 0x0F;

                if(slot_index == 2){
                    gpio_put(SCOPE_TRIG,1);
                }

                Slot* slot = slots.getSlot(slot_index);
                if(slot->isEnabled()){
     
                    // Change bus direction to transmit.
                    get_bus(UART_TX_PIN, true);
 
                    // Delay before we actually transmit
                    alarm_id_t aid = add_alarm_in_us (600, on_pre_transmit_timeout, 0, false);
                    if(aid <= 0){
                        printf("Failed so set transmit timeout\n");
                    }
  
                    state = state_wait_send_1; // wait to send first response byte
                } else { // Not one of ours so wait for first data (if any)
                    state = state_wait_receive_data_1;
                }

            }
            break;
        

        // Receive first data byte    
        case state_wait_receive_data_1:
            if(event == event_char_received) {
                received_data = ch;
                state = state_wait_receive_data_2;
            } 
            break;

            
        // Receive second data byte
        case state_wait_receive_data_2:
            if(event == event_char_received) {
               received_data |= ((uint32_t)ch << 8);
               state = state_wait_receive_data_3;
            } 
            break;

        // Receive third data byte
        case state_wait_receive_data_3:
            if(event == event_char_received) {
                received_data |= ((uint32_t)ch << 16);
                state = state_receive_complete;
            } 
            break;

        case state_receive_complete:
            // just lurk in this state until the end of frame timeout.
            break;

        case state_wait_send_1: 
            if(event == event_tx_wait_timeout){
                gpio_put(LED_PIN, 1);
                #ifdef USE_SCOPE_PINS
                gpio_put(SCOPE_PIN, 1);
                #endif

                // Note uart_putc_raw doc say:
                // Write single character to UART for transmission.
                // This function will block until all the character has been sent
                // However, looking at uart.h uart_putc_raw calls uart_write_blocking which waits for tx to become free
                // before writing but doesn't block afterwards.
                Slot* slot = slots.getSlot(slot_index);
                uart_putc_raw(UART_ID, slot->byte1());
                uart_set_irq_enables(UART_ID, true, true); // now want tx_needs_data interrupts.
                state = state_wait_send_2;
            }
            break;

        case state_wait_send_2: 
            if(event == event_char_transmitted){
                Slot* slot = slots.getSlot(slot_index);
                uart_putc_raw(UART_ID, slot->byte2());
                state = state_wait_send_3;
            }
            break;

        case state_wait_send_3:
            if(event == event_char_transmitted){
                Slot* slot = slots.getSlot(slot_index);
                uart_putc_raw(UART_ID, slot->byte3());
                state = state_wait_send_complete;
            }
            break;

        case state_wait_send_complete:
            if(event == event_char_transmitted){

                // Turn off uart_needs_data interrupts.
                uart_set_irq_enables(UART_ID, true, false);
                #ifdef USE_SCOPE_PINS
                gpio_put(SCOPE_PIN,0);
                #endif
                gpio_put(LED_PIN, 0);  // diagnostics
 
                state = state_transmit_complete;
            }
            break;
        
        case state_transmit_complete:
            // just lurk in this state until the end of frame timeout.
            #ifdef USE_SCOPE_PINS
            gpio_put(SCOPE_WAIT,1);
            #endif
            break;

        default:
            state = state_wait_for_address; // recover by resync.
            break;
    }

     
    critical_section_exit(&state_machine_critsec);
    return;
 
}


// Basic test of telemetry.  Non interrupt driven this simulates values but is easy
// to reason about and can be used if there appear to be interfacing problems.
void basic_telemetry() {
    
    int value = 0;
    int dirn = 1;
    while(1) {
 

        value += dirn;
        if(value >= 1000) {
            dirn = -1;
        } else if (value <= 0){
            dirn = 1;
        }
        slots.getSlot(2)->setValue(value);
        slots.getSlot(3)->setValue(10*dirn);
        slots.getSlot(4)->setValue(value);
        slots.getSlot(5)->setValue(10*dirn);
        slots.getSlot(6)->setValue(value);
        slots.getSlot(7)->setValue(10*dirn);

        // Wait for and get address byte
        uint8_t ch = uart_getc(UART_ID);
        int index = ch & 0x0F;
        Slot* slot = slots.getSlot(index);

        if(slot->isEnabled()){
      
            // Change bus direction to transmit.  Note, doesn't work if after the wait.
            get_bus(UART_TX_PIN, true);
         
            busy_wait_us_32(600);  // doesn't seem critical - MPX receiver channels seem to be c. half way through slot.
 
            gpio_put(LED_PIN, 1);  // diagnostics
            gpio_put(SCOPE_PIN, 1);

            uart_putc_raw(UART_ID, slot->byte1());
            uart_tx_wait_blocking(UART_ID);
            uart_putc_raw(UART_ID, slot->byte2());
            uart_tx_wait_blocking(UART_ID);
            uart_putc_raw(UART_ID, slot->byte3());
            uart_tx_wait_blocking(UART_ID);
       
            // Release the bus.
            get_bus(UART_TX_PIN, false);
  
            gpio_put(SCOPE_PIN, 0);
            gpio_put(LED_PIN, 0);  // diagnostics
     
        } else {
            busy_wait_us_32(1400); // 1400 uS, roughly the same as if we've waited and transmitted.
        }

        // Flush any reception of our or other transmission
        busy_wait_us_32(1000);
        while(uart_is_readable(UART_ID)){
           uart_getc(UART_ID);
        }

    }
}

void setup_uart() {

     // Set up our UART with the desired baud rate.
    int actual = uart_init(UART_ID, BAUD_RATE);
    printf("Actual baud rate: %d\n",actual);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Disable pulls on both UART pins otherwise they fight the receiver output.
    gpio_disable_pulls(UART_TX_PIN);
    gpio_disable_pulls(UART_RX_PIN);

  
    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(UART_ID, false, false);

    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    // Make sure UART doesn't translate newline to CRLF.
    uart_set_translate_crlf (UART_ID,false);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, false);

 
}

void setup_uart_interrupts() {
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_event);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - 
    // Only RX data at the moment until we want to track tx_needs_data
    uart_set_irq_enables(UART_ID, true, false);
}

// Wait for a break of at least 4mS without a character
// This should mean the next character received is an
// address character.
void initial_sync() {

    gpio_set_function(UART_TX_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(UART_TX_PIN, false); // input

    printf("Entering initial sync\n"); 
    uint64_t start = time_us_64 ();
    while(true){
        if(uart_is_readable(UART_ID)) {
            uart_getc(UART_ID); // and ignore
            start = time_us_64 (); // restart timer.
        }

        uint64_t now = time_us_64();
        if( (now - start) > 4000){
            break;
        }
    }
    printf("Exit initial sync\n"); 

}

void start_telemetry() {

    #ifdef USE_SCOPE_PINS
    // Extra pin to signal when we're outputting for debug
    gpio_init(SCOPE_PIN);
    gpio_set_dir(SCOPE_PIN, GPIO_OUT);
    gpio_put(SCOPE_PIN, 0);

    gpio_init(SCOPE_WAIT);
    gpio_set_dir(SCOPE_WAIT, GPIO_OUT);
    gpio_put(SCOPE_WAIT, 0);

    gpio_init(SCOPE_TRIG);
    gpio_set_dir(SCOPE_TRIG, GPIO_OUT);
    gpio_put(SCOPE_TRIG, 0);
    #endif

    critical_section_init (&state_machine_critsec);

    setup_uart();
    initial_sync();
    setup_uart_interrupts();
}

Slots& telemetry_slots() {
    return slots;
}
/// \end:telemetry[]
