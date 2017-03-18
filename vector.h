
#ifndef VECTORS_H
#define	VECTORS_H
 
#include "main.h"
#include "mcu-cpu.h"

#define VEC_BUF_SIZE     100 // number of 32-bit vector cmds buffered per axis
#define VEC_BUF_HI_WATER  95 // flow control limit

void initVectors();

void putVectorX();
Vector *getVectorX();
bool_t haveVectorsX();
bool_t vecBufXIsAtHighWater();
#ifdef XY
void putVectorY();
Vector *getVectorY();
bool_t haveVectorsY();
bool_t vecBufYIsAtHighWater();
#endif
#endif	/* VECTORS_H */
