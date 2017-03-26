
#ifndef SPI_PROTOCOL_H
#define	SPI_PROTOCOL_H

#define IMM_CMD_MASK 0x80
#define isImmCmd(byte) ((byte & IMM_CMD_MASK) == 0)



#endif	/* SPI_PROTOCOL_H */

/*
iiiiiii:        7-bit immediate cmd
a:              axis, X (0) or Y (1)
uuu:            microstep, 0 (1x) to 5 (32x)
xxxxxxxx:        8-bit signed acceleration in pulses/sec/sec
vvvvvvvvvvvvv:  13-bit signed velocity in pulses/sec
cccccccccccc:   12-bit pulse count
E-M: curve acceleration field, signed
zzzz: vector list markers
  14: sync, wait for all MCUs to sync
  15: eof, end of moving
`

Number before : is number of leading 1's
 
 0:  0iii iiii  -- 7-bit immediate cmd - more bytes may follow
 1:  100a uuuv vvvv vvvv vvvv cccc cccc cccc    (1 unused bit)
         -- velocity vector, 13-bit signed velocity in PPS, 
                             12-bit pulse count
 7:  1111 1110 auuu xxxx xxxx cccc cccc cccc  -- acceleration vector

Curve vectors, each field is one pulse of signed acceleration ...

 3:  1110 aEEE FFFG GGHH HIII JJJK KKLL LMMM --  9 3-bit
 6:  1111 110a FFFG GGHH HIII JJJK KKLL LMMM --  8 3-bit
 2:  110a EEEE FFFF GGGG HHHH IIII JJJJ KKKK --  7 4-bit
 5:  1111 100a EEEE FFFF GGGG HHHH IIII JJJJ --  6 4-bit (1 unused bit)
 4:  1111 00aF FFFF GGGG GHHH HHII IIIJ JJJJ --  5 5-bit (1 unused bit)
10:  1111 1111 110a FFFF FGGG GGHH HHHI IIII --  4 5-bit
 9:  1111 1111 10aF FFFF FFGG GGGG GHHH HHHH --  3 7-bit
14:  1111 1111 1111 110a GGGG GGGG HHHH HHHH --  2 8-bit
26:  1111 1111 1111 1111 1111 1111 110a zzzz  -- 4-bit vector marker

Sample Calculations ...
 

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