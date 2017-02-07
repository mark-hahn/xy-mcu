
#include <xc.h>
#include "event.h"
#include "pins-b.h"
#include "main.h"
#include "spi.h"
#include "vectors.h"

// step pulse is low, now waiting for timer interrupt to set it high
bool_t isPulsingX = FALSE;
bool_t isPulsingY = FALSE;

// called once from main.c and never returns
// events are dealt with in order of most urgent first
// SPI is highest priority in event loop and lowest in interrupt routine
void eventLoop() {
  while(1) {  
    // a 32-bit word has just arrived
    if(spiWordInByteIdx == 4) {
      spiWordInByteIdx = 0;
      handleNewSpiWord();
      continue;     
    }
    // SS is idle (high) between words, sync word ptr
    if(SPI_SS && spiWordInByteIdx > 0 && spiWordInByteIdx < 4) {
      spiWordInByteIdx == 0; 
      continue;
    }
    // X step pin was raised by interrupt
    if(isPulsingX && CCP1_PIN) { 
      if(status == statusHoming) chkHomingX();
      else chkMovingX();
      continue;
    }
    if(isPulsingY && CCP2_PIN) {
      if(status == statusHoming) chkHomingY();
      else chkMovingY();
      return;
    }
    
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  }
}  

