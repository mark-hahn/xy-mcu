
#ifndef VECTORS_H
#define	VECTORS_H
 
#define VEC_BUF_SIZE     100 // number of 32-bit vector commands buffered
#define VEC_BUF_HI_WATER  80 // flow control limit

void initVectors();
void handleNewSpiWord();
 
#endif	/* VECTORS_H */
