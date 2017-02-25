
#include <xc.h>
#include "spi.h"
#include "mcu-cpu.h"
#include "pins-b.h"
#include "vector.h"
#include "motor.h"

volatile char   spiByteFromCpu;  // set by spi interrupt, used in event loop
//char            spiByteToCpu;    // set by event loop, used in spi interrupt
unsigned long   spiWordIn;       // four of spiByteFromCpu
//ReturnStatus    spiReturnStatus; // four of spiByteToCpu
char            spiWordByteIdx;  // byte idx in both spiWordIn and spiReturnStatus

char nextRetWordType = 0; // specifies type of word to be returned next

void initSpi() {
  SSP1SSPPS  = SPI_SS_PPS;       // A7 => select in
  SSP1CLKPPS = SPI_CLK_PPS;      // C3 => clock in
  SSP1DATPPS = SPI_DATA_IN_PPS;  // C4 => data in
  SPI_DATA_OUT_PPS = 0x16;       // Data out => C5
   
  SPI_SS_TRIS       = 1;         // SPI TRIS select input
  SPI_CLK_TRIS      = 1;         // SPI TRIS clock input
  SPI_DATA_IN_TRIS  = 1;         // SPI TRIS data input
  SPI_DATA_OUT_TRIS = 0;         // SPI TRIS data output
  
  SSP1CON1bits.SSPM  = 4; // mode: spi slave with select enabled
  SSP1STATbits.SMP   = 0; // input sample edge (must be zero for slave mode)
  SSP1CON1bits.CKP   = 0; // 0: xmit clk low is idle
  SSP1STATbits.CKE   = 1; // clk edge in (1: active ->idle) (1: safe wcol?)
  SSP1CON3bits.BOEN  = 1; // enable buffer input overflow check (SSPOV))
  spiWordByteIdx = 0;
  /* From datasheet: Before enabling the module in SPI Slave mode, the clock
   line must match the proper Idle state (CKP) */
  while(SPI_CLK);
  
  SSP1CON1bits.SSPEN = 1; // enable SPI
  // start on word boundary
  while(!SPI_SS);
  
  SSP1CON1bits.SSPOV = 0; // clear errors
  SSP1CON1bits.WCOL  = 0;
  SSP1IF = 0;             // start int flag cleared
  PIE3bits.SSP1IE = 1;    // enable ints
}
