
#ifndef STATE_H
#define	STATE_H

#include "types.h"
#include "motor.h"

#define haveError() (ms->stateByte & 0x70)

// stateBytes
//   veee bboz  state byte
//      v: version (1-bit)
//    eee: error code (see above)
//      e: error bit
//      b: busy state
//      o: motor on (not in reset)
//      z: at home
//   aaaa aaaa  current position, top 8 bits of signed 16-bit word
//   aaaa aaaa  followed by bottom 8 bits
//   hhhh hhhh  home test position val, top 8 bits of signed 16-bit word
//   shhh hhhh  home test position val, bottom 8 bits
//   cccc cccc  8-bit cksum, sum of first 5 bytes

// Error codes 
#define I2C_OVERFLOW_ERROR  0x20
#define CMD_DATA_ERROR      0x30
#define CMD_NOT_DONE_ERROR  0x40
#define STEP_NOT_DONE_ERROR 0x50
#define MOTOR_LIMIT_ERROR   0x60

// state bits
#define ERROR_BIT           0x08
#define MOVING_BIT          0x04
#define MOTOR_ON_BIT        0x02
#define STOPPING_BIT        0x01

void  setCurState(uint8 newState);
void  setStateBit(uint8 mask, uint8 set);
void  setError(uint8 err);
void  setErrorInt(uint8 motorIdx, uint8 err);

#endif	/* STATE_H */

