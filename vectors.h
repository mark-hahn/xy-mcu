
#ifndef VECTORS_H
#define	VECTORS_H
 
#include "main.h"

#define VEC_BUF_SIZE     100 // number of 32-bit vector commands buffered
#define VEC_BUF_HI_WATER  80 // flow control limit

void initVectors();
void handleNewSpiWord();
bool_t vecBufXIsAtHighWater();
bool_t vecBufYIsAtHighWater();

#endif	/* VECTORS_H */
