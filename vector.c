
#include <xc.h>
#include "vector.h"
#include "spi.h"
#include "event.h"

// separate vector buffers for X and Y
Vector vecBufX[VEC_BUF_SIZE];
Vector vecBufY[VEC_BUF_SIZE];

Vector *vecBufHeadX, *currentVectorX, *vecBufHeadY, *currentVectorY;

void initVectors() {
  vecBufHeadX = currentVectorX = vecBufX;
  vecBufHeadY = currentVectorY = vecBufY;
}
void putVectorX() {
  vecBufHeadX->ctrlWord      = spiInts[0];
  vecBufHeadX->usecsPerPulse = spiInts[1];
  if (++vecBufHeadX == vecBufX + VEC_BUF_SIZE) vecBufHeadX = vecBufX;
  if (vecBufHeadX == currentVectorX) handleError(X, errorVecBufOverflow);
}

void putVectorY() {
    vecBufHeadY->ctrlWord      = spiInts[0];
    vecBufHeadY->usecsPerPulse = spiInts[1];
    if (++vecBufHeadY == vecBufY + VEC_BUF_SIZE) vecBufHeadY = vecBufY;
    if (vecBufHeadY == currentVectorY) handleError(Y, errorVecBufOverflow);
}

bool_t vecBufXIsAtHighWater() {
  int diff = (vecBufHeadX - currentVectorX);
  if (diff < 0 ) diff += VEC_BUF_SIZE;
  return (diff >= VEC_BUF_HI_WATER);
}

bool_t vecBufYIsAtHighWater() {
  int diff = (vecBufHeadY - currentVectorY);
  if(diff < 0) diff += VEC_BUF_SIZE;
  return (diff >= VEC_BUF_HI_WATER);
}