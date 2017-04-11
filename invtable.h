
#ifndef INVTABLE_H
#define	INVTABLE_H

#include <stdint.h>

// usecs in custom floating point
// min is 2 usecs and max is 8e6 usecs (8 secs)
// num of usecs is man*(2^exp);
typedef struct UsecsFp {
  int8_t   exp; // exponent -12 to +9
  uint16_t man; // mantissa approximately 2^13 to 2^14
} UsecsFp;

void initInvtable();
UsecsFp pps2usecs(uint16_t pps);

#endif

