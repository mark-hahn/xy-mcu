
#include <xc.h>
#include "spi-protocol.h"
#include "parse-spi.h"
#include "motor.h"
#include "main.h"

// return number of leading ones in a 32-bit word
uint8_t numLeading1s(uint32_t *word) {
  char byt, mask = 0x80;
  if((byt = ((char *) word)[3]) != 0xff) {
    for(char count = 0; ; count++, mask >>= 1) 
      if((byt & mask) == 0) return count;
  } 
  else if((byt = ((char *) word)[2]) != 0xff) {
    for(char count = 8; ; count++, mask >>= 1) 
      if((byt & mask) == 0) return count;
  } 
  else if((byt = ((char *) word)[1]) != 0xff) {
    for(char count = 16; ; count++, mask >>= 1) 
      if((byt & mask) == 0) return count;
  } 
  else {
    byt = ((char *) word)[0];
    for(char count = 24; count < 32; count++, mask >>= 1) 
      if((byt & mask) == 0) return count;
    return 32;
  }
}

#ifdef XY
// returns axis from spi vector (X:0, Y:1)
char axisFromSpiWord(uint32_t *word) {
  switch (numLeading1s(word)) {
    case  0: return ((((char *) word)[1]) & 0x10) != 0;
    case  3: return ((((char *) word)[3]) & 0x08) != 0;
    case  6: return ((((char *) word)[3]) & 0x01) != 0;
    case  2: return ((((char *) word)[3]) & 0x10) != 0;
    case  5: return ((((char *) word)[3]) & 0x01) != 0;
    case  4: return ((((char *) word)[3]) & 0x02) != 0;
    case 10: return ((((char *) word)[2]) & 0x10) != 0;
    case  9: return ((((char *) word)[2]) & 0x20) != 0;
    case 14: return ((((char *) word)[2]) & 0x01) != 0;
    case 26: return ((((char *) word)[0]) & 0x10) != 0;
  }
  return 0;  // shouldn't get here
}
#endif

// parse settings, move, delay, curve, or marker vector
// returns marker code, or zero if not marker
uint8_t  parseVector(uint32_t *vector, MoveState *moveState){
  
//  dbgPulseH(2);
  
  uint16_t vecInts[2];
  vecInts[0] = *((uint16_t *) &((uint8_t *) vector)[0]);
  vecInts[1] = *((uint16_t *) &((uint8_t *) vector)[2]);
  uint8_t topByte = ((uint8_t *) vector)[3];

  // settings/move
  if((topByte & 0x80) == 0) {    
    moveState->acceleration = 0;
    moveState->accellsIdx   = 0;
    if((topByte & 0x40) != 0) {                 // settings
      moveState->acceleration = (vecInts[0] & 0x00ff);
      moveState->currentPps   = (vecInts[1] & 0x0fff);
    }
    else {                                      // move
      moveState->pulseCount = (vecInts[0] & 0x0fff);
      if(moveState->pulseCount == 0) {
        moveState->delayUsecs = (vecInts[0] & 0xe000) | (vecInts[1] & 0x1fff);
        return 0;
      }
      moveState->targetPps = (vecInts[1] & 0x0fff);
      moveState->accelSign = (moveState->targetPps < moveState->currentPps);
    }
    moveState->ustep = (((uint8_t *) vector)[1] >> 5);
    moveState->dir   = ((topByte & 0x10) != 0);
  } 
  
  // marker
  else if(vecInts[1] == 0xffff && (vecInts[0] & 0xffe0) == 0xffc0)    
    return (vecInts[0] & 0x000f);
  
  // curve
  // each item value is sign-extended to int8_t
  else {
    uint32_t vec = *vector;
    uint8_t  i, len, bits, mask, byteVal, signMask, extbits;
    switch (numLeading1s(&vec)) {
      case  3: 
        len  = 9;
        bits = 3;
        mask  = 0x07;
        break;
      case  6: 
        len  = 8;
        bits = 3;
        mask  = 0x07;
        break;
      case  2: 
        len  = 7;
        bits = 4;
        mask  = 0x0f;
        break;
      case  5: 
        len  = 6;
        bits = 4;
        mask  = 0x0f;
        break;
      case  4: 
        len  = 5;
        bits = 5;
        mask  = 0x1f;
        break;
      case 10: 
        len  = 4;
        bits = 5;
        mask  = 0x1f;
        break;
      case  9: 
        len  = 3;
        bits = 7;
        mask  = 0x7f;
        break;
      case 14: 
        len  = 2;
  //    bits = 8;
  //    mask  = 0xff;
        break;
    }
    // dir, ustep, and pps are unchanged from last vector
    moveState->pulseCount = 0;
    moveState->accellsIdx = len;  
    if(len != 2) {
      signMask = (mask+1) >> 1;
      extbits  = ~mask;
      for(int idx =len-1; idx >= 0; idx--, vec >>= bits) {
        byteVal = vec & mask;
        if(byteVal & signMask) byteVal |= extbits;
        moveState->accells[idx] = (int8_t) byteVal;
      }
    }
    else {
      moveState->accells[0] = ((int8_t *) vector)[1];
      moveState->accells[1] = ((int8_t *) vector)[0];
    }
  }

//  dbgPulseH(3);
  
  return 0;
}
