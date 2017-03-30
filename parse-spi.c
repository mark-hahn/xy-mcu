
#include <xc.h>
#include "spi-protocol.h"
#include "parse-spi.h"
#include "motor.h"
#include "main.h"

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

#ifdef XY
// returns axis from spi vector (X:0, Y:1)
char axisFromSpiWord(uint32_t *word) {
  switch (numLeading1s(word)) {
    case  1:
    case  7: return ((((char *) word)[1]) & 0x10) != 0;
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

// parse accelleration, velocity, delay, curve, or marker vector
// returns marker code, or zero if not marker
uint8_t  parseVector(uint32_t *vector, MoveState *moveState){
  
  dbg(1);
  
  uint16_t vecInts[2];
  vecInts[0] = *((uint16_t *) &((uint8_t *) vector)[2]);
  vecInts[1] = *((uint16_t *) &((uint8_t *) vector)[0]);
  
  moveState->delayUsecs = 0;
  
  // marker
  if(vecInts[1] == 0xffff && (vecInts[0] & 0xffe0) == 0xffc0)    
    return (vecInts[0] & 0x000f);
  
  // velocity
  else  if((((uint8_t *) vector)[3] & 0xe0) == 0x80) {                      
    moveState->acceleration = 0;
    moveState->accellsIdx   = 0;
    moveState->pulseCount   = (vecInts[0] & 0x0fff);
    if(moveState->pulseCount == 0) {
      moveState->delayUsecs = (vecInts[0] & 0xe000) | (vecInts[1] & 0x1fff);
      return 0;
    }
    moveState->ustep = (((uint8_t *) vector)[1] >> 5);
    moveState->dir   = ((((uint8_t *) vector)[3] & 0x10) != 0);
    moveState->pps   = (vecInts[1] & 0x0fff);
  } 

  // acceleration
  else if (((uint8_t *) vector)[3] == 0xff && ((uint8_t *) vector)[2] == 0xe0) {       
    moveState->accellsIdx = 0;
    moveState->ustep = (((uint8_t *) vector)[1] >> 5);
    moveState->acceleration = (int8_t)(vecInts[1] & 0x00ff);
    moveState->pulseCount   = vecInts[0] & 0x0fff;
  }
  
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
      moveState->accells[0] = ((uint8_t *) vector)[1];
      moveState->accells[1] = ((uint8_t *) vector)[0];
    }
  }
  moveState->done = FALSE;

  dbg(0);
  
  
  return 0;
}

/*                 ---- PROTOCOL ----

iiiiiii:        7-bit immediate cmd
a:              axis, X (0) or Y (1)
d:              direction (0: backwards, 1:forwards)
uuu:            microstep, 0 (1x) to 5 (32x)
xxxxxxxx:        8-bit signed acceleration in pulses/sec/sec
vvvvvvvvvvvv:   12-bit velocity in pulses/sec
cccccccccccc:   12-bit pulse count
E-M: curve acceleration field, signed
zzzz: vector list markers
  15: eof, end of moving


Number before : is number of leading 1's
 
 0:  0iii iiii  -- 7-bit immediate cmd - more bytes may follow
 1:  100d vvvv vvvv vvvv uuua cccc cccc cccc  -- velocity vector  (1 unused bit)
     if pulse count is zero then uuudvvvvvvvvvvvv is 16-bit usecs delay (not pps)
 
 7:  1111 1110 xxxx xxxx uuua cccc cccc cccc  -- acceleration vector

Curve vectors, each field is one pulse of signed acceleration ...

 3:  1110 aEEE  FFFG GGHH  HIII JJJK  KKLL LMMM --  9 3-bit
 6:  1111 110a  FFFG GGHH  HIII JJJK  KKLL LMMM --  8 3-bit
 2:  110a EEEE  FFFF GGGG  HHHH IIII  JJJJ KKKK --  7 4-bit
 5:  1111 100a  EEEE FFFF  GGGG HHHH  IIII JJJJ --  6 4-bit (1 unused bit)
 4:  1111 00aF  FFFF GGGG  GHHH HHII  IIIJ JJJJ --  5 5-bit (1 unused bit)
10:  1111 1111  110a FFFF  FGGG GGHH  HHHI IIII --  4 5-bit
 9:  1111 1111  10aF FFFF  FFGG GGGG  GHHH HHHH --  3 7-bit
14:  1111 1111  1111 110a  GGGG GGGG  HHHH HHHH --  2 8-bit

26:  1111 1111 1111 1111 1111 1111 110a zzzz  -- 4-bit vector marker

Sample Calculations (unfinished) ...

Typical case:

1000 mm/sec/sec =>  mm/ms/sec, assuming 1000 pps this is 1 mm/sec of velocity change each ms.  To get to a speed of 100 mm/sec, it would take 100 ms in 100 pulses.  This would cover 10 mm.

at 3000 mm/sec/sec, 3mm/ms/sec, and 3000 pps, it would be 3mm/sec vel increment. to get to 270 mm/sec would require 90 steps in 30 ms, covering 5000 (185*90) mm, or 15 meters.

given 
  1) accel: mm/sec/sec  constant acceleration, typ. 1000
  2) vel:   mm/sec      end target speed,      typ. 100
  3) ppmm:  pulses/mm   constant ratio,        typ. 20  (1/4 step per pulse)

  vel*ppmm => pps: end pulses/sec  typ. 2000
  avg pps = 1000
  

each pulse:
  1ms -> 1mm/ms  0.5 mm
  2ms -> 2mm/ms  2 mm
  3ms -> 3mm/ms  4.5
  4ms -> 4mm/ms  8

  time: ms => 
  
avg vel: 50 mm/sec
avg pps: 1000


pps pulse rate (final update rate), you get mm/sec vel inc, number of steps, time, and distance.


10-bit (was 8) signed acceleration in PPS/sec, 3000 mm/s/s => 3mm/ms incr which is at most

12-bit pulse count 
 */
