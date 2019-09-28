#ifndef I2C_H
#define	I2C_H

#include <xc.h>
#include "types.h"
#include "motor.h"

// move commands start immediately even when moving
// all position and distance is in steps
// all speed is in steps/sec (except speed-move cmd)
// minimum system speed is 0.1 mm/sec => 8 1/80 mm
// all accelleration is in 1/80 mm/sec/sec

// steps are one phase (unipolar)
//       steps/rev:        2048
//       dist/rev:           40 mm
//       max distance:      625 mm
//       max step count: 32,000
//

// writes ...
//   1aaa aaaa  goto command, top 7 bits of goto addr
//      aaaa aaaa followed by bottom 8 bits
//   01ss ssss (speed-move cmd) set max speed = s*256 steps/sec, move to a
//     0aaa aaaa top 7 bits of move addr
//     aaaa aaaa bottom 8 bits
//   0001 0010  soft stop, deccelerates, no reset
//   0001 0011  soft stop, deccelerates first, then reset
//   0001 0100  hard stop (immediate reset)
//   0001 0101  motor on (reset off)
//   0001 0110  set curpos to home pos value setting
//   0001 0111  set regs, 16-bit values
//      max speed
//      no-accelleration speed limit (and start speed)
//      accelleration rate
//      home pos value
//
// Error codes 
//      2: i2c buffer overflow
//      3: i2c cksum error
//      4: command not done
//      7: bad command data (first byte invalid or length wrong)
//
// state bytes
//   vccc ebo0  state byte
//      v: version (1-bit)
//    ccc: error code (see above)
//      e: error bit
//      b: busy state
//      o: motor on (not in reset)
//   aaaa aaaa  current position, top 8 bits of signed 16-bit word
//   aaaa aaaa  followed by bottom 8 bits
//   cccc cccc  8-bit cksum, sum of first 5 bytes

#define NUM_RECV_BYTES NUM_SETTING_WORDS*2+4
#define NUM_SEND_BYTES 4  //  state, posH, posL, cksum

#define I2C_ADDR_MASK 0xf8  // motor idx in d3-d1 (d2-d0 in real addr)
#define I2C_ADDR      0x20  // real addr: 0x10 + motor #

extern volatile uint8 i2cRecvBytes[NUM_MOTORS][NUM_RECV_BYTES + 1];
extern volatile uint8 i2cRecvBytesPtr;
extern volatile uint8 i2cSendBytes[NUM_MOTORS][NUM_SEND_BYTES];
extern volatile uint8 i2cSendBytesPtr;

void i2cInit(void);
void checkI2c(void);
void setI2cCkSum(void);
void setI2cCkSumInt(uint8 motIdx);
void i2cInterrupt(void);

#endif	/* I2C_H */
