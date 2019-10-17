  
#ifndef PINS_H
#define	PINS_H

// debug pin is L0, RA4;  use dbg0 and dbg1
//#define debug


#define sclTRIS TRISB4
#define sdaTRIS TRISB1
#define sclLAT  LATB4
#define sdaLAT  LATB1

#define z0TRIS TRISA0
#define z1TRIS TRISA1
#define z2TRIS TRISA2
#define z3TRIS TRISA3

#define l0TRIS TRISA4
#define l1TRIS TRISA6
#define l2TRIS TRISA7
#define l3TRIS TRISB7

#define p0TRIS TRISB0
#define p1TRIS TRISB2
#define p2TRIS TRISB3
#define p3TRIS TRISB5

#define z0LAT LATA0
#define z1LAT LATA1
#define z2LAT LATA2
#define z3LAT LATA3

#define l0LAT LATA4
#define l1LAT LATA6
#define l2LAT LATA7
#define l3LAT LATB7

#define p0LAT LATB0
#define p1LAT LATB2
#define p2LAT LATB3
#define p3LAT LATB5

#define z0PORT PORTA
#define z1PORT PORTA
#define z2PORT PORTA
#define z3PORT PORTA

#define l0PORT PORTA
#define l1PORT PORTA
#define l2PORT PORTA
#define l3PORT PORTB

#define p0PORT PORTB
#define p1PORT PORTB
#define p2PORT PORTB
#define p3PORT PORTB

#define z0BIT 0x01
#define z1BIT 0x02
#define z2BIT 0x04
#define z3BIT 0x08

#define l0BIT 0x10
#define l1BIT 0x40
#define l2BIT 0x80
#define l3BIT 0x80

#define p0BIT 0x01
#define p1BIT 0x04
#define p2BIT 0x08
#define p3BIT 0x20

#ifdef debug
//#define dbg0 l0LAT = 1;
//#define dbg1 l0LAT = 0;
#else
#define dbg0
#define dbg1
#endif


#endif	/* PINS_H */

