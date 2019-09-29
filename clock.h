
#ifndef CLOCK_H
#define	CLOCK_H

#define CLK_RATE 10000 // ticks/sec, 100 usecs/tick, 10 KHz

extern volatile uint16 timeTicks; // units: 100 usecs, wraps on 6.5 secs

void   clkInit(void);

#endif	/* CLOCK_H */

