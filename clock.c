
#include <xc.h>
#include "types.h"
#include "clock.h"
#include "pins.h"
#include "motor.h"

void clkInit(void) {
  T2CONbits.T2CKPS  =  2;   // prescaler 1:16, 8 MHz -> 500 KHz
  T2CONbits.T2OUTPS =  0;   // postscaler 1:1
  PR2     = (500000 / CLK_RATE) - 1; // wraps at count 50 (100 usecs, 10 khz)
  TMR2ON  =  1;   // enable timer
  TMR2IF  =  0;   // int flag
  TMR2IE  =  1;   // enable timer int
}

volatile uint16 timeTicks;     // units: 100 usecs, wraps on ~6 secs
