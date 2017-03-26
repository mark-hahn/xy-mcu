
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
  uint32_t *vec = currentVectorX;
  if(currentVectorX == vecBufHeadX)
    // vector buf is empty
    handleError(X, errorVecBufUnderflow);
  if(++currentVectorX == vecBufX + VEC_BUF_SIZE) 
       currentVectorX = vecBufX;
  return vec;
}
bool_t haveVectorsX() {
  return (currentVectorX != vecBufHeadX);
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
  uint32_t *vec = currentVectorY;
  if(currentVectorY == vecBufHeadY)
    // vector buf is empty
    handleError(Y, errorVecBufUnderflow);
  if(++currentVectorY == vecBufY + VEC_BUF_SIZE) 
       currentVectorY = vecBufY;
  return vec;
}
bool_t haveVectorsY() {
  return (currentVectorY != vecBufHeadY);
}
bool_t vecBufYIsAtHighWater() {
  int diff = (vecBufHeadY - currentVectorY);
  if(diff < 0) diff += VEC_BUF_SIZE;
  return (diff >= VEC_BUF_HI_WATER);
}
#endif

