
#include "spi-protocol.h"
#include "parse-spi.h"
#include "motor.h"

// return number of leading ones in a 32-bit word
uint8_t numLeading1s(uint32_t *word) {
  char byt, mask = 0x80;
  if((byt = ((char *) word)[3]) != 0xff) {
    for(char count = 0; ; count++, mask >> 1) 
      if((byt & mask) == 0) return count;
  } 
  else if((byt = ((char *) word)[2]) != 0xff) {
    for(char count = 8; ; count++, mask >> 1) 
      if((byt & mask) == 0) return count;
  } 
  else if((byt = ((char *) word)[1]) != 0xff) {
    for(char count = 16; ; count++, mask >> 1) 
      if((byt & mask) == 0) return count;
  } 
  else {
    byt = ((char *) word)[0];
    for(char count = 24; count < 32; count++, mask >> 1) 
      if((byt & mask) == 0) return count;
    return 32;
  }
}

// returns axis from spi vector (X:0, Y:1)
char axisFromSpiWord(uint32_t *word) {
  switch (numLeading1s(word)) {
    case  1: return ((((char *) word)[3]) & 0x10) != 0;
    case  7: return ((((char *) word)[2]) & 0x80) != 0;
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

/*
typedef struct MoveState {
  uint8_t  dir;
  uint8_t  ustep;
  uint16_t pps;
  int8_t   acceleration;
  uint16_t curPulseCount;
  uint16_t pulsesInMove;
  int8_t   accells[10];
  uint8_t  accellsCount;
  uint8_t  accellsIdx;
  bool_t   done;
} MoveState;
*/

// parse accelleration, velocity, curve, or marker vector
// returns marker code, or zero if not marker
uint8_t  parseMove(MoveState *moveState){
  // marker
  if(spiInts[1] = 0xffff && (spiInts[0] & 0xfff0) == 0xfff0)    
    return (spiInts[0] & 0x000f);
    
  // velocity
  else  if((spiBytes[3] & 0xe0) == 0x80) {                      
    moveState->acceleration = 0;
    moveState->accellsIdx   = 0;
    moveState->ustep = (spiBytes[3] >> 1) & 0x07;
    int16_t pps = (int16_t) ((spiWord >> 12) & 0x1fff);
    if(pps & 0x01000) pps |= 0xe000;
    if(pps < 0) {
      pps = -pps;
      moveState->dir = 0;
    }
    else
      moveState->dir = 1;
    moveState->pps = (uint16_t) pps;
    moveState->pulseCount = spiInts[0] & 0x0fff;
  } 

  // acceleration
  else if (spiBytes[3] == 0xff && spiBytes[2] == 0xe0) {       
    moveState->accellsIdx = 0;
    moveState->ustep = (spiBytes[2] >> 4) & 0x07;
    moveState->acceleration = 
      (int8_t)(((spiBytes[2] & 0x0f) << 4) | (spiBytes[1] >> 4));
    moveState->pulseCount = spiInts[0] & 0x0fff;
  }
  
  // curve
  // each item value is sign-extended to int8_t
  // destroys global spiWord
  else {                          
    uint8_t  i, len, bits, mask, byteVal, signMask, extbits;
    switch (numLeading1s(&spiWord)) {
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
      for(i=len-1; i >= 0; i--, spiWord >>= bits) {
        byteVal = spiWord & mask;
        if(byteVal & signMask) byteVal |= extbits;
        moveState->accells[i] = (int8_t) byteVal;
      }
    }
    else {
      moveState->accells[0] = (int8_t *) spiBytes[1];
      moveState->accells[1] = (int8_t *) spiBytes[0];
    }
  }
  moveState->done = FALSE;
  return 0;
}

