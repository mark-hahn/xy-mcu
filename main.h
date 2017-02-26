
#ifndef MAIN_H
#define	MAIN_H

#include "mcu-cpu.h"

#define MFR      1   // eridien
#define PROD     1   // XY base
#define VERS     1   // XY base product (code or hw) version

#define _XTAL_FREQ 32000000 

extern volatile bool_t spiInt;
extern volatile bool_t CCP1Int;
extern volatile bool_t CCP2Int;
extern volatile char   intError;

#endif	/* MAIN_H */

