
// PIC16LF15355 Configuration Bit Settings

#pragma config FEXTOSC = OFF    // External Oscillator mode selection bits (Oscillator not enabled)
#pragma config RSTOSC = HFINT32 // Power-up default value for COSC bits (HFINTOSC with OSCFRQ= 32 MHz and CDIV = 1:1)
#pragma config CLKOUTEN = OFF   // Clock Out Enable bit (CLKOUT function is disabled; i/o or oscillator function on OSC2)
#pragma config CSWEN = ON       // Clock Switch Enable bit (Writing to NOSC and NDIV is allowed)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable bit (FSCM timer enabled)
#pragma config MCLRE = ON       // Master Clear Enable bit (MCLR pin is Master Clear function)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config LPBOREN = OFF    // Low-Power BOR enable bit (ULPBOR disabled)
#pragma config BOREN = OFF       // Brown-out reset enable bits (Brown-out Reset Enabled, SBOREN bit is ignored)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (VBOR) set to 1.9V on LF, and 2.45V on F Devices)
#pragma config ZCD = OFF        // Zero-cross detect disable (Zero-cross detect circuit is disabled at POR.)
#pragma config PPS1WAY = OFF     // Peripheral Pin Select one-way control (The PPSLOCK bit can be cleared and set only once in software)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable bit (Stack Overflow or Underflow will cause a reset)
#pragma config WDTCPS = WDTCPS_31// WDT Period Select bits (Divider ratio 1:0x10000; software control of WDTPS)
#pragma config WDTE = OFF       // WDT operating mode (WDT enabled regardless of sleep; SWDTEN ignored)
#pragma config WDTCWS = WDTCWS_7// WDT Window Select bits (window always open (100%); software control; keyed access not required)
#pragma config WDTCCS = SC      // WDT input clock selector (Software Control)
#pragma config BBSIZE = BB512   //  (512 char boot block size)
#pragma config BBEN = OFF       //  (Boot Block disabled)
#pragma config SAFEN = OFF      //  (SAF disabled)
#pragma config WRTAPP = OFF     //  (Application Block not write protected)
#pragma config WRTB = OFF       //  (Boot Block not write protected)
#pragma config WRTC = OFF       //  (Configuration Register not write protected)
#pragma config WRTSAF = OFF     //  (SAF not write protected)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (High Voltage on MCLR/Vpp must be used for programming)
#pragma config CP = OFF         // UserNVM Program memory code protection bit (UserNVM code protection disabled)

#include <xc.h>
#include "main.h"
#include "timer.h"
#include "pins-b.h"
#include "vector.h"
#include "spi.h"
#include "motor.h"
#include "event.h"
#include "dac.h"

volatile bool_t spiInt  = FALSE;
volatile bool_t CCP1Int = FALSE;
volatile bool_t CCP2Int = FALSE;
volatile char   intError = 0;

// global interrupt routine
void interrupt isr(void) {
  if(SSP1IF) {
  // spi byte arrived
    SSP1IF = 0;
    spiBytesIn[3-spiBytesInIdx++] = SSP1BUF;
   // does not flag eventloop, IOC does below after all four bytes arrived
  }
  if(SPI_SS_IOC_IF) {
  // spi word arrived (SS went high)
    SPI_SS_IOC_IF = 0;
    if(spiInt)                  intError = errorspiBytesOverrun;
    else if(spiBytesInIdx != 4) intError = errorSpiByteSync;
    else spiInt = TRUE; // flag eventloop
  }
  if(CCP1IE && CCP1IF) { 
    // X timer compare int
    CCP1IF     = 0;
    STEP_X_LAT = 1; // driver pulse active edge
    CCPR1H   = timeX.timeBytes[1];  // set next compare time
    CCPR1L   = timeX.timeBytes[0];
    CCP1Int  = TRUE; // flag eventloop
  }
  if(CCP2IE && CCP2IF) { 
    // Y timer compare int
    CCP2IF     = 0;
    STEP_Y_LAT = 1;
    CCPR2H   = timeY.timeBytes[1];
    CCPR2L   = timeY.timeBytes[0];
    CCP2Int  = TRUE;
  }
}  

void main(void) {
  statusRec.rec.apiVers   = API_VERSION;
  statusRec.rec.mfr       = MFR;  
  statusRec.rec.prod      = PROD; 
  statusRec.rec.vers      = VERS; 
  statusRec.rec.homeDistX = 0;
  statusRec.rec.homeDistY = 0;

  ANSELA = 0; // no analog inputs
  ANSELB = 0; // these &^%$&^ regs cause a lot of trouble
  ANSELC = 0; // they should not default to on and override everything else

  // change these to defined constants   TODO
  // and have each init do their own
  TRISA = 0b10100000; // all out except ss and on
  TRISB = 0b11000000; // all out except ICSP pins
  TRISC = 0b11011001; // all in except miso, vstep, and fan

  initMotor();
  initVectors();
  initEvent();
  initSpi();
  initTimer();
  
  setState(statusSleeping);

   // global ints on
  PEIE  =  1; // Peripheral Interrupt Enable
  GIE   =  1; // Global Interrupt Enable

  eventLoop(); // doesn't return
}
