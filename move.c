
#include <xc.h>
#include "types.h"
#include "pins.h"
#include "clock.h"
#include "move.h"
#include "state.h"
#include "motor.h"

void chkMoving() {
  // in the process of stepping
  if(ms->stepPending || ms->stepped || haveError()) return;
  
  if((ms->curPos == ms->targetPos) && (ms->speed <= sv->jerkSpeed)) {
    stopStepping();
    return;
  }
  
  ms->targetDir = (ms->targetPos >= ms->curPos);
  
  // check ms->speed/acceleration
  uint16 accel = ms->accelleration/ms->speed;
  
  if(((ms->speed > sv->jerkSpeed) && withinDecellDist()) || 
      (ms->dir != ms->targetDir)) {
    // decellerate
    if(accel >= ms->speed) accel = ms->speed;
    ms->speed -= accel;
  }
  else if(ms->targetSpeed > ms->speed) {
    // accelerate
    ms->speed += accel;
  }
  if(ms->speed > ms->targetSpeed) ms->speed = ms->targetSpeed;

  // check direction
  if(ms->dir != ms->targetDir && (ms->speed <= sv->jerkSpeed)) {
    // slow enough, change ms->dir
    ms->dir = ms->targetDir;
  }
  setStep();
}

void moveCommand(int16 pos) {
  ms->targetPos = pos;
  if((ms->stateByte & BUSY_BIT) && (ms->speed > sv->jerkSpeed)) {
    // already moving fast, keep going same way
    chkMoving();
  }
  else if(ms->curPos != ms->targetPos) {
    ms->dir = (ms->targetPos >= ms->curPos);
    // start moving
    ms->stopping = false;
    ms->speed = ms->targetSpeed;
    if((ms->speed > sv->jerkSpeed)) ms->speed = sv->jerkSpeed;
    ms->lastStepTicks = timeTicks;
    setStateBit(BUSY_BIT, 1);
    chkMoving();
  }
  else {
    // at target
    stopStepping();
  }
}

