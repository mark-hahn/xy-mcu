
#ifndef INVTABLE_H
#define	INVTABLE_H

#include <stdint.h>

// pps is pulses/sec in fixed-point 9.6 format
// pps2usecs converts pps to usecs in custom floating point
// min is 2 usecs and max is 8e6 usecs (8 secs)
// num of usecs is ((2^14) + man)*(2^(exp-15));
typedef struct UsecsFp {
  uint8_t  exp; // exponent,  3-bits, 0 to -7 (negate exp)
  uint16_t man; // mantissa, 14-bits, approximately 2^13 to 2^14
} UsecsFp;

extern UsecsFp usecsFp;  // global set by pps2usecs call

void initInvtable();
UsecsFp pps2usecs(uint16_t pps);

#endif

