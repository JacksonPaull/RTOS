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


// Print jitter histogram
void Jitter(int32_t MaxJitter, uint32_t const JitterSize, uint32_t JitterHistogram[]){
  // write this for Lab 3 (the latest)
	
}


void ADC(int num_args, ...);

void print_help(int num_args, ...);
void clear_screen(int num_args, ...);

typedef struct Interpreter_Command {
	char name[CMD_NAME_LEN_MAX];
	void(*func)(int num_args, ...);
	char help_message[HELP_MSG_MAX_LEN];
} Command;


const Command commands[] = {
	{"ADC_In", &ADC,
	"ADC_In buf num_samples\r\n\tSample [num_samples] times from ADC0 and place into buffer at [buf]\r\n\n"},
	
	
	{"help", &print_help,
		"help\r\n\tPrints all help strings\r\n\n"},
	{"clear", &clear_screen,
	"clear\r\n\tNo arguments, clears the screen\r\n\n"},	
	
	// Sentinel function, do not replace or move from last spot
	{"exit", 0,
	"exit\r\n\tExits\r\n\n"} 
};


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
		for(int i = 0; i < 16 * 8; i++) args[i] = 0; // Clear out all args
		char *arg0 = args;
		char *arg1 = args + 16;
		char *arg2 = args + 16*2;
		char *arg3 = args + 16*3;
		char *arg4 = args + 16*4;
		char *arg5 = args + 16*5;
		char *arg6 = args + 16*6;
		char *arg7 = args + 16*7;
		
		// Parse line
		int i = 0, j = 0, k = 0;
		
		while(line[j] != ' ' && line[j] != 0) {
			cmd_name[i++] = line[j++];		
		}
		cmd_name[i] = 0;
		
		
		while(line[j] != 0)
		{
			i = 0;
			while(line[j] == ' ') {j++;} // Consume all whitespace
				
			while(line[j] != ' ' && line[j] != 0) {
				arg0[16*k + i] = line[j];	// Note: abusing compiler assumption that all the args will be one after the other on the stack, could be wrong
				i++;
				j++;
			}
			arg0[16*k + i] = 0; // Ensure null terminated string
			k += 1; //Move to next argument
		}
		
		//printf("Parsed command\r\n\t[%s] [%s] [%s] [%s] [%s]", cmd_name, arg0, arg1, arg2, arg3);

//		Command cmd = commands[0];
//		for(int i = 0; strcmp(cmd.name, "exit") != 0; cmd=commands[++i]) {	
//			if(strcmp(cmd.name, line) == 0) {
//				// We have found the function that we are wanting to call, call it with all (found) arguments
//				cmd.func(num_args, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
//			}
//		}
//		
//		if(strcmp(cmd.name, line)==0) {
//				return;
//		}
				
		
		printf("\r\nCommand not implemented yet: \"%s\"\r\n", line);
	}
}

void clear_screen(int num_args, ...) {
	printf("\033c");
}

void ADC(int num_args, ...) {
	
	
	va_list args;
	va_start(args, num_args);
	uint32_t num_samples = atoi(va_arg(args, char*));
	va_end(args);

	printf("Collecting %d samples from ADC...\r\n", num_samples);

	for(int i = 0; i < num_samples; i++) {
		// Collect a sample
		uint32_t sample = ADC_In();
		
		printf("\t[%d]: %d\r\n", i, sample);
	}
}

void print_help(int num_args, ...) {
	
	printf("HELP\r\n===========================================\r\n\n");
	Command cmd = commands[0];
	for(int i = 0; strcmp(cmd.name, "exit") != 0; cmd=commands[++i]) {
		printf("%s", cmd.help_message);
	}
	
}

	
	// lcd command
	