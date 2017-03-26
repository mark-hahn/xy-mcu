
#include "spi-protocol.h"
#include "parse-spi.h"

// return number of leading ones in a 32-bit word
char numLeading1s(uint32_t *word) {
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
    for(char count = 24; ; count++, mask >> 1) 
      if((byt & mask) == 0) return count;
  }
}

// returns axis from spi vector (X:0, Y:1)
char axisFromSpiWord(uint32_t *word) {
  switch (numLeading1s(word)) {
    case  1: return (*word & 0x10000000) != 0;
    case  7: return (*word & 0x00800000) != 0;
    case  3: return (*word & 0x08000000) != 0;
    case  6: return (*word & 0x01000000) != 0;
    case  2: return (*word & 0x10000000) != 0;
    case  5: return (*word & 0x01000000) != 0;
    case  4: return (*word & 0x02000000) != 0;
    case 10: return (*word & 0x00100000) != 0;
    case  9: return (*word & 0x00200000) != 0;
    case 14: return (*word & 0x00010000) != 0;
    case 26: return (*word & 0x00000010) != 0;
  }
}

// returns accelerations from packed spi word
// first array item is number of packed values (len)
// second item is offset (ofs), e.g. accel=2, ofs=4, actual accel is -2
void accelsFromSpiWord(uint32_t word, uint8_t *accels) {
  uint32_t mask;
  char i, len, ofs, bits, mask;
  switch (numLeading1s(word)) {
    case  3: 
      len  = 9;
      bits = 3;
      ofs  = 4;
      mask  = 0x07;
      break;
    case  6: 
      len  = 8;
      bits = 3;
      ofs  = 4;
      mask  = 0x07;
      break;
    case  2: 
      len  = 7;
      bits = 4;
      ofs  = 8;
      mask  = 0x0f;
      break;
    case  5: 
      len  = 6;
      bits = 4;
      ofs  = 8;
      mask  = 0x0f;
      break;
    case  4: 
      len  = 5;
      bits = 5;
      ofs  = 16;
      mask  = 0x1f;
      break;
    case 10: 
      len  = 4;
      bits = 5;
      ofs  = 16;
      mask  = 0x1f;
       break;
    case  9: 
      len  = 3;
      bits = 7;
      ofs  = 64;
      mask  = 0x7f;
      break;
    case 14: 
      len  = 2;
      bits = 8;
      ofs  = 128;
      mask  = 0xff;
      break;
  }
  accels[0] = len;  
  accels[1] = ofs;
  for(i=len+1; i > 1; i--, word >>= bits)
    accels[i] = word & mask;
}


