# Lab 1 Preparation
*Written by: Jackson Paull 1/18/25*

## 2 (UARTInts_4C123)
a) This example used UART0. What lines of C code define which port will be used for the UART channel?
> A: Lines 89-91 in UART0int.c:
>> ```
>>  GPIO_PORTA_AFSEL_R |= 0x03;          // enable alt funct on PA1-0
>>  GPIO_PORTA_DEN_R |= 0x03;             // enable digital I/O on PA1-0
>>                                        // configure PA1-0 as UART
>>  GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R&0xFFFFFF00)+0x00000011;
>>  GPIO_PORTA_AMSEL_R = 0;               // disable analog functionality on PA
>> ```
> 

b) What lines of C code define the baud rate, parity, data bits and stop bits for the UART?
> A: Lines 78-81 define the baud rate, and set the word length. Notably, the parity enable bit (bit 1) and the stp2 (bit 3) bits are  both left unset meaning that there will be 1 start bit, 1 stop bit, 8 data bits, and no parity bit.
> 
> Here the baud rate is 115,200 bps = 14.0625kBps
>> ```
>>  UART0_IBRD_R = 43;                    // IBRD = int(80,000,000 / (16 * 115,200)) = int>>(43.403)
>>  UART0_FBRD_R = 26;                    // FBRD = round(0.4028 * 64 ) = 26
>>                                        // 8 bit word length (no parity bits, one stop bit, FIFOs)
>>  UART0_LCRH_R = (UART_LCRH_WLEN_8|UART_LCRH_FEN);

c) Which port pins do we use for the UART? Which pin transmits and which pin receives?
> A: We are using UART0 which is specific to pins A0 and A1. From table 10-2 in the datasheet, A0 is Rx, while A1 is Tx

d) Look in the uart.c driver to find what low-level C code inputs one byte from the UART port
> A: ```copyHardwareToSoftware()``` moves a byte from UART0_DR_R into the Rx Fifo, therefore reading the byte from UART (though not necessarily ingesting / processing it yet through software)

e) Similarly, find the low-level C code that outputs one byte to the UART port.
> A: ```copySoftwareToHardware()``` similarly moves data from the Tx FIFO queue to the data register.

f) Find in the project the interrupt vector table. In particular, how does the system set the ISR vector?
> A: The vector found in the table will be filled by the assembler at compile time. The exported function names for any interrupt handlers must match (hence why it is unadvised to deviate from the standard names), then once an interrupt is triggered at runtime the vector will correctly point to the implementation of the interrupt handler.

g) This code UART0_ICR_R = UART_ICR_TXIC; acknowledges a serial transmit interrupt. Explain how the
acknowledgement occurs in general for all devices and in specific for this device.
> A: When an interrupt is triggered, a flag is set in the ICR register. Writing a 1 to this specific bit acknowledges the interrupt and clears the flag. If a second interrupt of the same kind will be triggered immediately after, it will be postponed until the first is dealt with and acknowledged. On the TM4C123, the TXIC bit is bit 5, hence UART_ICR_TXIC = 0x20, as in the code

h) Look in the data sheet of the TM4C123 and determine the extent of hardware buffering of the UART channel.
For example, simple microcontrollers like the MSP432 only have a transmit data register and a transmit shift
register. So, the software can output two bytes before having to wait. The serial ports on the PC have 16 bytes
of buffering. So, the software can output 16 bytes before having to wait. The MSP432 only has a receive data
register and a receive shift register. This means the software must read the received data within 10 bit times
after the receive flag is set in order to prevent overrun. Is the TM4C123 like the MSP432 (allowing just two
bytes), or is it like the PC (having a larger hardware FIFO buffer)?
> A: According to section 14.3.8, there are two FIFOs (which are each 16x8, one for Tx one for Rx) allowing for 16 bytes to be input / output before stalling.


## 3 (ST7735_4C123)
a) What synchronization method do we use for the low-level command writedata?  
> A: Busy-wait synchronization

b) Explain the parameters of the function ST7735_DrawChar. I.e., how do you use this function?  
> A: x, y are the pixel coordinates on the breakout display, c is the character you want to print, bgColor and textColor are the colors of the text background and the text itself respectively, and size is a scaling factor for the number of pixels to draw on to.

c) Which port pins do we use for the LCD? Find the connection diagram needed to interface the LCD.  
> A: We use SSI 0 on port A using pins 3, 6, and 7 as the output pins, then 2, 3, 5 as SSI0

d) Specify which other device shares pins with the LCD.  
> A: If we use SSI1 or 3, they both share pins on port D so any device using SSI1 while we init with port D would cause an overlap


## 4 (Periodic_SysTickInts_4C123, ST7735_4C123)

a) What C code defines the period of the SysTick interrupt?  
> A: The setting of STRELOAD in conjunction with the clock source (and its respective frequency). In these cases, we use a bus clock of 80 MHz and set STRELOAD based on that.

b) Look at these projects PeriodicSysTickInts_4C123, and ST7735_4C123. How does the software
establish the bus frequency? Find the code that sets the SYSCTL_RCC and SYSCTL_RCC2 registers. Look
these two registers up in the data sheet. Look at these three projects to explain how the system clock is
established. We will be running at 80 MHz for most labs in the class.  
> A: ```PLL_Init()``` in ```PLL.c``` sets the SYSCTL_RCC registers. Using the 16MHz crystal we enable a 400Mhz PLL (which we set to divide by 5 through setting a divisor of 5 in SYSDIV2) yielding 80MHz

c) Look up in the data sheet what condition causes this SysTick interrupt and how we acknowledge this
interrupt? In particular, what sets the COUNT flag in the NVIC_ST_CTRL_R and what clears it?  
> A: The count flag is set when the clock counts to 0, and the flag is cleared whenever a read is performed on the STCTRL register (bit 16 is COUNT). COUNT is also cleared on any write to STCURRENT

## 5
Look up the explicit sequence of events that occur as an interrupt is processed. Read section 2.5 in the TM4C123
data sheet (http://www.ti.com/lit/ds/symlink/tm4c123gh6pm.pdf). Look at the assembly code generated for an
interrupt service routine.

a) What is the first assembly instruction in the ISR? What is the last instruction?  
> A: The first assembly instruction in the ISR handler itself is a MOVW, however there are automatic PUSH's that occur before this. The last is BX LR

b) How does the system save the prior context as it switches threads when an interrupt is triggered?  
> A: The system makes use of the stack, and saves the old pc, psr, isr, and other necessary registers on the stack when performing a context switch.

c) How does the system restore context as it switches back after executing the ISR?  
> A: After pushing a lot of things on to the stack when entering an ISR, the same things are popped off the stack (assuming AAPCS compliant interrupt handlers and therefore stack balancing)