
#include <xc.h>
#include "vector.h"
#include "spi.h"
#include "event.h"
#include "pins.h"


// separate vector buffers for X and Y
uint32_t vecBufX[VEC_BUF_SIZE];
uint32_t *vecBufHeadX, *currentVectorX;
#ifdef XY
uint32_t vecBufY[VEC_BUF_SIZE];
uint32_t *vecBufHeadY, *currentVectorY;
#endif


void initVectors() {
  vecBufHeadX = currentVectorX = vecBufX;
#ifdef XY
  vecBufHeadY = currentVectorY = vecBufY;
#endif
}

void putVectorX() {
  *vecBufHeadX = spiWord;
  if (++vecBufHeadX == vecBufX + VEC_BUF_SIZE) vecBufHeadX = vecBufX;
  if (vecBufHeadX == currentVectorX) handleError(X, errorVecBufOverflow);
}

uint32_t *getVectorX() {
  if(currentVectorX == vecBufHeadX) return (uint32_t *) 0;
  uint32_t *vec = currentVectorX;
  if(++currentVectorX == vecBufX + VEC_BUF_SIZE) 
       currentVectorX = vecBufX;
  return vec;
}

bool_t vecBufXIsAtHighWater() {
  int diff = (vecBufHeadX - currentVectorX);
  if (diff < 0 ) diff += VEC_BUF_SIZE;
  return (diff >= VEC_BUF_HI_WATER);
}

#ifdef XY
void putVectorY() {
  *vecBufHeadY = spiWord;
  if (++vecBufHeadY == vecBufY + VEC_BUF_SIZE) vecBufHeadY = vecBufY;
  if (vecBufHeadY == currentVectorY) handleError(Y, errorVecBufOverflow);
}

uint32_t *getVectorY() {
  if(currentVectorY == vecBufHeadY) return (uint32_t *) 0;
  uint32_t *vec = currentVectorY;
  if(++currentVectorY == vecBufY + VEC_BUF_SIZE) 
       currentVectorY = vecBufY;
  return vec;
}

bool_t vecBufYIsAtHighWater() {
  int diff = (vecBufHeadY - currentVectorY);
  if(diff < 0) diff += VEC_BUF_SIZE;
  return (diff >= VEC_BUF_HI_WATER);
}
#endif

