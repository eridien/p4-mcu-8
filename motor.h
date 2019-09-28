
#ifndef MOTOR_H
#define	MOTOR_H

#include <xc.h>
#include "types.h"

#define NUM_MOTORS 3

// steps are one phase (unipolar)
//       steps/rev:        2048
//       dist/rev:           40 mm
//       max distance:      625 mm
//       max step count: 32,000
#define STEPS_MM = (2048/40)

// global for use in main chk loop
extern uint8  motorIdx;
extern struct motorState      *ms;
extern struct motorSettings   *sv;

extern volatile unsigned char *motorPort[NUM_MOTORS][4];  // mcu port byte
extern uint8                   motorMask[NUM_MOTORS][4];  // bit mask

struct motorState {
  uint8  stateByte;
  int16  curPos;
  int16  targetPos;
  bool   dir;
  bool   targetDir;
  uint8  phase;
  uint16 speed;
  uint16 targetSpeed;
  bool   stepPending;
  bool   stepped;
  uint16 nextStepTicks;
  uint16 lastStepTicks;
  bool   resetAfterSoftStop;
  bool   i2cCmdBusy;
} mState[NUM_MOTORS];

#define POS_UNKNOWN_CODE -9999 // not homed since lastmotor off

// constants loadable from command
struct motorSettings {
  uint16 maxSpeed;
  uint16 minSpeed;
  uint16 noAccelSpeedLimit;
  uint16 accellerationRate;
  uint16 homePos;
};

#define NUM_SETTING_WORDS 5

union settingsUnion{
  uint16 reg[NUM_SETTING_WORDS];
  struct motorSettings val;
} mSet[NUM_MOTORS];

void motorInit(void);
void chkMotor(void);
bool withinDecellDist(void);
void softStopCommand(bool reset);
void setStep(void);
void stopStepping(void);
void resetMotor(void);
void motorOnCmd(void);
void processMotorCmd(void);
void clockInterrupt(void);
uint16 getLastStep(void);
void setNextStep(uint16 ticks);

#endif	/* MOTOR_H */

