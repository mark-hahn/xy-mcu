
#ifndef VECTORS_H
#define	VECTORS_H

#define VEC_BUF_SIZE     64 // 256/4, allows byte masking and modulo arithmetic
#define VEC_BUF_HI_WATER 48 // flow control limit

void initVectors();
void handleNewVecBufByte();

#endif	/* VECTORS_H */

