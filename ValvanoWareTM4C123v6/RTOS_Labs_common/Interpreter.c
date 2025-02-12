// *************Interpreter.c**************
// Students implement this as part of EE445M/EE380L.12 Lab 1,2,3,4 
// High-level OS user interface
// 
// Runs on LM4F120/TM4C123
// Jonathan W. Valvano 1/18/20, valvano@mail.utexas.edu
#include <stdint.h>
#include <string.h> 
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "../RTOS_Labs_common/OS.h"
#include "../RTOS_Labs_common/ST7735.h"
#include "../RTOS_Labs_common/ADC.h"
#include "../inc/ADCT0ATrigger.h"
#include "../inc/ADCSWTrigger.h"
#include "../RTOS_Labs_common/UART0int.h"
#include "../RTOS_Labs_common/eDisk.h"
#include "../RTOS_Labs_common/eFile.h"


#define CMD_NAME_LEN_MAX 64
#define HELP_MSG_MAX_LEN 256 // Note: probably want to find a better solution for this storage later rather than preallocating everything
#define ARG_LEN_MAX 16




int ADC(int num_args, ...);
int lcd(int num_args, ...);
int print_help(int num_args, ...);
int clear_screen(int num_args, ...);
int time(int num_args, ...);
int ADC_Channel(int num_args, ...);
int time_reset(int num_args, ...);
int jitter_hist(int num_args, ...);
int max_jitter(int num_args, ...);
int num_threads(int num_args, ...);
void Jitter(int32_t MaxJitter, uint32_t const JitterSize, uint32_t JitterHistogram[]);

typedef struct Interpreter_Command {
	char name[CMD_NAME_LEN_MAX];
	int(*func)(int num_args, ...);
	char help_message[HELP_MSG_MAX_LEN];
} Command;


const Command commands[] = {
	{"adc_in", &ADC,
	"adc_in buf num_samples\r\n\tSample [num_samples] times from ADC0\r\n\n"},
	{"adc_channel", &ADC_Channel, ""},
	
	{"time", &time, "todo"},
	{"time_reset", &time_reset, "todo"},
	{"lcd", &lcd, "todo"},
	{"max_jitter", &max_jitter, "todo"},
	{"num_threads", &num_threads, "todo"},
	
	{"help", &print_help,
		"help\r\n\tPrints all help strings\r\n\n"},
	
	{"clear", &clear_screen,
	"clear\r\n\tNo arguments, clears the screen\r\n\n"},	
	
	// Sentinel function, do not replace or move from last spot
	{"exit", 0,
	"exit\r\n\tExits\r\n\n"} 
};

extern void DisableInterrupts();
extern void EnableInterrupts();

// *********** Command line interpreter (shell) ************
void Interpreter(void){ 

	while(1) {
		// Read Command
		char line[512];
		printf("[command line]: ");
		UART_InString(line, 512);
		printf("\r\n"); //Flush
		
		// Allocate space for all parseable instructions
		char cmd_name[CMD_NAME_LEN_MAX];
		int num_args = 0;
		char args[16 * 8];
		
		char *arg0 = args;
		char *arg1 = args + ARG_LEN_MAX;
		char *arg2 = args + ARG_LEN_MAX*2;
		char *arg3 = args + ARG_LEN_MAX*3;
		char *arg4 = args + ARG_LEN_MAX*4;
		char *arg5 = args + ARG_LEN_MAX*5;
		char *arg6 = args + ARG_LEN_MAX*6;
		char *arg7 = args + ARG_LEN_MAX*7;
		
		// Parse line
		int i = 0, j = 0, k = 0;
		
		while(line[j] != ' ' && line[j] != 0) {
			cmd_name[i++] = line[j++];		
		}
		cmd_name[i] = 0;
		
		
		while(line[j] != 0)
		{
			i = 0;
			while(line[j] == ' ' && line[j] != 0) {j++;} // Consume all whitespace
				
			if(line[j] == '"') {
				//Arg is a string, parse until end quote
				j++;
				while(line[j] != '"') {
					arg0[16*k + i] = line[j];
					i++;
					j++;
				}
				j++; // move past the quote
			}
			else { // parse until next whitespace
				while(line[j] != ' ' && line[j] != 0) {
					arg0[16*k + i] = line[j];
					i++;
					j++;
				}
			}
			arg0[16*k + i] = 0; // Ensure null terminated string
			k += 1; //Move to next argument
		}
	
		
		int func_found = 0;
		Command cmd = commands[0];
		for(int i = 0; strcmp(cmd.name, "exit") != 0; cmd=commands[++i]) {	
			if(strcmp(cmd.name, cmd_name) == 0) {
				// We have found the function that we are wanting to call, call it with all (found) arguments
				int code = cmd.func(num_args, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
				if(code != 0)
					printf("ERROR, FUNCTION NOT FINISHED. Code: %d", code);
				func_found=1;
				break;
			}
		}
		if(!func_found) {
			if(strcmp(cmd_name, "exit")==0) {
				return;
			}
			printf("\r\nCommand not implemented yet: \"%s\"\r\n", line);
		}
	}
}

int lcd(int num_args, ...) {
	va_list args;
	va_start(args, num_args);
	uint32_t dev = strtoul(va_arg(args, char*), NULL, 10);
	uint32_t line = strtoul(va_arg(args, char*), NULL, 10);
	char *msg = va_arg(args, char*);
	int32_t val = atoi(va_arg(args, char*));
	va_end(args);
	
	
	ST7735_Message(dev, line, msg, val);
	return 0;
}

int time_reset(int num_args, ...) {
	OS_ClearMsTime();
	return 0;
}

// Print jitter histogram
void Jitter(int32_t MaxJitter, uint32_t const JitterSize, uint32_t JitterHistogram[]){
  // write this for Lab 3 (the latest)
	
}

int jitter_hist(int num_args, ...) {
	// Call Jitter() after gathering the relevant data
	return 1;
}

int max_jitter(int num_args, ...) {
	int32_t j = OS_get_max_jitter();
	printf("Max_Jitter: %d\n", j);
	return 0;
}

int num_threads(int num_args, ...) {
	uint16_t nt = OS_get_num_threads();
	printf("Num_Threads: %u\n", nt);
	return 0;
}

int time(int num_args, ...) {
	uint32_t t = OS_MsTime();
	printf("OS Time: %u(ms)\r\n", t);
	return 0;
}

int clear_screen(int num_args, ...) {
	printf("\033c");
	return 0;
}



int ADC(int num_args, ...) {
	va_list args;
	va_start(args, num_args);
	uint32_t num_samples = strtoul(va_arg(args, char*), NULL, 10);
	va_end(args);

	printf("Collecting %d samples from ADC...\r\n", num_samples);
	uint32_t total = 0;
	for(uint32_t i = 0; i < num_samples; i++) {
		// Collect a sample
		uint32_t sample = ADC_In();
		total += sample;
		printf("\t[%d]: %u\r\n", i, sample);
	}
	uint32_t avg = total / num_samples;
	float avg_volts = ADC_toVolts(avg);
	printf("\tAvg: %u (%fV)\r\n", avg, avg_volts);
	return 0;
}

int ADC_Channel(int num_args, ...) {
	uint32_t c = ADC_getChannel();
	printf("\tChannel Num: %d\r\n", c);
	return 0;
}

int print_help(int num_args, ...) {
	
	// Note: There exists a race condition here, but unsure as to what it could be.
	// Only ISRs being called should be adhering to AAPCS, so they shouldnt prematurely exit this function...
	printf("HELP\r\n===========================================\r\n\n");
	Command cmd = commands[0];
	for(int i = 0; strcmp(cmd.name, "exit") != 0; cmd=commands[++i]) {
		printf("%s\r\n", cmd.name);
	}
	return 0;
}

	