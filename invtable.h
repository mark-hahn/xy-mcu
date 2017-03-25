
#ifndef INVTABLE_H
#define	INVTABLE_H

#include <stdint.h>

// invtable is defined in invtable.asm
extern const uint16_t invtable1;
extern const uint16_t invtable2;

uint16_t pps2usecs(uint16_t pps);

#endif

