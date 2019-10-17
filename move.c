
#include <xc.h>
#include "types.h"
#include "pins.h"
#include "move.h"
#include "state.h"
#include "motor.h"

void chkMoving() {
  // in the process of stepping

  if(ms->stepPending || ms->stepped || haveError()) return;
  
  if((ms->curPos == ms->targetPos) && underJerkSpeed()) {
    stopStepping();
    return;
  }
  
  ms->targetDir = (ms->targetPos >= ms->curPos);

  // check ms->speed/acceleration
  if((!underJerkSpeed() && withinDecellDist()) || (ms->dir != ms->targetDir))
    // decellerate
    ms->speed -= ms->accelleration/ms->speed;
  else if(ms->targetSpeed > ms->speed) {
    // accelerate
    ms->speed += ms->accelleration/ms->speed;
  }
  if(ms->speed > ms->targetSpeed) ms->speed = ms->targetSpeed;
  if(ms->speed < sv->jerk)        ms->speed = sv->jerk;

  // check direction
  if(ms->dir != ms->targetDir && underJerkSpeed()) {
    // slow enough, change ms->dir
    ms->dir = ms->targetDir;
  }
  setStep();
}

void moveCommand(int16 pos) {
  ms->targetPos = pos;
  if(!underJerkSpeed()) {
    // already moving fast, keep going same way
    chkMoving();
  }
  else if(ms->curPos != ms->targetPos) {
    ms->dir = (ms->targetPos >= ms->curPos);
    // start moving
    setStateBit(BUSY_BIT, 1);
    ms->stopping = false;
    chkMoving();
  }
  else {
    // at target
    stopStepping();
  }
}

