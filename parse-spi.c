
#include "spi-protocol.h"
#include "parse-spi.h"

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

// returns accelerations from packed spi word
// first accels item is number of packed values (len)
// each item value is sign-extended to int8_t
void accelsFromSpiWord(uint32_t *word, int8_t *accels) {
  uint32_t wordVal = *word;
  uint8_t  i, len, bits, mask, byteVal, signMask, extbits;
  switch (numLeading1s(word)) {
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
  accels[0] = len;  
  if(len != 2) {
    signMask = (mask+1) >> 1;
    extbits  = ~mask;
    for(i=len; i > 0; i--, wordVal >>= bits) {
      byteVal = wordVal & mask;
      if(byteVal & signMask) byteVal |= extbits;
      accels[i] = (int8_t) byteVal;
    }
  }
  else {
    accels[1] = ((int8_t *) word)[1];
    accels[2] = ((int8_t *) word)[0];
  }
}


