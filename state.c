

#include "types.h"
#include "state.h"
#include "i2c.h"
#include "motor.h"

// do not use with interrupts off
void setCurState(uint8 newState) {
  // v bit (version is zero)
  GIE = 0;
  ms->stateByte = newState;
  setI2cCkSum();
  GIE = 1;
}

void setStateBit(uint8 mask, uint8 set){
  setCurState((ms->stateByte & ~mask) | (set ? mask : 0));
}

// do not use from interrupt
void setError(uint8 err) {
  setCurState(err);
  resetMotor();
}

// use from interrupt
void setErrorInt(uint8 motIdx, uint8 err) {
  mState[motIdx].stateByte = 
          ((mState[motIdx].stateByte & 0x8f) | err) | ERROR_BIT;
  setI2cCkSumInt(motIdx);
}
