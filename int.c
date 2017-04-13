
#include <xc.h>
#include "pins.h"
#include "mcu-api.h"
#include "process.h"
#include "int.h"
#include "util.h"
#include "int-cmd.h"

#ifdef XY
uint8_t spi_axis = X;
#endif

// global interrupt routine
void interrupt isr(void) {

  //////////  SPI INTERRUPT  //////////
  if(SSP1IF) {
    SSP1IF = 0;
    uint8_t spiByte = SSP1BUF;

    if(SSP1CON1bits.WCOL) {
      SSP1CON1bits.WCOL = 0;
      handleError(0, errorSpiWcol);
      return;
    }
    if(SSP1CON1bits.SSPOV) {
      SSP1CON1bits.SSPOV = 0;
      handleError(0, errorSpiOvflw);
      return;
    }
#ifdef XY
    if((spiByte & 0xe0) == 0xc0)  // settings (including axis) cmd
      spi_axis = (spiByte & 0x10) >> 4;
  
    // immediate commands (non-axis) can go into either buf
    // so they might be processed out of order
    if(spi_axis == X) 
#endif
    {
      uint8_t oldWrPtrX = spiBufWrX;
      uint8_t newWrPtrX = ((spiBufWrX + 1) & 0x7f);
      if(newWrPtrX == spiBufRdX)
        handleError(0, bufOvflwX);
      else {
        spiBufX[oldWrPtrX] = spiByte;          
        spiBufWrX = newWrPtrX;
      }
    }
#ifdef XY
    else {
      uint8_t oldWrPtrY = spiBufWrY;
      uint8_t newWrPtrY = ((spiBufWrY + 1) & 0x7f);
      if(newWrPtrY == spiBufRdY)
        handleError(0, bufOvflwY);
      else {
        spiBufY[oldWrPtrY] = spiByte;          
        spiBufWrY = newWrPtrY;
      }
    }
#endif
  }
  
  //////////  SLAVE-SELECT PACKET IOC INTERRUPT  //////////
  if(SPI_SS_IOC_IF) {
    SPI_SS_IOC_IF = 0;
    if(retBufWr != retBufRd) {
      SSP1BUF = retBuf[retBufRd];
      retBufRd = ((retBufRd + 1) & 0x0f);
    }
    else
      SSP1BUF = bufSpaceByte();
  }

  //////////  X TIMER COMPARE CCP1 INTERRUPT  //////////
  if(CCP1IF) { 
    STEP_X_LAT = 1; // driver pulse high (only does anything when set low before)
    processCmd(X);
    CCP1IF = 0;
  }

  //////////  X DRIVER FAULT PIN IOC INTERRUPT  //////////
  if(X_FAULT_IOC_IF) {             
    X_FAULT_IOC_IF = 0;
    handleError(0, errorFaultX);
    return;
  }

#ifdef XY
  //////////  Y TIMER COMPARE CCP2 INTERRUPT  //////////
  if(CCP2IF) { 
    STEP_Y_LAT = 1; // driver pulse high (only does anything when set low before)
    processCmd(Y);
    CCP2IF = 0;
  }

  //////////  Y DRIVER FAULT PIN IOC INTERRUPT  //////////
  if(Y_FAULT_IOC_IF) {             
    Y_FAULT_IOC_IF = 0;
    handleError(0, errorFaultY);
    return;
  }
#endif
}

