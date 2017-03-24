
#include <xc.h>
#include "spi.h"
#include "mcu-cpu.h"
#include "pins.h"
#include "vector.h"
#include "motor.h"

uint32_t spiWord;
uint16_t spiInts[2];
char    *spiBytes;

volatile char spiBytesIn[4];  // a word (four chars) from SPI, big-endian
volatile char spiBytesInIdx;  // index for spiBytesIn

void initSpi() {
  SPI_SS_TRIS       = 1;         // SPI TRIS select input
  SPI_CLK_TRIS      = 1;         // SPI TRIS clock input
  SPI_DATA_IN_TRIS  = 1;         // SPI TRIS data input
  SPI_DATA_OUT_TRIS = 1;         // SPI TRIS data (output later) 
  
  SSP1CON1bits.SSPM  = 4; // mode: spi slave with slave select enabled
  SSP1STATbits.SMP   = 0; // input sample edge (must be zero for slave mode)
  SSP1CON1bits.CKP   = 0; // 0: xmit clk low is idle
  SSP1STATbits.CKE   = 1; // clk edge in (1: falling edge if above is 0)
  SSP1CON3bits.BOEN  = 1; // enable buffer input overflow check (SSPOV))
  spiBytesInIdx = 0;

  SSP1SSPPS  = SPI_SS_PPS;       // slave select in
  SSP1CLKPPS = SPI_CLK_PPS;      // clock in
  SSP1DATPPS = SPI_DATA_IN_PPS;  // data in
  SPI_DATA_OUT_PPS = 0x16;       // Data out (0x16: spi miso)
   
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

  SPI_DATA_OUT_TRIS = 0; // wait until now to not short out other mcu's miso
  
  // interrupt on SS rising
  SPI_SS_IOC = 1;
  SPI_SS_IOC_IF = 0;  // IOC int flag for specific A7 pin
  spiInt = 0;
  
}
