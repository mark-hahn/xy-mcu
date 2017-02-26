
#ifndef VECTORS_H
#define	VECTORS_H
 
#include "main.h"
#include "mcu-cpu.h"

#define VEC_BUF_SIZE     100 // number of 32-bit vector cmds buffered per axis
#define VEC_BUF_HI_WATER  95 // flow control limit

extern Vector vecBufX[VEC_BUF_SIZE];
extern Vector vecBufY[VEC_BUF_SIZE];

extern Vector *vecBufHeadX;
extern Vector *currentVectorX;
extern Vector *vecBufHeadY;
extern Vector *currentVectorY;

void initVectors();
void putVectorX();
void putVectorY();
bool_t vecBufXIsAtHighWater();
bool_t vecBufYIsAtHighWater();

#endif	/* VECTORS_H */
