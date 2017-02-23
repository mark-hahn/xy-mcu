
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

// ignore first spi Int Error after boot
bool_t spiInt  = FALSE;
bool_t CCP1Int = FALSE;
bool_t CCP2Int = FALSE;

// global interrupt routine
// reloading timer compare values is most urgent, so first
// SPI is highest priority in event loop and lowest here
void interrupt isr(void) {
  if(SSP1IF) {
    spiByteFromCpu = SSP1BUF;
    SSP1IF = 0;
    SSP1CON1bits.SSPOV = 0; // clear errors
    SSP1CON1bits.WCOL  = 0;
    ((char *) &spiWordIn)[3-spiWordByteIdx++] = spiByteFromCpu;
    if(spiWordByteIdx == 4) spiInt = TRUE;
  }
  if(CCP1IF) { // X timer compare int
    // CCP1 match just happened
    CCP1_LAT = 1;
    CCPR1H   = timeX.timeBytes[1];
    CCPR1L   = timeX.timeBytes[0];
    CCP1Int  = TRUE;
    CCP1IF   = 0;
  }
  if(CCP2IF) { // Y timer compare int (same comments above apply)
    CCP2_LAT = 1;
    CCPR2H   = timeY.timeBytes[1];
    CCPR2L   = timeY.timeBytes[0];
    CCP2Int  = TRUE;
    CCP2IF   = 0;
  }
}  

void main(void) {
  ANSELA = 0; // no analog inputs
  ANSELB = 0;
  ANSELC = 0;

  initDac(); 
  initVectors();
  initMotor();
  initEvent();
  initSpi();
  
  LATC6 = 1;
  TRISC6 = 0; // event loop debug trace -- hlim
  LATC6 = 0;
  LATC6 = 1;
  
  LATC7 = 1;
  TRISC7 = 0; // interrupt debug trace -- vlim

  initTimer();

   // global ints on
  PEIE  =  1; // Peripheral Interrupt Enable
  GIE   =  1; // Global Interrupt Enable

  eventLoop(); // doesn't return
}
