
#ifndef PINS_B_H
#define	PINS_B_H

// MCU pin assignments for XY rev B board

// TRIS, tri-state selects
#define SPI_SS_TRIS       TRISA7 // SPI Slave Select input, SS
#define SPI_CLK_TRIS      TRISC3 // SPI Clock input, SCLK
#define SPI_DATA_IN_TRIS  TRISC4 // SPI Data input,  MOSI
#define SPI_DATA_OUT_TRIS TRISC5 // SPI Data output, MISO
#define STEP_X_TRIS       TRISB4 // CCP 1 pin output, X step
#define STEP_Y_TRIS       TRISC2 // CCP 2 pin output, Y step
#define FAN_TRIS          TRISC1 // fan pin output
#define LIMIT_SW_X_TRIS   TRISC6
#define LIMIT_SW_Y_TRIS   TRISC7

// pin input values unlatched
#define SPI_SS            RA7    // SPI Slave Select level, no latch
#define SPI_CLK           RC3    
#define LIMIT_SW_X        RC6
#define LIMIT_SW_Y        RC7

// LAT, pin latches
#define MS1_X_LAT         LATB0
#define MS2_X_LAT         LATB1
#define MS3_X_LAT         LATB2
#define RESET_X_LAT       LATB3
#define STEP_X_LAT        LATB4
#define DIR_X_LAT         LATB5

#define MS1_Y_LAT         LATA0
#define MS2_Y_LAT         LATA1
#define MS3_Y_LAT         LATA3
#define RESET_Y_LAT       LATA4
#define STEP_Y_LAT        LATC2
#define DIR_Y_LAT         LATA6
 
#define FAN_LAT           LATC1  // fan pin latch


// SPI: PPS, peripheral select
#define SPI_SS_PPS        0x07    // SSP1SSPPS  <= A7, SPI Slave Select input
#define SPI_CLK_PPS       0x13    // SSP1CLKPPS <= C3, SPI Clock input
#define SPI_DATA_IN_PPS   0x14    // SSP1DATPPS <= C4, SPI Data input
#define SPI_DATA_OUT_PPS  RC5PPS  // Data out   => C5, SPI Data output


#endif	/* PINS_B_H */

 