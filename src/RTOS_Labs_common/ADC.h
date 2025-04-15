// *************ADC.h**************
// EE445M/EE380L.6 Labs 1, 2, Lab 3, and Lab 4 
// mid-level ADC functions
// you are allowed to call functions in the low level ADCSWTrigger driver
// 
// Runs on LM4F120/TM4C123
// Jonathan W. Valvano Jan 5, 2020, valvano@mail.utexas.edu

#include <stdint.h>

// ********** ADC_Init **********
// Initalize ADC0 on sequencer 3 for specified pin
// See TM4c Manual page 801 for table
// Inputs: 
//   uint32_t channelNum: Which pin is sampled with ss3.
// Outputs:
//   int: 1 if error, 0 if success
int ADC_Init(uint32_t channelNum);

// ********** ADC_In **********
// Start sequencer through software, 
// wait for it to finish, and return sample
// Inputs: None
// Outputs: ADC sample
//	 note: 2^12 alternatives on 3.3V range
uint32_t ADC_In(void);

// ********** ADC_toVolts **********
// Convert digital ADC value to volts
// Inputs: 
//   uint32_t data: 12 bit adc data
// Outputs float: Volt (0-3.3V for valid ADC data)
float ADC_toVolts(uint32_t data);

// ********** ADC_getChannel **********
// Return the current channel for the ADC
// Inputs: None
// Outputs: uint32_t: Channel (0-11)
//   Note: See TM4c manual page 801
uint32_t ADC_getChannel(void);