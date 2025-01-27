// *************ADC.c**************
// EE445M/EE380L.6 Labs 1, 2, Lab 3, and Lab 4 
// mid-level ADC functions
// you are allowed to call functions in the low level ADCSWTrigger driver
// 
// Runs on LM4F120/TM4C123
// Jonathan W. Valvano Jan 5, 2020, valvano@mail.utexas.edu
#include <stdint.h>
#include "../inc/ADCSWTrigger.h"
#include "../inc/tm4c123gh6pm.h"


// Device globals, can be moved to a struct later to virtualize the driver
static uint32_t channel = -1;


// channelNum (0 to 11) specifies which pin is sampled with sequencer 3
// software start
// return with error 1, if channelNum>11, 
// otherwise initialize ADC and return 0 (success)
int ADC_Init(uint32_t channelNum){	
	volatile uint32_t delay;
	uint32_t pinID;
	SYSCTL_RCGCADC_R |= 0x1; // Turn on adc clock
	
	channel = channelNum;
	
//	// Set up appropriate pin / port 
	switch(channelNum) {
		case 0: //E3
			pinID = 0x8;
		case 1: //E2
			pinID = 0x4;
		case 2: //E1
			pinID = 0x2;
		case 3: //E0
			pinID = 0x1;
    case 8: //E5
      pinID = 0x20;
    case 9:  //E4
      pinID = 0x10;
		
	
			// Port E Set up
			SYSCTL_RCGCGPIO_R |= 0x10;   // Turn on clock 
			delay = SYSCTL_RCGCGPIO_R;
			GPIO_PORTE_DIR_R &= ~pinID;  //PEx set to input
			GPIO_PORTE_AFSEL_R |= pinID; //PEx alternate function enabled
			GPIO_PORTE_DEN_R &= ~pinID;  //Diable digital input on PEx
			GPIO_PORTE_AMSEL_R |= pinID; //Enable analog mode
			break;
		
		
		case 4: //D3
			pinID = 0x8;
		case 5: //D2
			pinID = 0x4;
		case 6: //D1
			pinID = 0x2;
		case 7: //D0
			pinID = 0x1;
		
	
			// Port D Set up
			SYSCTL_RCGCGPIO_R |= 0x8;    // Turn on clock 
			delay = SYSCTL_RCGCGPIO_R;
			GPIO_PORTD_DIR_R &= ~pinID;  //PDx set to input
			GPIO_PORTD_AFSEL_R |= pinID; //PDx alternate function enabled
			GPIO_PORTD_DEN_R &= ~pinID;  //Diable digital input on PDx
			GPIO_PORTD_AMSEL_R |= pinID; //Enable analog mode
			break;
	
		
		case 10: //B4
			pinID = 0x10;
		case 11: //B5
			pinID = 0x20;

			// Port B Set up
			SYSCTL_RCGCGPIO_R |= 0x2; 	 // Turn on clock 
			delay = SYSCTL_RCGCGPIO_R;
			GPIO_PORTB_DIR_R &= ~pinID;  //PDx set to input
			GPIO_PORTB_AFSEL_R |= pinID; //PDx alternate function enabled
			GPIO_PORTB_DEN_R &= ~pinID;  //Diable digital input on PDx
			GPIO_PORTB_AMSEL_R |= pinID; //Enable analog mode
			break;
			
		default:
			return 1; // Unsupported channel num
	}

//	pinID = 0x8;
//		
//	// HARD CODED FOR NOW
//	SYSCTL_RCGCGPIO_R |= 0x10;   // Turn on clock 
//	delay = SYSCTL_RCGCGPIO_R;
//	GPIO_PORTE_DIR_R &= ~pinID;  //PEx set to input
//	GPIO_PORTE_AFSEL_R |= pinID; //PEx alternate function enabled
//	GPIO_PORTE_DEN_R &= ~pinID;  //Diable digital input on PEx
//	GPIO_PORTE_AMSEL_R |= pinID; //Enable analog mode
	
	
	ADC0_PC_R = 0x1;						// Set sampling speed to 125kHz
	ADC0_SSPRI_R = 0x3210; 			// Make sequencer 3 low priority
	
	ADC0_ACTSS_R &= ~0x8; // Disable sequencer 3
	ADC0_EMUX_R  &= ~0xF000; // Specify software trigger on sequencer 3
	ADC0_SSMUX3_R = channelNum;	// Set channel number
	ADC0_SSCTL3_R = 0x6; 				// Enable IE0 so that flag is set when conversion finished
	ADC0_ACTSS_R |= 0x8; 				// Enable sequencer 3
  return 0;
}

uint32_t ADC_getChannel(void) {
	return channel;
}

// software start sequencer 3 and return 12 bit ADC result
uint32_t ADC_In(void){

	ADC0_PSSI_R |= 0x8; // Write 8 to ADC_PSSI_R to initate conversion on s3
	while((ADC0_RIS_R & 0x8) == 0){} // busy wait for sequencer to finish
	ADC0_ISC_R |= 0x8; //Acknowledge
  volatile uint32_t data = ADC0_SSFIFO3_R;
  return data; // Return read result
}

float ADC_toVolts(uint32_t data){
	static const int inverse_resolution = 1241; // 2^12 / 3.3V =1241.212121 ~= 1241, accurate to 4 sig figs, good enough
	float ret = data;
	return ret / inverse_resolution;
	
}
