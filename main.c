
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
#pragma config WDTE = SWDTEN     // WDT operating mode (WDT enabled/disabled by SWDTEN bit in WDTCON0)
#pragma config WDTCWS = WDTCWS_7// WDT Window Select bits (window always open (100%); software control; keyed access not required)
#pragma config WDTCCS = SC      // WDT input clock selector (Software Control)
#pragma config BBSIZE = BB512   //  (512 char boot block size) 
#pragma config BBEN = ON        //  (Boot Block enabled)
#pragma config SAFEN = OFF      //  (SAF disabled)
#pragma config WRTAPP = OFF     //  (Application Block not write protected)
#pragma config WRTB = OFF       //  (Boot Block not write protected)
#pragma config WRTC = OFF       //  (Configuration Register not write protected)
#pragma config WRTSAF = OFF     //  (SAF not write protected)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (High Voltage on MCLR/Vpp must be used for programming)
#pragma config CP = OFF         // UserNVM Program memory code protection bit (UserNVM code protection disabled)

#include <xc.h>
#include "main.h"
#include "mcu-cpu.h"
#include "timer.h"
#include "pins.h"
#include "vector.h"
#include "spi.h"
#include "motor.h"
#include "event.h"
#include "invtable.h"

#ifdef XY
#include "fan.h"
#endif
#ifdef Z2
#include "pwm-vref.h"
#endif

#define INT_ERROR_FAULT_X      (0x00 | errorFault);
#define INT_ERROR_FAULT_Y      (0x80 | errorFault);
#define INT_ERROR_SPURIOUS_INT (0x80 | errorSpuriousInt);

// interrupt error checked by eventloop, d7 is axis
volatile char   intError = 0;  

volatile bool_t spiInt  = FALSE;
volatile bool_t CCP1Int = FALSE;
#ifdef XY
volatile bool_t CCP2Int = FALSE;
#endif

// global interrupt routine
void interrupt isr(void) {
  if(SSP1IF) {
  // spi byte arrived
    SSP1IF = 0;
    if(++spiBytesInIdx > 4) intError = errorSpiByteSync;
    else spiBytesIn[4-spiBytesInIdx] = SSP1BUF;
  }
  if(SPI_SS_IOC_IF) {
  // spi word arrived (SS went high)
    SPI_SS_IOC_IF = 0;
    if(spiInt) intError = errorSpiBytesOverrun;
    else {
      spiBytesInIdx = 0;   
      spiInt = TRUE; // flag eventloop
    }
  }
  if(CCP1IE && CCP1IF) { 
    // X timer compare int
    CCP1IF     = 0;
    STEP_X_LAT = 1; // driver pulse active edge
    CCPR1H   = timeX.timeBytes[1];  // set next compare time
    CCPR1L   = timeX.timeBytes[0];
    CCP1Int  = TRUE; // flag eventloop
    return;
  }
  if(X_FAULT_IOC_IF) {             
    X_FAULT_IOC_IF = 0;
    intError = INT_ERROR_FAULT_X;
  }

#ifdef XY
  if(CCP2IE && CCP2IF) { 
    // Y timer compare int
    CCP2IF     = 0;
    STEP_Y_LAT = 1;
    CCPR2H   = timeY.timeBytes[1];
    CCPR2L   = timeY.timeBytes[0];
    CCP2Int  = TRUE;
    return;
  }
  if(Y_FAULT_IOC_IF) {        
    Y_FAULT_IOC_IF = 0;
    intError = INT_ERROR_FAULT_Y;
  }
#endif
}

void main(void) {
  SWDTEN = 1;  // start watchdog, must bark before 2 seconds

  statusRec.rec.len       = STATUS_SPI_BYTE_COUNT;
  statusRec.rec.type      = STATUS_REC;
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
#ifdef XY
  TRISA = 0b10100000; // all out except ss and on
  TRISB = 0b11000000; // all out except ICSP pins
  TRISC = 0b11011001; // all in except miso, vstep, and fan
#endif
#ifdef Z2
  TRISA = 0b00001011; // all out except ICSP & MCLR
  TRISB = 0b01110000; // sclk & mosi in (miso in until later)
  TRISC = 0b11000100; // all out except ss, fault, and lim
#endif
  
  initDbg(); /// dbgPulseH(3);

  initInvtable();
  initVectors();
  initMotor();
  initEvent();
  initSpi();
  initTimer();
#ifdef XY
  initFan();
#endif
  
  handleError(0, errorReset);

   // global ints on
  IOCIE =  1; // Interrupt on pin change (ss and faults)
  PEIE  =  1; // Peripheral Interrupt Enable
  GIE   =  1; // Global Interrupt Enable

  eventLoop(); // doesn't return
}
