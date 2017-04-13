
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
};

// ustep (microstep) matches chip MS1, MS2, and MS3 everywhere in MCU

///////////////  SPI BYTES FROM CPU TO MCU  ///////////////

// need to handle limit sw already up or down  -- TODO


// ===== immediate commands =====
// 4-bits in bottom nibble, top is zero
enum immCmds { 
  noopCmd,  // no-op, just for getting return byte
  clearCmd, // clear state: error, bufs, status count, etc. (no motor reset)
  abortCmd, // abort (set aborted error)
  unlockCmd,// unlock motors
  lockCmd,  // lock motors
  flashCmd  // updateFlashCode, // set flash bytes as no-app and reboot
}

// 0010 SSSS fan speed S: 0-15
// 0011 DDDD Request data, D: data type

// ===== buffered commands ===== 

// (only first settings command is persistent)
// 110A DUUU settings: A: axis, U: ustep (chip fmt), D: FORWARD or BACKWARD 

// 1000 0000 delay: 2 byte time pair follows
// 10CC CCCC pulse stream: C timed pulses (2 bytes each) follow, C > 0
// 1110 CCCC repeat pulse, 2 byte usec pair that follows, C times
// 1111 110L repeat pulse, 2 byte usec pair that follows, until limit sw == L

// 0001 CCCC set motor current, C: 0-15
// 0000 1110 clear distance counters
// 0000 1111 save distance counters, set flag
// 1111 1000 inc status count for CPU (e.g. homing done), see status byte
// 1111 1000 dec status count for CPU (e.g. homing done), see status byte

// ===== available command codes =====

// 0000 0110 to 0000 .... immediate
// 0000 .... to 0000 1101  buffered
// 01xx xxxx


/////////////// SPI BYTES FROM MCU TO CPU
// bytes can only be returned to CPU in first byte after SS starts

// first is default when no error and no data requested
// 10FX XGYY  status count > 0 (F:X, G:Y), buf space, XX,YY: 2^(2*XX), 1, 4, 16, 64
// 110E EEEE  MCU frozen with error, E: error code
// 0CCC CCCC  Data: Header C: num bytes, trailer C: data type from req

#endif
