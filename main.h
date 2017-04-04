
#ifndef MAIN_H
#define	MAIN_H

#include "mcu-cpu.h"

#define MFR      1   // eridien
#ifdef XY
#define PROD     1   // XY base
#endif
#ifdef Z2
#define PROD     2   // XY base
#endif

#define VERS     1   // XY base product (code or hw) version

#define _XTAL_FREQ 32000000 

extern volatile bool_t spiInt;
extern volatile bool_t CCP1Int;
#ifdef XY
extern volatile bool_t CCP2Int;
#endif
extern volatile char   intError;

#ifdef XY
#define dbgTris // TRISC7
#define dbgPin  // RC7
#endif

#ifdef Z2
#define dbgTris TRISB7
#define dbgPin  RB7
#endif

#define initDbg()    // dbgTris = 0
#define dbg(x)       // dbgPin = x
#define dbgToggle()  // dbgPin = !dbgPin
#define dbgPulseH(n) // for(int _n=0; _n<n; _n++) {dbgPin = 1; dbgPin = 0;}
#define dbgPulseL(n) // for(int _n=0; _n<n; _n++) {dbgPin = 0; dbgPin = 1;}

#endif	/* MAIN_H */

