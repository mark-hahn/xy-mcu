
#include <xc.h>
#include "spi.h"
#include "pins-b.h"

char volatile spiWordIn[4]; // filled by int routine, removed in event loop
char volatile spiWordInByteIdx;
char spiByteOut; // set by event loop each time SPI byte exchanged

// this queue maintained by chkSpi()
// status out buf by bytes ...
  // 1 header marker, 0xff, syncs bytes, 0xfe for flow control
  // 1 status, axis, error code, and state
  // 3 home distance X
  // 3 home distance Y

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
  SSP1CON1bits.CKP   = 0; // clk low is idle (should match CPU)
  SSP1STATbits.CKE   = 1; // clk non-idle -> idle bit capture (safer wcol))
  SSP1CON3bits.BOEN  = 0; // enable buffer overflow check (SSPOV))
  spiWordInByteIdx = 0;

  /* Before enabling the module in SPI Slave mode, the clock
   line must match the proper Idle state (CKP) */
  while(SPI_CLK);
  SSP1CON1bits.SSPEN = 1; // enable SPI
}
