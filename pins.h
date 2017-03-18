
#ifndef PINS_H
#define	PINS_H

#ifdef XY
// MCU pin assignments for XY

// TRIS, tri-state selects
#define SPI_SS_TRIS       TRISA7 // SPI Slave Select input, SS
#define SPI_CLK_TRIS      TRISC3 // SPI Clock input, SCLK
#define SPI_DATA_IN_TRIS  TRISC4 // SPI Data input,  MOSI
#define SPI_DATA_OUT_TRIS TRISC5 // SPI Data output, MISO

#define RESET_X_TRIS      TRISB3
#define RESET_Y_TRIS      TRISA4
#define STEP_X_TRIS       TRISB4 // CCP 1 pin output, X step
#define STEP_Y_TRIS       TRISC2 // CCP 2 pin output, Y step
#define PWM_TRIS          TRISC1 // pwm (fan) pin output
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
#define PWM_LAT           LATC1  // pwm (fan) pin latch


// SPI: PPS, peripheral select
#define SPI_SS_PPS        0x07    // SSP1SSPPS  <= A7, SPI Slave Select input
#define SPI_CLK_PPS       0x13    // SSP1CLKPPS <= C3, SPI Clock input
#define SPI_DATA_IN_PPS   0x14    // SSP1DATPPS <= C4, SPI Data input
#define SPI_DATA_OUT_PPS  RC5PPS  // Data out   => C5, SPI Data output
#define SPI_FAN_OUT_PPS   RC1PPS  // pwm3 out   => C1, fan output

#define SPI_SS_IOC        IOCAPbits.IOCAP7  // interrupt on rising A7 pin
#define SPI_SS_IOC_IF     IOCAFbits.IOCAF7  // interrupt flag

#define X_FAULT_IOC       IOCBNbits.IOCBN3  // interrupt on falling B3 pin
#define X_FAULT_IOC_IF    IOCBFbits.IOCBF3  // interrupt flag

#define Y_FAULT_IOC       IOCCNbits.IOCCN0  // interrupt on falling C0 pin
#define Y_FAULT_IOC_IF    IOCCFbits.IOCCF0  // interrupt flag
#endif	

#ifdef Z2

// TRIS, tri-state selects
#define SPI_SS_TRIS       TRISC2 // SPI Slave Select input, SS
#define SPI_CLK_TRIS      TRISB4 // SPI Clock input, SCLK
#define SPI_DATA_IN_TRIS  TRISB6 // SPI Data input,  MOSI
#define SPI_DATA_OUT_TRIS TRISB5 // SPI Data output, MISO

#define RESET_X_TRIS      TRISC5
#define STEP_X_TRIS       TRISA4 // CCP 1 pin output, X step
#define PWM_VREF_TRIS     TRISA2 // pwm (vref) pin output
#define LIMIT_SW_X_TRIS   TRISC7

// pin input values unlatched
#define SPI_SS            RC2    // SPI Slave Select level, no latch
#define SPI_CLK           RB4    
#define LIMIT_SW_X        RC7

// LAT, pin latches
#define MS1_X_LAT        LATC1
#define MS2_X_LAT        LATC3
#define MS3_X_LAT        LATC4
#define RESET_X_LAT      LATC5
#define STEP_X_LAT       LATA4
#define DIR_X_LAT        LATA5
#define PWM_VREF_LAT     LATA2  // pwm (vref) pin latch

// SPI: PPS, peripheral select
#define SPI_SS_PPS        0x11    // SSP1SSPPS  <= A7, SPI Slave Select input
#define SPI_CLK_PPS       0x0c    // SSP1CLKPPS <= C3, SPI Clock input
#define SPI_DATA_IN_PPS   0x0e    // SSP1DATPPS <= C4, SPI Data input
#define SPI_DATA_OUT_PPS  RB5PPS  // Data out   => C5, SPI Data output
#define SPI_PWM_VREF_PPS  RA2PPS  // pwm3 out   => A2, pwm vref output

#define SPI_SS_IOC        IOCAPbits.IOCAP7  // interrupt on rising A7 pin
#define SPI_SS_IOC_IF     IOCAFbits.IOCAF7  // interrupt flag

#define X_FAULT_IOC       IOCCPbits.IOCCP2  // interrupt on falling B3 pin
#define X_FAULT_IOC_IF    IOCCFbits.IOCCF2  // interrupt flag
#endif

#endif
