
#include <xc.h>
#include "types.h"
#include "motor.h"
#include "i2c.h"
#include "state.h"
#include "pins.h"
#include "clock.h"
#include "move.h"

// globals for use in main chk loop
uint8  motorIdx;
struct motorState      *ms;
struct motorSettings   *sv;

volatile unsigned char *motorPort[NUM_MOTORS][4] = {
  {&z0PORT, &z1PORT, &z2PORT, &z3PORT},
  {&l0PORT, &l1PORT, &l2PORT, &l3PORT},
  {&p0PORT, &p1PORT, &p2PORT, &p3PORT},
};

uint8 motorMask[NUM_MOTORS][4] = {
  {z0BIT, z1BIT, z2BIT, z3BIT},
  {l0BIT, l1BIT, l2BIT, l3BIT},
  {p0BIT, p1BIT, p2BIT, p3BIT},
};

void clrMotorPin(uint8 mot, uint8 pin) {
  *motorPort[mot][pin] &= ~motorMask[mot][pin];
}

void setMotorPin(uint8 mot, uint8 pin) {
  *motorPort[mot][pin] |= ~motorMask[mot][pin];
}

// default startup values
// must match settingsStruct
uint16 settingsInit[NUM_SETTING_WORDS] = {
   6400,  // max ms->speed: 80 mm/sec
      2,  // min ms->speed: 2 steps/sec (else sw blows up)
   1600,  // no-accelleration ms->speed limit:  20 mm/sec
     80,  // accelleration rate: 1 mm/sec/sec
      0,  // homePos
};

// estimated decell distance by ms->speed
// wild guess for now
uint16 decellTable[][2] = {
  {150*STEPS_MM, 32*STEPS_MM},
  {100*STEPS_MM, 16*STEPS_MM},
  { 80*STEPS_MM,  8*STEPS_MM},
  { 60*STEPS_MM,  4*STEPS_MM},
  { 40*STEPS_MM,  2*STEPS_MM},
  { 20*STEPS_MM,  1*STEPS_MM},   
};

bool withinDecellDist() {
  int16 distRemaining = (ms->targetPos - ms->curPos);
  ms->targetDir = 1;
  if(distRemaining < 0) {
    ms->targetDir = 0;
    distRemaining = -distRemaining;
  }
  if(ms->targetDir != ms->dir) return true;
  
  for(uint8 i = 0; i < sizeof(decellTable)/2; i++) {
    if(ms->speed >= decellTable[i][0] &&
       distRemaining <= decellTable[i][1]) {
      return true;
    }
  }
  return false;
}

void motorInit() {
  for(uint8 motIdx=0; motIdx < NUM_MOTORS; motIdx++) {
    struct motorState *p = &mState[motIdx];
    p->curPos          = POS_UNKNOWN_CODE;  // 1/80 mm
    p->dir             = 1;  // 1 => forward
    p->phase           = 0;
    p->targetPos       = 1;  // 1/80 mm
    p->speed           = 0;  // 1/80 mm/sec
    p->targetSpeed     = 0;  // 1/80 mm/sec
    
    for(uint8 i = 0; i < NUM_SETTING_WORDS; i++) {
       mSet[motIdx].reg[i] = settingsInit[i];
    }
  }
  
  // motors turned off
  z0TRIS = 1;
  z1TRIS = 1;
  z2TRIS = 1;
  z3TRIS = 1;

  l0TRIS = 1;
  l1TRIS = 1;
  l2TRIS = 1;
  l3TRIS = 1;

  p0TRIS = 1;
  p1TRIS = 1;
  p2TRIS = 1;
  p3TRIS = 1;
}

void setMotorSettings() {
  for(uint8 i = 0; i < sizeof(mSet[motorIdx]) / 
                       sizeof(mSet[motorIdx].reg[0]); i++) {
    mSet[motorIdx].reg[i] = (i2cRecvBytes[motorIdx][2*i + 1] << 8) | 
                             i2cRecvBytes[motorIdx][2*i + 2];
  }
}

void stopStepping() {
  setStateBit(MOVING_BIT,   0);
  setStateBit(STOPPING_BIT, 0);
  ms->stepPending = false;
  ms->stepped     = false;
}

void resetMotor() {
  stopStepping();
  ms->curPos = POS_UNKNOWN_CODE;
  setStateBit(MOTOR_ON_BIT, 0);
  for(uint8 i=0; i<4; i++)
    clrMotorPin(motorIdx, i);
}

bool underAccellLimit() {
  return (ms->speed <= sv->noAccelSpeedLimit);
}

void setStep() {
  if(ms->speed == 0) ms->speed = 1;
  uint16 stepTicks = CLK_RATE / ms->speed;
  if(stepTicks == 0) stepTicks = 1;
  setNextStep(getLastStep() + stepTicks);
  ms->stepped = false;
  ms->stepPending = true;
}

void chkStopping() {
  // in the process of stepping
  if(ms->stepPending || ms->stepped) return;
  // check ms->speed/acceleration
  if(!underAccellLimit()) {
    // decellerate
    ms->speed -= sv->accellerationRate;
  }
  else {
    stopStepping();
    if(ms->resetAfterSoftStop) resetMotor();
    return;
  }
  setStep();
}

// from main loop
void chkMotor() {
  if(ms->stepped) {
    ms->curPos++;
    ms->stepped = false;
  }
  if(ms->stateByte & STOPPING_BIT) {
    if(!haveError()) chkStopping();
    
  } else if(ms->stateByte & MOVING_BIT) {
    if(!haveError()) chkMoving();
  }
}

void softStopCommand(bool resetAfter) {
  ms->stateByte != STOPPING_BIT;
  ms->resetAfterSoftStop = resetAfter;
}

void setMotorPins(uint8 phase) {
  switch (phase) {
    case 0:
      clrMotorPin(motorIdx, 1); clrMotorPin(motorIdx, 1);
      clrMotorPin(motorIdx, 0); clrMotorPin(motorIdx, 0);
      break;
    case 1:
      clrMotorPin(motorIdx, 0); clrMotorPin(motorIdx, 1);
      clrMotorPin(motorIdx, 1); clrMotorPin(motorIdx, 0);
      break;
    case 2:
      clrMotorPin(motorIdx, 0); clrMotorPin(motorIdx, 0);
      clrMotorPin(motorIdx, 1); clrMotorPin(motorIdx, 1);
      break;
    case 3:
      clrMotorPin(motorIdx, 1); clrMotorPin(motorIdx, 0);
      clrMotorPin(motorIdx, 0); clrMotorPin(motorIdx, 1);
      break;    
  }
}

void motorOnCmd() {
  ms->curPos = POS_UNKNOWN_CODE;
  ms->stateByte |= MOTOR_ON_BIT;
  setMotorPins(ms->phase);
}

uint8 numBytesRecvd;

bool lenErr(uint8 expected) {
  if(expected != numBytesRecvd) {
    setError(CMD_DATA_ERROR);
    return true;
  }
  return false;
}
  
// from i2c
void processMotorCmd() {
  volatile uint8 *rb = ((volatile uint8 *) i2cRecvBytes[motorIdx]);
  numBytesRecvd   = rb[0];
  uint8 firstByte = rb[1];
  
  if((firstByte & 0x80) == 0) {
    if(lenErr(2)) return;
    // simple goto pos command, 15-bits in 1/80 mm/sec
    moveCommand(((int16) firstByte << 8) | rb[2]);
  }
  else if((firstByte & 0xfc) == 0x80) {
    if(lenErr(3)) return;
    // set max ms->speed reg encoded as 10 mm/sec, 150 max
    sv->maxSpeed = (firstByte & 0x0f) * 10;
    // followed by goto pos caommand
    moveCommand(((int16) rb[2] << 8) | rb[3]);
  }
  else switch(firstByte & 0xf0) {
    case 0xa0: if(!lenErr(1)) softStopCommand(false); break; // stop,no reset
    case 0xb0: if(!lenErr(1)) softStopCommand(true);  break; // stop with reset
    case 0xc0: if(!lenErr(1)) resetMotor();           break; // hard stop (immediate reset)
    case 0xd0: if(!lenErr(1)) motorOnCmd();           break; // reset off
    case 0xf0: if(!lenErr(NUM_SETTING_WORDS)) 
                 setMotorSettings();                  break; // set all regs
    default: lenErr(255); // invalid cmd sets CMD_DATA_ERROR
  }
}

uint16 getLastStep(void) {
  GIE = 0;
  uint16 temp = ms->lastStepTicks;
  GIE = 1; 
  return temp;
}

void setNextStep(uint16 ticks) {
  GIE = 0;
  ms->nextStepTicks = ticks;
  GIE = 1; 
}

void clockInterrupt(void) {
  timeTicks++;
  for(int motIdx = 0; motIdx < NUM_MOTORS; motIdx++) {
    struct motorState *p = &mState[motIdx];
    if(p->stepPending && p->nextStepTicks == timeTicks) {
      if(p->stepped) {
        // last motor step not handled yet
        setErrorInt(motIdx, STEP_NOT_DONE_ERROR);
        return;
      }
      p->phase += (p->dir ? 1 : -1);
      if(p->phase ==   4) p->phase = 0;
      if(p->phase == 255) p->phase = 3;
      setMotorPins(p->phase);
      
      p->stepPending = false;
      p->lastStepTicks = timeTicks;
      p->stepped = true;
    }
  }
}
