
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

void setMotorPin(uint8 mot, uint8 pin, bool on) {
  if(on) *motorPort[mot][pin] |=  motorMask[mot][pin];
  else   *motorPort[mot][pin] &= ~motorMask[mot][pin];
}

// default startup values
// must match settingsStruct
uint16 settingsInit[NUM_SETTING_WORDS] = {
      1,  // accelIdx accelleration rate: 1 mm/sec/sec
   6400,  // max speed: 80 mm/sec
   1600,  // jerk  20 mm/sec
      0,  // homePos: home value used by command
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
    p->curPos          = 0;  // 1/80 mm
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

void setMotorSettings(uint8 numWords) {
  for(uint8 i = 0; i < sizeof(mSet[motorIdx]) / 
                       sizeof(mSet[motorIdx].reg[0]); i++) {
    if(i == numWords) break;
    mSet[motorIdx].reg[i] = (i2cRecvBytes[motorIdx][2*i + 1] << 8) | 
                             i2cRecvBytes[motorIdx][2*i + 2];
  }
  ms->haveSettings = true;
}

void stopStepping() {
  setStateBit(BUSY_BIT, 0);
  ms->stepPending = false;
  ms->stepped     = false;
}

void resetMotor() {
  stopStepping();
  ms->curPos = 0;
  setStateBit(MOTOR_ON_BIT, 0);
  for(uint8 i=0; i<4; i++)
    setMotorPin(motorIdx, i, 0);
}

bool underJerkSpeed() {
  return (ms->speed <= sv->jerk);
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
  if(!underJerkSpeed()) {
    // decellerate
    ms->speed -= ms->accelleration;
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
  if(ms->stopping) {
    if(!haveError()) chkStopping();
    
  } else if(ms->stateByte & BUSY_BIT) {
    if(!haveError()) chkMoving();
  }
}

void softStopCommand(bool resetAfter) {
  ms->stopping = true;
  ms->resetAfterSoftStop = resetAfter;
}

void setMotorPins(uint8 phase) {
  switch (phase) {
    case 0:
      setMotorPin(motorIdx, 0, 1); setMotorPin(motorIdx, 1, 1);
      setMotorPin(motorIdx, 2, 0); setMotorPin(motorIdx, 3, 0);
      break;
    case 1:
      setMotorPin(motorIdx, 0, 0); setMotorPin(motorIdx, 1, 1);
      setMotorPin(motorIdx, 2, 1); setMotorPin(motorIdx, 3, 0);
      break;
    case 2:
      setMotorPin(motorIdx, 0, 0); setMotorPin(motorIdx, 1, 0);
      setMotorPin(motorIdx, 2, 1); setMotorPin(motorIdx, 3, 1);
      break;
    case 3:
      setMotorPin(motorIdx, 0, 1); setMotorPin(motorIdx, 1, 0);
      setMotorPin(motorIdx, 2, 0); setMotorPin(motorIdx, 3, 1);
      break;    
  }
}

void motorOnCmd() {
  setStateBit(MOTOR_ON_BIT, 1);
  setMotorPins(ms->phase);
}

uint8 numBytesRecvd;

// called on every command except settings
bool lenIs(uint8 expected, bool chkSettings) {
  if(chkSettings && !ms->haveSettings) {
    setError(NO_SETTINGS);
    return false;
  }
  if (expected != numBytesRecvd) {
    setError(CMD_DATA_ERROR);
    return false;
  }
  return true;
}
  
//  accel is 0..7: none, 4000, 8000, 20000, 40000, 80000, 200000, 400000 steps/sec/sec
//  for 1/40 mm steps: none, 100, 200, 500, 1000, 2000, 5000, 10000 mm/sec/sec
const uint16 accelTable[8] = // (steps/sec/sec accel) / 8
       {0, 500, 1000, 2500, 5000, 10000, 25000, 50000};

void processCommand() {
  volatile uint8 *rb = ((volatile uint8 *) i2cRecvBytes);
  numBytesRecvd   = rb[0];
  uint8 firstByte = rb[1];
  if ((firstByte & 0x80) == 0x80) {
    if (lenIs(2, true)) {
      // move command
      ms->targetSpeed = sv->speed;
      moveCommand(((int16) (firstByte & 0x7f) << 8) | rb[2]);
    }
  } else if ((firstByte & 0xc0) == 0x40) {
    // speed-move command
    if (lenIs(3, true)) {
      // changes settings for speed
      ms->targetSpeed = (uint16) (firstByte & 0x3f) << 8;
      moveCommand((int16) (((uint16) rb[2] << 8) | rb[3]));
    }
  } else if ((firstByte & 0xf8) == 0x08) {
    // accel-speed-move command
    if (lenIs(5, true)) {
      // changes settings for acceleration and speed
      sv->speed = (((uint16) rb[2] << 8) | rb[3]);
      ms->accelleration = accelTable[firstByte & 0x07];
      ms->targetSpeed = sv->speed;
      moveCommand((int16) (((uint16) rb[4] << 8) | rb[5]));
    }
  } else if ((firstByte & 0xe0) == 0x20) {
    // jog command relative - no bounds checking and doesn't need to be homed
    if (lenIs(2, true)) {
      motorOnCmd();
      uint16 dist = ((( (uint16) firstByte & 0x0f) << 8) | rb[2]);
      // direction bit is in d4
      if(firstByte & 0x10) ms->targetPos = ms->curPos + dist;
      else                 ms->targetPos = ms->curPos - dist;
      ms->accelleration = 0;
      ms->targetSpeed   = sv->jerk;
      moveCommand(true);
    }
  } else if (firstByte == 0x02) {
    // jog command relative - no bounds checking and doesn't need to be homed
    if (lenIs(3, true)) {
      motorOnCmd(); 
      ms->accelleration = 0;
      ms->targetSpeed  = sv->jerk;
      moveCommand(ms->curPos + (int16) (((uint16) rb[2] << 8) | rb[3]));
    }
  } else if (firstByte == 0x03) {
    // jog command relative - no bounds checking and doesn't need to be homed
    if (lenIs(3, true)) {
      motorOnCmd();
      ms->accelleration = 0;
      ms->targetSpeed  = sv->jerk;
      moveCommand((int16) (ms->curPos - (int16) (((uint16) rb[2] << 8) | rb[3])));
    }
  } else if (firstByte == 0x01) {
    // setPos command
    if (lenIs(3, false)) {
      ms->curPos =  (int16) (((uint16) rb[2] << 8) | rb[3]);
    }
  } else if (firstByte == 0x1f) {
    // load settings command
    uint8 numWords = (numBytesRecvd - 1) / 2;
    if ((numBytesRecvd & 0x01) == 1 &&
            numWords > 0 && numWords <= NUM_SETTING_WORDS) {
      setMotorSettings(numWords);
    } else {
      setError(CMD_DATA_ERROR);
    }
  } else if((firstByte & 0xfc) == 0x04) {
   // next status contains special value
    if (lenIs(1, true)) {
      ms->nextStateSpecialVal = (firstByte & 0x03) + 1;
    } else {
      setError(CMD_DATA_ERROR);
    }
  } else if ((firstByte & 0xf0) == 0x10) {

    uint8 bottomNib = firstByte & 0x0f;
    // one-byte commands
    if (lenIs(1, (bottomNib != 4 && bottomNib != 7))) {
      switch (bottomNib) {
        case 2: softStopCommand(false);      break; // stop,no reset
        case 3: softStopCommand(true);       break; // stop with reset
        case 4: resetMotor();                break; // hard stop (immediate reset)
        case 5: motorOnCmd();                break; // reset off
        default: setError(CMD_DATA_ERROR);
      }
    }
  } 
  else setError(CMD_DATA_ERROR);
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
