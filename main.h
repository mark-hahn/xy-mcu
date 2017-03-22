
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

#define initDbg()   LIMIT_SW_X_TRIS = 0
#define dbg(x)      LIMIT_SW_X = x
#define dbgToggle() LIMIT_SW_X = !LIMIT_SW_X


#endif	/* MAIN_H */

