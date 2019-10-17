

#include "types.h"
#include "state.h"
#include "i2c.h"
#include "motor.h"

bool intsWereEnabled = false;

void intsOff(void) {
  intsWereEnabled = GIE;
  GIE = 0;
}

void intsOn(void) {
  GIE = intsWereEnabled;
}

void setCurState(uint8 newState) {
  // v bit (version is zero)
  ms->stateByte = newState;
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
  mState[motIdx].stateByte = err;
}
