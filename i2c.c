
#include <xc.h>
#include "types.h"
#include "pins.h"
#include "i2c.h"
#include "state.h"
#include "motor.h"

volatile uint8 i2cRecvBytes[NUM_MOTORS][NUM_RECV_BYTES];
volatile uint8 i2cRecvBytesPtr;
volatile uint8 i2cSendBytes[NUM_SEND_BYTES-1];
volatile uint8 i2cSendBytesPtr;
volatile uint8 motIdxInPacket;
volatile uint8 bytesRecvd;

void i2cInit() {    
    sclTRIS = 1;
    sdaTRIS = 1;

//    SSP1CON1bits.SSPM = 0x0e;        // slave mode, 7-bit, S & P ints enabled 
    SSP1CON1bits.SSPM = 0x06;          // slave mode, 7-bit
    SSP1MSK           = I2C_ADDR_MASK; // address mask, check all top 5 bits
    SSP1ADD           = I2C_ADDR;      // slave address (7-bit addr is 8 or 12)
    SSP1STATbits.SMP  = 0;             // slew-rate enabled
    SSP1STATbits.CKE  = 1;             // smb voltage levels
    SSP1CON2bits.SEN  = 1;             // enable clock stretching 
    SSP1CON3bits.AHEN = 0;             // no clock stretch for addr ack
    SSP1CON3bits.DHEN = 0;             // no clock stretch for data ack
    SSP1CON3bits.BOEN = 1;             // enable buffer overwrite check
           
    SSPEN = 1;                         // Enable the serial port
    SSP1IF = 0;                        // nothing received yet
    SSP1IE = 1;                        // Enable ints
}

void checkI2c() {
  if(ms->i2cCmdBusy) {
    if(!haveError()) {
      processCommand();
    }
    ms->i2cCmdBusy = false;
  }
}

volatile struct motorState *p; // temp ptr

void i2cInterrupt(void) {
  dbg1
  if (WCOL || SSPOV) {
    setErrorInt(motIdxInPacket, OVERFLOW_ERROR);
  }
  else {
    if(!SSP1STATbits.D_nA) { 
      // received addr byte, extract motor number
      motIdxInPacket = (SSP1BUF & 0x06) >> 1;
      
      if(SSP1STATbits.R_nW) {
        // send packet (i2c read from slave), load buffer for first byte
        p = &mState[motIdxInPacket];
        SSP1BUF = p->stateByte;
        p->stateByte &= 0x8f; // reading status clears any error
        i2cSendBytes[0] = p->curPos >> 8;
        i2cSendBytes[1] = p->curPos & 0x00ff;
        i2cSendBytesPtr = 0;
      }
      else
        // recv packet (i2c write to slave), 
        i2cRecvBytesPtr = 0;
        bytesRecvd = 0;
    }
    else {
      if(!SSP1STATbits.R_nW) {
        // received data byte (i2c write to slave)
        
        if(mState[motIdxInPacket].i2cCmdBusy) {
          //last recv not handled yet by main loop
          setErrorInt(motIdxInPacket, OVERFLOW_ERROR);
        } 
        else {
          bytesRecvd++;
          if(i2cRecvBytesPtr < NUM_RECV_BYTES) 
            i2cRecvBytes[motIdxInPacket][i2cRecvBytesPtr++] = SSP1BUF;
          if(i2cRecvBytes[motIdxInPacket][0] == bytesRecvd-1) {
            // recvd all data
            i2cRecvBytes[motIdxInPacket][0] = i2cRecvBytesPtr-1;
            mState[motIdxInPacket].i2cCmdBusy = true;
          }
        }
      }
      else {
        // sent byte (i2c read from slave), load buffer for next send
        SSP1BUF = i2cSendBytes[i2cSendBytesPtr++];
      }
    }
  }
  CKP = 1; // end stretch
  volatile int z = SSP1BUF;  // clear BF 
  dbg0
}
