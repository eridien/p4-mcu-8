
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
//      s: stopping
//   aaaa aaaa  current position, top 8 bits of signed 16-bit word
//   aaaa aaaa  followed by bottom 8 bits
//   cccc cccc  8-bit cksum, sum of first 5 bytes

// Error codes 
#define MOTOR_FAULT_ERROR   0x10
#define OVERFLOW_ERROR      0x20
#define CMD_DATA_ERROR      0x30
#define STEP_NOT_DONE_ERROR 0x40
#define BOUNDS_ERROR        0x50
#define NO_SETTINGS         0x60
#define NOT_HOMED           0x70
#define CLEAR_ERROR         0xff // magic code to clear error

// state byte
#define ERR_CODE            0x70
#define AUX_RES_BIT         0x08 // do-d1 indicate what is in pos word
#define BUSY_BIT            0x04
#define MOTOR_ON_BIT        0x02
#define HOMED_BIT           0x01

void  setStateBit(uint8 mask, uint8 set);
void  setError(uint8 err);
void  setErrorInt(uint8 motorIdx, uint8 err);
void  intsOff(void);
void  intsOn(void);

#endif	/* STATE_H */

