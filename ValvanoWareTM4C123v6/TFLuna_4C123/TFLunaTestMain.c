// TFLunaTestMain.c
// Runs on LM4F120/TM4C123
// Jonathan Valvano
// Jan 28, 2024

// SJ-PM-TF-Luna+A01
// Pin
// 1    Red  5V
// 2    RxD receiving data    U1Tx PC5 is TxD (output of this microcontroller)
// 3    TxD transmitting data U1Rx PC4 is RxD (input to this microcontroller)
// 4    black ground


#include <stdint.h>
#include "../inc/PLL.h"
#include "../inc/CortexM.h"
#include "TFLuna.h"
#include "../inc/tm4c123gh6pm.h"
#define UART 0
#if UART
#include "../inc/UART.h"
#endif
#include "../inc/ST7735.h"
uint8_t ReceiveMessage[32];
uint8_t DataMessage[16];
uint32_t TFLunaOk,TFLunaOk2,TFLunaError;
#if UART
void PrintMessage(uint8_t msg[]){
  uint32_t i=0;
  while(i<msg[1]){
    UART_OutChar('0');
    UART_OutChar('x');
    UART_OutUHex2(msg[i]);
    if((i+1) < msg[1]){    
      UART_OutChar(',');
    }
    i++;
  }
  UART_OutString("\n\r");
}
void PrintData(uint8_t msg[]){
  uint32_t i;
  for(i=0; i<9; i++){
//    UART_OutChar('0');
//    UART_OutChar('x');
    UART_OutUHex2(msg[i]);
    if(i < 8){    
      UART_OutChar(',');
    }
  }
  UART_OutString("\n\r");
}
#endif
#define LFLunaMAX 10000000
int ReceiveData(uint8_t buf[]){
  uint32_t timeout=0;
  uint32_t i=0;
  uint8_t data;
// search for 59
  do{
    while(TFLuna_InStatus()==0){
      timeout++;
      if(timeout>LFLunaMAX){
        TFLunaError++; 
        return 0;
      }
    }
    data = TFLuna_InChar();
  }while(data != 0x59);
  buf[0] = data;
  for(i=1; i<9; i++){
    while(TFLuna_InStatus()==0){
      timeout++;
      if(timeout>LFLunaMAX){ 
        TFLunaError++; 
        return 0;
      }
    }
    buf[i] = TFLuna_InChar();
  }
//  buf[i] = 0;
  return 1;
}
int ReceiveResponse(uint8_t buf[]){
  uint32_t timeout=0;
  uint32_t i=0;
  uint8_t data;
// search for 5A
  do{
    while(TFLuna_InStatus()==0){
      timeout++;
      if(timeout>LFLunaMAX){ 
        TFLunaError++; 
        return 0;
      }
    }
    data = TFLuna_InChar();
  }while(data != 0x5A);
  buf[0] = data;
// receive length
  while(TFLuna_InStatus()==0){
    timeout++;
    if(timeout>LFLunaMAX){ 
      TFLunaError++; 
      return 0;
    }
  }
  buf[1] = TFLuna_InChar();
  for(i=2; i<buf[1]; i++){
    while(TFLuna_InStatus()==0){
      timeout++;
      if(timeout>LFLunaMAX){ 
        TFLunaError++; 
        return 0;
      }
    }
    buf[i] = TFLuna_InChar();
  }
 // buf[i] = 0;
  return 1;
}

uint32_t TFDistance; // mm
uint32_t TFDistance2; // mm
#define TFSIZE 10    // filter size
#if UART
int main1(void){// main1, use for initial testing sensor
  uint32_t sum=0;
  DisableInterrupts();
  PLL_Init(Bus80MHz);       // set system clock to 80 MHz
  TFLunaOk = 0;
  TFLuna_Init(0);         // initialize UART1, debug mode
  UART_Init(); // debugging
  EnableInterrupts();       // Enable interrupts
  UART_OutString("TF Luna Test\n\r");
  TFLuna_Format_Standard_mm();
  if(ReceiveResponse(ReceiveMessage)){
    UART_OutString("Formatmm "); PrintMessage(ReceiveMessage);
  }
  TFLuna_Frame_Rate();
  if(ReceiveResponse(ReceiveMessage)){
    UART_OutString("Rate="); UART_OutUDec(TFLunaRate);UART_OutChar(' '); PrintMessage(ReceiveMessage);
  }
  TFLuna_SaveSettings();
  if(ReceiveResponse(ReceiveMessage)){
    UART_OutString("Save     "); PrintMessage(ReceiveMessage);
  }
  TFLuna_System_Reset();
  if(ReceiveResponse(ReceiveMessage)){
    UART_OutString("Reset    "); PrintMessage(ReceiveMessage);
  }
  while(1){
    if(ReceiveData(DataMessage)){
      sum = sum+DataMessage[3]*256+DataMessage[2];
      if((TFLunaOk%TFSIZE)==0){
        TFDistance = sum/TFSIZE;
        //UART_OutString("Data     "); PrintData(DataMessage);
        UART_OutUDec(TFDistance); UART_OutString("\n\r");
        sum = 0;
      }
      TFLunaOk++;
    }
  }
}
#endif
//************************
// main2 is real-time version with not UART0 debugging output
void ProcessDistance(uint32_t distance){
  TFDistance = distance;
  TFLunaOk++;
}
void ProcessDistance2(uint32_t distance){
  TFDistance2 = distance;
  TFLunaOk2++;
}
int main2(void){ // minimum functionality to run in real time
  DisableInterrupts();
  PLL_Init(Bus80MHz);       // set system clock to 80 MHz
  TFLuna_Init(&ProcessDistance);         // initialize UART1, real-time mode mode
  EnableInterrupts();       // Enable interrupts
  TFLuna_Format_Standard_mm();
  TFLuna_Frame_Rate();      // 100 samples/sec
  TFLuna_SaveSettings();
  TFLuna_System_Reset();
  while(1){
  }
}

//************************
// use main3 to estimate time to perform measurements
void Timer0A_Init(void){volatile uint32_t delay;
  SYSCTL_RCGCTIMER_R |= 0x01;      // 0) activate timer0
  delay = SYSCTL_RCGCTIMER_R;            // allows time to finish activating)
  TIMER0_CTL_R &= ~0x00000001;     // 1) disable timer0A during setup
  TIMER0_CFG_R = 0x00000000;       // 2) configure for 32-bit timer mode
  TIMER0_TAMR_R = 0x00000002;      // 3) configure for periodic mode, default down-count settings
  TIMER0_TAILR_R = 0xFFFFFFFF;     // 4) reload value
  TIMER0_TAPR_R = 0;               // 5) 12.5ns timer0A
  TIMER0_ICR_R = 0x00000001;       // 6) clear timer0A timeout flag
  TIMER0_IMR_R = 0x00000000;       // 7) disar, timeout interrupt
  TIMER0_CTL_R |= 0x00000001;      // 10) enable timer0A
}
#if UART
// overhead of TFLuna will be 17us/measurement at 100 measurements/sec
uint32_t start,stop,noLuna,Luna;
int main3(void){
  DisableInterrupts();
  PLL_Init(Bus80MHz);       // set system clock to 80 MHz
  UART_Init(); // debugging
  UART_OutString("TF Luna Overhead Test\n\r");
  while(1){
    DisableInterrupts();
    Timer0A_Init();
    start = TIMER0_TAR_R;
    for(int i=0; i<5000000;i++){
    }
    noLuna = start-TIMER0_TAR_R;
  //  UART_OutString(" No  Luna time = "); UART_OutUDec(noLuna); UART_OutString(" cycles\n\r");
    TFLuna_Init(&ProcessDistance);         // initialize UART1, real-time mode mode
    EnableInterrupts();       // Enable interrupts
    TFLuna_Format_Standard_mm();
    TFLuna_Frame_Rate();
    TFLuna_SaveSettings();
    TFLuna_System_Reset();
    Timer0A_Init();
    TFLunaOk = 0;
    start = TIMER0_TAR_R;
    for(int i=0; i<5000000;i++){
    }
    Luna = start-TIMER0_TAR_R;
    DisableInterrupts();

  //  UART_OutString("With Luna time = "); UART_OutUDec(Luna); UART_OutString(" cycles\n\r");
    uint32_t Difference = Luna-noLuna;
    uint32_t CyclesPerMeasurement = Difference/TFLunaOk;
    UART_OutString("Diff = "); UART_OutUDec(Difference); UART_OutString(" cycles, TFLunaOk= "); UART_OutUDec(TFLunaOk); UART_OutString(", ");
    UART_OutString("each = "); UART_OutUDec(CyclesPerMeasurement/80); UART_OutString(" usec\n\r");
  }
}
#endif
int main(void){ // debug with ST7735
	uint32_t sum=0; uint32_t i=0;
  DisableInterrupts();
  PLL_Init(Bus80MHz);       // set system clock to 80 MHz
  ST7735_InitR(INITR_REDTAB);
  ST7735_DrawString(0,0,"TF Luna Test",ST7735_YELLOW);
  ST7735_DrawString(0,1,"PC5->RxD, PC4<-TxD",ST7735_YELLOW);
	ST7735_SetCursor(0,3); ST7735_OutString("d= "); 
	ST7735_OutUDec4(TFDistance); ST7735_OutString(" mm ");
	TFLunaOk = 0;
  TFLuna_Init(&ProcessDistance);         // initialize UART1, real-time mode mode
  EnableInterrupts();       // Enable interrupts
  TFLuna_Format_Standard_mm();
  TFLuna_Frame_Rate();      // 100 samples/sec
  TFLuna_SaveSettings();
  TFLuna_System_Reset();
  while(1){
		if(TFLunaOk){
			TFLunaOk = 0;
			sum = sum + TFDistance; // LPF
			i++;
			if(i >= TFSIZE){
				i = 0;
				ST7735_SetCursor(3,3);
			  ST7735_OutUDec4(sum/TFSIZE);
				sum = 0;
			}
		}
  }
}
int main5(void){ // debug with ST7735
	uint32_t sum=0; uint32_t sum2=0; uint32_t i=0;uint32_t i2=0;
  DisableInterrupts();
  PLL_Init(Bus80MHz);       // set system clock to 80 MHz
  ST7735_InitR(INITR_REDTAB);
  ST7735_DrawString(0,0,"TF Luna Test",ST7735_YELLOW);
  ST7735_DrawString(0,1,"PC5->RxD, PC4<-TxD",ST7735_YELLOW);
  ST7735_DrawString(0,2,"PC7->RxD, PC6<-TxD",ST7735_YELLOW);
	ST7735_SetCursor(0,4); ST7735_OutString("d=  "); 
	ST7735_OutUDec4(TFDistance); ST7735_OutString(" mm ");
	ST7735_SetCursor(0,5); ST7735_OutString("d2= "); 
	ST7735_OutUDec4(TFDistance2); ST7735_OutString(" mm ");
	TFLunaOk = 0; TFLunaOk2 = 0;
  TFLuna_Init(&ProcessDistance);         // initialize UART1, real-time mode mode
  TFLuna2_Init(&ProcessDistance2);         // initialize UART1, real-time mode mode
  EnableInterrupts();       // Enable interrupts
  TFLuna_Format_Standard_mm();
  TFLuna_Frame_Rate();      // 100 samples/sec
  TFLuna_SaveSettings();
  TFLuna_System_Reset();
  while(1){
		if(TFLunaOk){
			TFLunaOk = 0;
			sum = sum + TFDistance; // LPF
			i++;
			if(i >= TFSIZE){
				i = 0;
				ST7735_SetCursor(3,4);
			  ST7735_OutUDec4(sum/TFSIZE);
				sum = 0;
			}
		}
		if(TFLunaOk2){
			TFLunaOk2 = 0;
			sum2 = sum2 + TFDistance2; // LPF
			i2++;
			if(i2 >= TFSIZE){
				i2 = 0;
				ST7735_SetCursor(3,5);
			  ST7735_OutUDec4(sum2/TFSIZE);
				sum2 = 0;
			}
		}
  }
}