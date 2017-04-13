
#ifndef MCU_API_H
#define	MCU_API_H

// This file contains all definitions shared by CPU and MCU
// and should be included in CPU and MCU apps
// If this file must change, make sure it is backwards compatible
// Keep this file in both apps the same (except for MCU_H and CPU_H defines)

#define XY 0  // XY mcu
#define Z  1  // Z  mcu
#define X  0  // X axis
#define Y  1  // Y axis

#define FORWARD  1 // motor dir bit
#define BACKWARD 0

//typedef uint8_t bool_t;
#define TRUE  1
#define FALSE 0

enum Error2 {
  noError = 0,
  errorReset2,  // always in this state after mcu reset
  bufOvflwX,
  bufOvflwY
}

// ustep (microstep) matches chip MS1, MS2, and MS3 everywhere in MCU

///////////////  SPI BYTES FROM CPU TO MCU  ///////////////

// need to handle limit sw already up or down  -- TODO

// ===== immediate commands =====

// 0000 0000 no-op, just for getting return byte
// 0000 0001 request return data, following byte is data type
// 0000 0010 clear error (reset)
// 0000 0011 abort (set aborted error)
// 0000 0100 unlock motors
// 0000 0101 lock motors
// 0010 SSSS fan speed S: 0-15
// 0000 1110 updateFlashCode, // set flash bytes as no-app and reboot
// 0000 1111 <reserved for multiple byte immediate commands>

// ===== buffered commands ===== 

// (only first settings command is persistent)
// 110A DUUU settings: A: axis, U: ustep (chip fmt), D: FORWARD or BACKWARD 

// 1000 0000 delay: 2 byte time pair follows
// 10CC CCCC pulse stream: C timed pulses (2 bytes each) follow, C > 0
// 1110 CCCC repeat pulse, 2 byte usec pair that follows, C times
// 1111 110L repeat pulse, 2 byte usec pair that follows, until limit sw == L

// 0001 CCCC set motor current, C: 0-15
// 1111 10SS set status bits for CPU (e.g. homing done)
// 0000 1100 clear distance counters
// 0000 1101 save distance counters, set flag

// ===== available command codes =====

// 0000 011x
// 0000 1000 -> 0000 1011
// 0011 xxxx
// 01xx xxxx


/////////////// SPI BYTES FROM MCU TO CPU
// bytes can only be returned to CPU in first byte after SS starts

// 00EE EEEE  MCU frozen with error, E: error code, 0x00 not allowed
// 01XX XYYY  x and y buffer-in space, X,Y: 2^(x-1), 2^(y-1) bytes available, 0 -> none, 1, 2, ... , 64
// 1HII DDDD  Data Nibble, H: 0:lsn 1:msn, I:idx, 0 to 3 for 4 bytes D: data nibble, 
 
// Data return bytes
//   0: data code (TBD), 1..3: data
   

           

#endif
