
// PIC16F1827 Configuration Bit Settings
// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = OFF       // Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = ON       // MCLR Pin Function Select (MCLR/VPP pin function is MCLR)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Memory Code Protection (Data memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = ON        // Internal/External Switchover (Internal/External Switchover mode is enabled)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)
// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = ON       // PLL Enable (4x PLL enabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)

#include <xc.h>
#include "types.h"
#include "i2c.h" 
#include "pins.h"
#include "state.h"
#include "motor.h"
#include "clock.h"

void main(void) {
  ANSELA = 0; // no analog inputs
  ANSELB = 0; // these &^%$&^ regs cause a lot of trouble
  
  i2cInit();
  clkInit();
  motorInit();

  PEIE =  1;   // enable peripheral ints
  GIE  =  1;   // enable all ints
  
  // main event loop -- never ends
  while(true) {
    // motorIdx, ms, and sv are globals
    for(motorIdx=0; motorIdx < NUM_MOTORS; motorIdx++) {
      ms = &mState[motorIdx];
      sv = &(mSet[motorIdx].val);
      checkI2c();
      chkMotor();
    }
  }
}

// global interrupt routine
void __interrupt() globalInt() {
  // motor (clock) interrupts every 20 usecs (50 KHz))
  if(TMR2IF) {
    TMR2IF = 0;
    clockInterrupt();
  }
  // i2c interrupts usually 25 usecs
  if(SSP1IF) {
    SSP1IF = 0;
    i2cInterrupt();
  }
}


