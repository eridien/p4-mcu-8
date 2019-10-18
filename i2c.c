
#include <xc.h>
#include "types.h"
#include "pins.h"
#include "i2c.h"
#include "state.h"
#include "motor.h"

volatile uint8 i2cRecvBytes[NUM_MOTORS][NUM_RECV_BYTES];
volatile uint8 i2cRecvBytesPtr;
volatile uint8 i2cSendBytes[NUM_SEND_BYTES];
volatile uint8 i2cSendBytesPtr;
volatile bool  inPacket;
volatile uint8 motIdxInPacket;

void i2cInit() {    
    sclTRIS = 1;
    sdaTRIS = 1;

    SSP1CON1bits.SSPM = 0x0e;          // slave mode, 7-bit, S & P ints enabled 
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

// called from interrupt
void setSendBytes(uint8 motIdx) {
  struct motorState *p = &mState[motIdx];
  i2cSendBytes[0] = p->stateByte;
  i2cSendBytes[1] = p->curPos >> 8;
  i2cSendBytes[2] = p->curPos & 0x00ff;
}

void checkI2c() {
  if(haveError() && (ms->stateByte & BUSY_BIT)) {
    // have error and motor moving, stop and reset
    softStopCommand(true);
  }
  if(ms->i2cCmdBusy) {
    if(!haveError()) {
      processCommand();
    }
    // don't clear until done with data
    ms->i2cCmdBusy = false;
  }
}

volatile struct motorState *p; // temp ptr

void i2cInterrupt(void) {
  // SSP1STATbits.S is set during entire packet
  if(SSP1STATbits.S && !inPacket) { 
    // received start bit, prepare for packet
    i2cRecvBytesPtr = 1; // skip over length byte
    i2cSendBytesPtr = 1; // not used for first byte
    WCOL = 0;                   // clear WCOL
    volatile int x = SSP1BUF;   // clear SSPOV
    inPacket = true;
    motIdxInPacket = 99;
  }
  else if(SSP1STATbits.P) { 
    // received stop bit, on read tell loop that data is available
    inPacket = false;
    if(motIdxInPacket != 99) {
      // packet was for us
      if (WCOL || SSPOV) {
        setErrorInt(motIdxInPacket, OVERFLOW_ERROR);
      }
      else {
        if(!SSP1STATbits.R_nW) {
          // total length of recv is stored in first byte
          i2cRecvBytes[motIdxInPacket][0] = i2cRecvBytesPtr-1;
          mState[motIdxInPacket].i2cCmdBusy = true;
        } else {
          // master just read status,  clear error
          mState[motIdxInPacket].stateByte &= 0x8f;
        }
      }
    }
  }
  else {
    if(!SSP1STATbits.D_nA) { 
      // received addr byte, extract motor number
      motIdxInPacket = (SSP1BUF & 0x06) >> 1;
      if(SSP1STATbits.R_nW) {
        // send packet (i2c read from slave), load buffer for first byte
        dbg1
        p = &mState[motIdxInPacket];
        i2cSendBytes[0] = p->stateByte;
        i2cSendBytes[1] = p->curPos >> 8;
        i2cSendBytes[2] = p->curPos & 0x00ff;
        SSP1BUF = i2cSendBytes[0];
      }
    }
    else {
      if(!SSP1STATbits.R_nW) {
        if(mState[motIdxInPacket].i2cCmdBusy) {
          // oops, last recv not handled yet by main loop
          setErrorInt(motIdxInPacket, OVERFLOW_ERROR);
        } else {
          // received byte (i2c write to slave)
          if(i2cRecvBytesPtr < NUM_RECV_BYTES) 
            i2cRecvBytes[motIdxInPacket][i2cRecvBytesPtr++] = SSP1BUF;
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
