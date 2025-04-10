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
#include "../RTOS_Lab5_ProcessLoader/loader.h"
#include "../RTOS_Labs_common/esp8266.h"
#include "Interpreter.h"


#define CMD_NAME_LEN_MAX 20
#define ARG_LEN_MAX 16
#define MAX_LINE_LEN 128

#define arg0 (args+ARG_LEN_MAX*0)
#define arg1 (args+ARG_LEN_MAX*1)
#define arg2 (args+ARG_LEN_MAX*2)
#define arg3 (args+ARG_LEN_MAX*3)
#define arg4 (args+ARG_LEN_MAX*4)
#define arg5 (args+ARG_LEN_MAX*5)
#define arg6 (args+ARG_LEN_MAX*6)
#define arg7 (args+ARG_LEN_MAX*7)


// Debug flag to re-format the disk every time we launch
#define EMERGENCY 0

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
int int_time_reset(int num_args, ...);
int int_time(int num_args, ...);
int ls(int num_args, ...);
int cd(int num_args, ...);
int cat(int num_args, ...);
int rm(int num_args, ...);
int touch(int num_args, ...);
int mkdir(int num_args, ...);
int save(int num_args, ...);
int append(int num_args, ...);
int format_drive(int num_armgs, ...);
int run(int num_args, ...);

// TODO Move help messages into a file

typedef struct Interpreter_Command {
	char name[CMD_NAME_LEN_MAX];
	int(*func)(int num_args, ...);
} Command;


const Command commands[] = {
	{"adc_in", &ADC}, 										// "adc_in buf num_samples\r\n\tSample [num_samples] times from ADC0\r\n\n"},
	{"adc_channel", &ADC_Channel}, 				// "todo"},
	{"time", &time}, 											// "todo"},
	{"time_reset", &time_reset}, 					// "todo"},
	{"lcd", &lcd}, 												// "todo"},
	{"max_jitter", &max_jitter}, 					// "todo"},
	{"num_threads", &num_threads}, 				// "todo"},
	{"int_time", &int_time}, 							// "int_time <disabled=0/enabled=1> <total=0/percentage=1>\r\n"},
	{"int_time_reset", &int_time_reset}, 	//"int_time_reset\r\n\tReset the counters tracking how long interrupts are disabled\r\n"},
	{"jitter_hist", &jitter_hist}, 				// "jitter_hist <id> <lcd_id>\r\n\t" "id: ID of jitter tracker to print out\r\n\t"},
	{"help", &print_help}, 								//"help\r\n\tPrints all help strings\r\n\n"},
	{"clear", &clear_screen},							// "clear\r\n\tNo arguments, clears the screen\r\n\n"},	
	{"save", &save},
	{"format_drive", &format_drive},
	{"run", &run},
	
#if EFILE_H
	{"ls", &ls},
	{"cd", &cd},
	{"cat", &cat},
	{"rm", &rm},
	{"touch", &touch}, 
	{"mkdir", &mkdir},
	{"append", &append},
#endif
	
	// Sentinel function, do not replace or move from last spot
	{"exit", 0} 													//, "exit\r\n\tExits\r\n\n"} 
};

extern void DisableInterrupts();
extern void EnableInterrupts();

int TELNET_SERVER_ID = 0;

void Interpreter_Out(char *s) {
	if(OS_Id() == TELNET_SERVER_ID) {
		ESP8266_Send(s);
	}
	else {
		UART_OutString(s);
	}
}

void Interpreter_In(char *s, uint32_t n) {
	if(OS_Id() == TELNET_SERVER_ID) {
		ESP8266_Receive(s, n);
	}
	else {
		UART_InString(s, n);
	}
}

#if EFILE_H
void pwd(void) {
	Dir_t d;
	Dir_t parent;
	DirEntry_t de;
	eFile_OpenCurrentDir(&d);
	eFile_Open("..", &parent);
	if(d.iNode->sector_num == parent.iNode->sector_num) {
		// Only the root is its own parent
		Interpreter_Out("[root] > ");
	}
	else {
		eFile_D_lookup_by_sector(&parent, d.iNode->sector_num, &de);
		char s[32];
		sprintf(s, "[%s] > ", de.name);
		Interpreter_Out(s);
	}
	eFile_D_close(&parent);
	eFile_D_close(&d);
}
#endif


void Interpreter_register_remote_thread(void) {
	TELNET_SERVER_ID = OS_Id();
}

void Interpreter_unregister_remote_thread(void) {
	TELNET_SERVER_ID = 0;
}



// *********** Command line interpreter (shell) ************
void Interpreter(void){ 
	char* line = malloc(MAX_LINE_LEN);
	char* cmd_name = malloc(CMD_NAME_LEN_MAX);
	char* args = malloc(16*8);
	
	#if EMERGENCY
	Interpreter_Out("Formatting the disk...\r\n");
	format_drive(0);
	#endif
	
	while(1) {
		// Read Command
		#if EFILE_H
		pwd();
		#else
		Interpreter_Out("[command] > "); 
		#endif
		
		Interpreter_In(line, MAX_LINE_LEN);
		Interpreter_Out("\r\n"); //Flush
		
		// Allocate space for all parseable instructions
		
		int num_args = 0;
		
		
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
				if(code != 0) {
					char buff[45];
					sprintf(buff, "ERROR, FUNCTION NOT FINISHED. Code: %d", code);
					Interpreter_Out(buff);
				}
				func_found=1;
				break;
			}
		}
		if(!func_found) {
			if(strcmp(cmd_name, "exit")==0) {
				break;
			}
			Interpreter_Out("\r\nCommand not implemented yet\r\n");
		}
	}
	
	free(line);
	free(args);
	free(cmd_name);
}

int format_drive(int num_armgs, ...) {
	eFile_Format();
	return 0;
}

int save(int num_args, ...) {
	eFile_Unmount();
	return 0;
}


static const ELFSymbol_t symtab[] = {
	{"ST7735_Message", ST7735_Message}
};
int run(int num_args, ...) {
	va_list args;
	va_start(args, num_args);
	char *path = va_arg(args, char*);
	va_end(args);
	
	ELFEnv_t env = {symtab, 1};
	if(!exec_elf(path, &env)) {return 1;}
	return 0;
}

#if EFILE_H
int ls(int num_args, ...) {
	Dir_t d;
	eFile_Open(".", &d);
	
	char fn[MAX_FILE_NAME_LENGTH+1];
	uint32_t sz;
	while(eFile_D_read_next(&d, fn, &sz)) {
		char buf[128];
		char dots[] = "..................................................................";
		dots[MAX_FILE_NAME_LENGTH - strlen(fn)] = 0;
		sprintf(buf, "%s%s%d\r\n", fn, dots, sz);
		Interpreter_Out(buf);
	}
	
	eFile_D_close(&d);
	return 0;
}

int cd(int num_args, ...) {
	va_list args;
	va_start(args, num_args);
	char *path = va_arg(args, char*);
	va_end(args);

	eFile_CD(path);
	return 0;
}

int cat(int num_args, ...) {
	va_list args;
	va_start(args, num_args);
	char *path = va_arg(args, char*);
	va_end(args);
	
	File_t f;
	eFile_Open(path, &f);
	char buf[128];
	while(eFile_F_read(&f, buf, 128)) {
		Interpreter_Out(buf);
		memset(buf, 0, 128);
	}
	Interpreter_Out("\r\n"); // Flush out
	return 0;
}


int append(int num_args, ...) {
	va_list args;
	va_start(args, num_args);
	char * fp = va_arg(args, char*);
	char *text = va_arg(args, char*);
	va_end(args);
	
	File_t f;
	eFile_Open(fp, &f);
	eFile_F_write(&f, text, strlen(text));
	eFile_F_close(&f);
	
	return 0;
}

int rm(int num_args, ...) {
	va_list args;
	va_start(args, num_args);
	char *path = va_arg(args, char*);
	va_end(args);
	
	return eFile_Remove(path) != 1;
}

int touch(int num_args, ...) {
	va_list args;
	va_start(args, num_args);
	char *path = va_arg(args, char*);
	va_end(args);
	
	return eFile_Create(path) != 1;
}

int mkdir(int num_args, ...) {
	va_list args;
	va_start(args, num_args);
	char *path = va_arg(args, char*);
	va_end(args);
	
	return eFile_CreateDir(path) != 1;
}
#endif

int int_time_reset(int num_args, ...) {
	// No args
	OS_reset_int_time();
	return 0;
}


int int_time(int num_args, ...) {
	va_list args;
	va_start(args, num_args);
	uint32_t enabled = strtoul(va_arg(args, char*), NULL, 10);
	uint32_t percentage = strtoul(va_arg(args, char*), NULL, 10);
	va_end(args);
	
	if(enabled > 1 || percentage > 1) {
		Interpreter_Out("Error: <enabled> and <percentage> must be 0 or 1");
	}
	
	double p;
	uint32_t t;
	char s[64];
	switch(2*enabled + percentage) {
		case 0b11:
			p = OS_get_percent_time_ints_enabled();
			sprintf(s, "%%time interrupts  enabled: %.2f\r\n", p);
			Interpreter_Out(s);
			break;
		
		case 0b10:
			t = OS_get_time_ints_enabled();
			sprintf(s, "total time interrupts enabled: %d(us)", t);
			Interpreter_Out(s);
			break;
		
		case 0b01:
			p = OS_get_percent_time_ints_disabled();
			sprintf(s, "%%time interrupts disabled: %.2f\r\n", p);
			Interpreter_Out(s);
			break;
		
		case 0b00:
			t = OS_get_time_ints_disabled();
			sprintf(s, "total time interrupts disabled: %d(us)", t);
			Interpreter_Out(s);
			break;	
	}
	
	return 0;
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

// Print jitter histogram on specified lcd
void Jitter(Jitter_t* J, uint8_t lcd_id){
	
	ST7735_Message(lcd_id, 0, "max_jitter= ", J->maxJitter);
	int j = 1;
	for(int i = 0; i < JITTERSIZE; i++) {
		uint32_t cnt = J->JitterHistogram[i];
		if(cnt == 0)
			continue;
		
		char buf[9];
		sprintf(buf, "%d(%s) | ", i, J->unit); 
		ST7735_Message(lcd_id, j, buf, cnt);
		j++;
	}
}

int jitter_hist(int num_args, ...) {
	va_list args;
	va_start(args, num_args);
	uint8_t id = strtoul(va_arg(args, char*), NULL, 10);
	uint8_t lcd_id = strtoul(va_arg(args, char*), NULL, 10);
	va_end(args);
	
	Jitter_t* J = OS_get_jitter_struct(id);
	Jitter(J, lcd_id);
	return 0;
}

int max_jitter(int num_args, ...) {
	va_list args;
	va_start(args, num_args);
	uint32_t id = strtoul(va_arg(args, char*), NULL, 10);
	va_end(args);
	char s[64];
	
	Jitter_t* J = OS_get_jitter_struct(id);
	sprintf(s, "Max_Jitter: %d\r\n", J->maxJitter);
	Interpreter_Out(s);
	return 0;
}

int num_threads(int num_args, ...) {
	uint16_t nt = OS_get_num_threads();
	char s[64];
	sprintf(s, "Num_Threads: %u\r\n", nt);
	Interpreter_Out(s);
	return 0;
}

int time(int num_args, ...) {
	uint32_t t = OS_MsTime();
	char s[64];
	sprintf(s, "OS Time: %u(ms)\r\n", t);
	Interpreter_Out(s);
	return 0;
}

int clear_screen(int num_args, ...) {
	Interpreter_Out("\033c");
	return 0;
}



int ADC(int num_args, ...) {
	va_list args;
	va_start(args, num_args);
	uint32_t num_samples = strtoul(va_arg(args, char*), NULL, 10);
	va_end(args);

	char s[65];
	sprintf(s, "Collecting %d samples from ADC...\r\n", num_samples);
	Interpreter_Out(s);
	uint32_t total = 0;
	for(uint32_t i = 0; i < num_samples; i++) {
		// Collect a sample
		uint32_t sample = ADC_In();
		total += sample;
		sprintf(s, "\t[%d]: %u\r\n", i, sample);
		Interpreter_Out(s);
	}
	uint32_t avg = total / num_samples;
	float avg_volts = ADC_toVolts(avg);
	sprintf(s, "\tAvg: %u (%fV)\r\n", avg, avg_volts);
	Interpreter_Out(s);
	return 0;
}

int ADC_Channel(int num_args, ...) {
	uint32_t c = ADC_getChannel();
	char s[64];
	sprintf(s, "\tChannel Num: %d\r\n", c);
	Interpreter_Out(s);
	return 0;
}

int print_help(int num_args, ...) {
	Interpreter_Out("HELP\r\n===========================================\r\n\n");
	Command cmd = commands[0];
	for(int i = 0; strcmp(cmd.name, "exit") != 0; cmd=commands[++i]) {
		char s[32];
		sprintf(s, "%s\r\n", cmd.name);
		Interpreter_Out(s);
	}
	return 0;
}

	