// Code to drive a stepper motor controlled clock
// (C) Bruce Porteous 2021
// Stepper directly drives the minute hand.  Effectively uses
// Bresenham's line drawing algorithm to decide when to move the motor
// to give the required number of steps per hour from a regular (10Hz)
// timer tick

#include <stdio.h>
#include <assert.h>
#include "pico/stdlib.h"
#include "clock.h"

#define CALIBRATE 0

const uint LED_PIN = 25;

// Wiring:
// 31  GP26     *
// 30  RUN      |
// 29  GP22  *  |
// 28  GND   |  *       ENABLE
// 27  GP21  |          M0
// 26  GP20  |          M1
// 25  GP19  |          M2
// 24  GP18  |          RESET
// 23  GND   *          SLEEP
// 22  GP17             STEP
// 21  GP16             DIR


// Pins on DRV89825
#define DIR 16
#define STEP 17
#define SLEEP 22
#define RESET 18
#define M2 19
#define M1 20
#define M0 21
#define ENABLE 26

#define MICROSTEP 1
#define STEPS_PER_REV 200
#define GEAR_RATIO 18

// Bresenham variables
long _steps = 3600 * 100;  // number of ticks per rev (1hr) at 100Hz
long _dist = MICROSTEP * STEPS_PER_REV * GEAR_RATIO;    // steps per rev with 200 steps on the motor and x32 microstepping
long _err = 2 * _dist - _steps;  // Bresenham error term
int state = 1;


repeating_timer_t time;

// Timer callback.  Core of bresehnam, maintains the error term for ever and
// decides whether to move the stepper (pulse the step pin) as needed.
extern "C" bool callback (repeating_timer_t *rt) {
  if(_err > 0) {
    gpio_put(STEP, true);
    busy_wait_us(10);
    gpio_put(STEP, false);
    
    gpio_put(LED_PIN, state);
    state ^= 1;
    _err = _err - 2 * _steps;
  }
  _err = _err + 2 * _dist;
  return true;
}

extern "C" int main() {

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN,true);
    gpio_put(LED_PIN,state);

    gpio_init(DIR);
    gpio_init(STEP);
    gpio_init(SLEEP);
    gpio_init(RESET);
    gpio_init(M2);
    gpio_init(M1);
    gpio_init(M0);
    gpio_init(ENABLE);
    
    gpio_set_dir(DIR, GPIO_OUT);
    gpio_set_dir(STEP, GPIO_OUT);
    gpio_set_dir(SLEEP, GPIO_OUT);
    gpio_set_dir(RESET, GPIO_OUT);
    gpio_set_dir(M2, GPIO_OUT);
    gpio_set_dir(M1, GPIO_OUT);
    gpio_set_dir(M0, GPIO_OUT);
    gpio_set_dir(ENABLE, GPIO_OUT);
    
    gpio_put(DIR, 1);
    gpio_put(STEP, 0);
    gpio_put(M0, 0); // setting needs to match steps per rev
    gpio_put(M1, 0);
    gpio_put(M2, 0);
    gpio_put(ENABLE, 0);
    gpio_put(SLEEP, 1);
    gpio_put(RESET, 1);

    int t = -10;
       
    // Call back at 10Hz (every 100mS)
    add_repeating_timer_ms(t, &callback, 0, &time);
    while(true) {

    } 
}
