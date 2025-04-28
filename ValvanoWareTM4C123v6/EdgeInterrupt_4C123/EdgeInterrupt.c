// EdgeInterrupt.c
// Runs on LM4F120 or TM4C123
// Request an interrupt on the falling edge of PF4 (when the user
// button is pressed) and increment a counter in the interrupt.  Note
// that button bouncing is not addressed.
// Daniel Valvano
// May 3, 2015

/* This example accompanies the book
   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers"
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2015
   Volume 1, Program 9.4
   
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2014
   Volume 2, Program 5.6, Section 5.5

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// user button connected to PF4 (increment counter on falling edge)

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/EdgeInterruptPortF.h"
#include "../inc/CortexM.h"
#include "../inc/PLL.h"
#include "../inc/TExaS.h"


// global variable visible in Watch window of debugger
// increments at least once per button press
extern volatile uint32_t FallingEdges;
//********************************************************************************
// debuging profile, pick up to 7 unused bits and send to Logic Analyzer
// TExaSdisplay logic analyzer shows 5 bits 0,0,PF4,PF3,PF2,PF1,PF0 
// edit this to output which pins you use for profiling
// you can output up to 7 pins
// use for debugging profile
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))



//debug code
int main(void){
// pick one of the following three lines, all three set PLL to 80 MHz
  PLL_Init(Bus80MHz);        // 1) call to have no TExaS debugging
  //TExaS_Init(LOGICANALYZERF);  // 2) call to activate logic analyzer
  //TExaS_Init(SCOPE);         // or 3) call to activate analog scope PD3
  
  EdgeCounterPortF_Init();           // initialize GPIO Port F interrupt
  EnableInterrupts();
  while(1){
    PF3 ^= 0x08;
  }
}
