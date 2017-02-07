
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
#include "vectors.h"
#include "spi.h"
#include "event.h"

// global interrupt routine
// absolute minimum code here
// reloading timer compare values is most urgent, so first
// SPI is highest priority in event loop and lowest in interrupt routine
void interrupt isr(void) {
  if(CCP1IF) { // X timer compare int
    // next timer 1 value to match for X pulse
    // values must be set in busy loop before next int
    CCPR1L = ccpXLowByte;
    CCPR1H = ccpXHighByte;
    // if new compare value is alreay passed by now then delay will be 65536 clks
    // int flag clr should be last to make sure nothing above retriggered flag
    CCP1IF = 0;
  }
  if(CCP2IF) { // Y timer compare int (same comments above apply)
    CCPR2L = ccpYLowByte;
    CCPR2H = ccpYHighByte;
    CCP2IF = 0;
  }
  // SPI exchanged, get data in and set data out
  if(SSP1IF) {
    SSP1IF = 0;
    spiWordIn[spiWordInByteIdx++] = SSP1BUF;
    SSP1BUF = spiByteOut;
  }
}  

void main(void) {
  ANSELC = 0; // no analog inputs
  
  initDac(); 
  initVectors();
  initTimer();
  initSpi();
  
  // global ints on
  PEIE  =  1; // Peripheral Interrupt Enable
  GIE   =  1; // Global Interrupt Enable

  eventLoop(); // doesn't return
}
