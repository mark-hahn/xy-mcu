
#ifndef MAIN_H
#define	MAIN_H

/*
 * TODO  Notes ...
 *   limit switch error when taking off from home?
 */

#define _XTAL_FREQ 32000000 

#define X 0  /* idx for X axis */
#define Y 1  /* idx for Y axis */

// only XC8 compiler is supported for these types

typedef char bool_t;
#define TRUE 1
#define FALSE 0

// time, unit: usec
// max time is 71 minutes
typedef unsigned long time_t;     // 32 bits unsigned
typedef unsigned int shortTime_t; // 16 bits unsigned

// position, unit: 0.00625 mm, 1/32 step distance (smallest microstep)
// max position is +- 52 meters
typedef signed short long pos_t; // 24 bits signed

// direction, +1 or -1
typedef signed char dir_t;  // 8 bits signed

#endif	/* MAIN_H */

