// EdgeInterrupt.c
// Runs on LM4F120 or TM4C123
// Request an interrupt on both edges edge of PE3  Note
// Button bouncing must be addressed in hardware.
// Jonathan Valvano
// Jan 31, 2022

/* This example accompanies the book
   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers"
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2021
   Volume 1, Section 9.5
   
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2021
   Volume 2, Section 5.5

 Copyright 2022 by Jonathan W. Valvano, valvano@mail.utexas.edu
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



#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/CortexM.h"

uint32_t RiseCount,FallCount; 
void Switch_Init(uint32_t priority){
  SYSCTL_RCGCGPIO_R |= 0x00000010; // activate clock for port E
  while((SYSCTL_PRGPIO_R & 0x00000010) == 0){};

  GPIO_PORTE_DIR_R   &= ~0x08;     //  make PE3 in (positive logic)
  GPIO_PORTE_DEN_R   |=  0x08;     //  enable digital I/O on PE3   
  GPIO_PORTE_IS_R    &= ~0x08;     //  PE3 is edge-sensitive
  GPIO_PORTE_IBE_R   |=  0x08;     //  PE3 is both edges
  GPIO_PORTE_IM_R |= 0x08;         // arm interrupt on PE3  
  NVIC_PRI1_R = (NVIC_PRI1_R
               & 0xFFFFFF00)
               | (priority<<5);   // Set Priority Level
  NVIC_EN0_R = 1<<4;              // enable interrupt 4 in NVIC 
  RiseCount   = 0;      	 // user function 
  FallCount = 0;      // user function 
}
void GPIOPortE_Handler(void){
  GPIO_PORTE_ICR_R = 0x08;    // clear flag3  
  if(GPIO_PORTE_DATA_R&0x08){ // now touched
    RiseCount++;              // execute user task 
  }
  else{                       // now released
    FallCount++;              // execute user task
  }
}


//debug code
int main(void){ 
  Switch_Init(5);     // initialize GPIO Port F interrupt
  EnableInterrupts();           // interrupt after all initialization
  while(1){

  }
}
