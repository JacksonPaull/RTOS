// *************os.c**************
// EE445M/EE380L.6 Labs 1, 2, 3, and 4 
// High-level OS functions
// Students will implement these functions as part of Lab
// Runs on LM4F120/TM4C123
// Jonathan W. Valvano 
// Jan 12, 2020, valvano@mail.utexas.edu


#include <stdint.h>
#include <stdio.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/CortexM.h"
#include "../inc/PLL.h"
#include "../inc/LaunchPad.h"
#include "../inc/Timer4A.h"
#include "../inc/WTimer0A.h"
#include "../RTOS_Labs_common/OS.h"
#include "../RTOS_Labs_common/ST7735.h"
#include "../inc/ADCT0ATrigger.h"
#include "../RTOS_Labs_common/UART0int.h"
#include "../RTOS_Labs_common/eFile.h"
#include "../inc/Timer0A.h"
#include "../RTOS_Lab2_RTOSkernel/LinkedList.h"
#include "../RTOS_Lab2_RTOSkernel/scheduler.h"


extern void ContextSwitch(TCB_t *next_thread);


// Performance Measurements 
int32_t MaxJitter;             // largest time jitter between interrupts in usec
#define JITTERSIZE 64
uint32_t const JitterSize=JITTERSIZE;
uint32_t JitterHistogram[JITTERSIZE]={0,};

//Note: current max run time will be 2^16 * 1000s ~= 2 years, which seems reasonable
//uint16_t OS_MsTimeResetCount = 0; // Number of times the OS timer has rolled over, allowing for greater run-times

uint32_t OS_MsCount = 0;
uint16_t thread_cnt = 0;

// Thread control structures
TCB_t threads[MAX_NUM_THREADS];
unsigned long stacks[MAX_NUM_THREADS][STACK_SIZE];

TCB_t *RunPt = 0; // Currently running thread
TCB_t *inactive_thread_list_head = 0;
// TCB_t *sleeping_thread_list_head;
// TCB_t *blocked_thread_list_head;

/*------------------------------------------------------------------------------
  Systick Interrupt Handler
  SysTick interrupt happens every 10 ms
  used for preemptive thread switch
 *------------------------------------------------------------------------------*/

 
void SysTick_Handler(void) {
	INTCTRL |= 1<<28; // Software trigger of PendSV interrupt
	
	// Decrement sleep counter(s)
		// TODO
  
} // end SysTick_Handler

unsigned long OS_LockScheduler(void){
  // lab 4 might need this for disk formating
  return 0;// replace with solution
}
void OS_UnLockScheduler(unsigned long previous){
  // lab 4 might need this for disk formating
}


void SysTick_Init(unsigned long period){
	STCTRL &= ~0x1; //Disable systick during setup
  STRELOAD = period-1;
	STCURRENT = 0;
	SYSPRI3 = (SYSPRI3 & ~0xEFFFFFFF) | 0x40000000; // Set to priority 2
	STCTRL|= 0x7; 
}

void PendSV_Handler(void) {
	// Find out which thread to schedule and context switch
	TCB_t *next_node = round_robin_scheduler(RunPt);
//	if(next_node == RunPt)
//		return;
	
	STCURRENT = 0; // Reset timer
	ContextSwitch(next_node);
}



void thread_init_stack(TCB_t* thread, void(*task)(void)) {
	int id = thread->id;
	unsigned long* stack = stacks[id];
	
	thread->sp = stack+STACK_SIZE-18*4; // Start at bottom of stack and init registers on stack 
	
	thread->sp[0]  = 0x04040404; //R4
	thread->sp[1]  = 0x05050505; //R5
	thread->sp[2]  = 0x06060606; //R6
	thread->sp[3]  = 0x07070707; //R7
	thread->sp[4]  = 0x08080808; //R8
	thread->sp[5]  = 0x09090909; //R9
	thread->sp[6]  = 0x10101010; //R10
	thread->sp[7]  = 0x11111111; //R11
	thread->sp[8]  = 0x00000000; //R0
	thread->sp[9]  = 0x01010101; //R1
	thread->sp[10] = 0x02020202; //R2
	thread->sp[11] = 0303030303; //R3
	thread->sp[12] = 0x12121212; //R12
	thread->sp[13] = (unsigned long) &OS_Kill; //LR - Note: any exiting thread should call OS_Kill
	thread->sp[14] = (unsigned long) task; // PC, should point to thread main task
	thread->sp[15] = 0xFFFFFF00; // PSR all bits set and indicate that we are in thread mode after popping this
	thread->sp[16] = MAGIC;
	thread->sp[17] = MAGIC;
}

void OS_thread_init(void) {
	TCB_t* thread = 0;
	TCB_t* prev_thread=0;
	
	for(int i = 0; i < MAX_NUM_THREADS; i++) {
		prev_thread = thread;
		thread = &threads[i];
		if(i==0) {
			TCB_LL_create_linear(&inactive_thread_list_head, thread);
		}
		else {
			TCB_LL_append_linear(inactive_thread_list_head, thread);
		}
		
		thread->id=++thread_cnt;
		thread->sleep_count = 0;
		
		// TODO init anything else from the thread
		// Note: Stacks are initialized when making the thread
			// This also ensures that any program which exits without 
			//first clearing the stack won't mess up any new threads
	}
}

/**
 * @details  Initialize operating system, disable interrupts until OS_Launch.
 * Initialize OS controlled I/O: serial, ADC, systick, LaunchPad I/O and timers.
 * Interrupts not yet enabled.
 * @param  none
 * @return none
 * @brief  Initialize OS
 */
void OS_Init(void){
	
	// init launch pad / pll
	PLL_Init(Bus80MHz);
	LaunchPad_Init();
	
	DisableInterrupts();
	
	// Init anything else used by OS
	OS_MsTime_Init();
	OS_thread_init();
	
	// Set PendSV priority to 2;
	SYSPRI3 = (SYSPRI3 &0xFF0FFFFF) | 0x00400000;
	
	//TODO Init UART, any OS controlled ADCs, etc

}; 

// ******** OS_InitSemaphore ************
// initialize semaphore 
// input:  pointer to a semaphore
// output: none
void OS_InitSemaphore(Sema4Type *semaPt, int32_t value){
  // put Lab 2 (and beyond) solution here

}; 

// ******** OS_Wait ************
// decrement semaphore 
// Lab2 spinlock
// Lab3 block if less than zero
// input:  pointer to a counting semaphore
// output: none
void OS_Wait(Sema4Type *semaPt){
  // put Lab 2 (and beyond) solution here
  
}; 

// ******** OS_Signal ************
// increment semaphore 
// Lab2 spinlock
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a counting semaphore
// output: none
void OS_Signal(Sema4Type *semaPt){
  // put Lab 2 (and beyond) solution here

}; 

// ******** OS_bWait ************
// Lab2 spinlock, set to 0
// Lab3 block if less than zero
// input:  pointer to a binary semaphore
// output: none
void OS_bWait(Sema4Type *semaPt){
  // put Lab 2 (and beyond) solution here

}; 

// ******** OS_bSignal ************
// Lab2 spinlock, set to 1
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a binary semaphore
// output: none
void OS_bSignal(Sema4Type *semaPt){
  // put Lab 2 (and beyond) solution here

}; 



//******** OS_AddThread *************** 
// add a foregound thread to the scheduler
// Inputs: pointer to a void/void foreground task
//         number of bytes allocated for its stack
//         priority, 0 is highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// stack size must be divisable by 8 (aligned to double word boundary)
// In Lab 2, you can ignore both the stackSize and priority fields
// In Lab 3, you can ignore the stackSize fields
int OS_AddThread(void(*task)(void), 
   uint32_t stackSize, uint32_t priority){
  // put Lab 2 (and beyond) solution here

	// Take first thread from active list
	TCB_t *thread = TCB_LL_pop_head_linear(&inactive_thread_list_head);
	if(thread == 0) 
			return 0; // Cannot pull anything from list
		 
	
	thread_init_stack(thread, task);
	
	if(RunPt == 0) {
		TCB_LL_create_circular(&RunPt, thread);
	}
	else {
		TCB_LL_append_circular(RunPt, thread);
	}
     
  return 1; 
};

//******** OS_AddProcess *************** 
// add a process with foregound thread to the scheduler
// Inputs: pointer to a void/void entry point
//         pointer to process text (code) segment
//         pointer to process data segment
//         number of bytes allocated for its stack
//         priority (0 is highest)
// Outputs: 1 if successful, 0 if this process can not be added
// This function will be needed for Lab 5
// In Labs 2-4, this function can be ignored
int OS_AddProcess(void(*entry)(void), void *text, void *data, 
  unsigned long stackSize, unsigned long priority){
  // put Lab 5 solution here

     
  return 0; // replace this line with Lab 5 solution
}


//******** OS_Id *************** 
// returns the thread ID for the currently running thread
// Inputs: none
// Outputs: Thread ID, number greater than zero 
uint32_t OS_Id(void){
  return RunPt->id;
};


//******** OS_AddPeriodicThread *************** 
// add a background periodic task
// typically this function receives the highest priority
// Inputs: pointer to a void/void background function
//         period given in system time units (12.5ns)
//         priority 0 is the highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// You are free to select the time resolution for this function
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal   OS_AddThread
// This task does not have a Thread ID
// In lab 1, this command will be called 1 time
// In lab 2, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, this command will be called 0 1 or 2 times
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddPeriodicThread(void(*task)(void), 
   uint32_t period, uint32_t priority){
  // put Lab 2 (and beyond) solution here
  
     
  return 0; // replace this line with solution
};


/*----------------------------------------------------------------------------
  PF1 Interrupt Handler
 *----------------------------------------------------------------------------*/
void GPIOPortF_Handler(void){
 
}

//******** OS_AddSW1Task *************** 
// add a background task to run whenever the SW1 (PF4) button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is the highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal   OS_AddThread
// This task does not have a Thread ID
// In labs 2 and 3, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddSW1Task(void(*task)(void), uint32_t priority){
  // put Lab 2 (and beyond) solution here
 
  return 0; // replace this line with solution
};

//******** OS_AddSW2Task *************** 
// add a background task to run whenever the SW2 (PF0) button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is highest, 5 is lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed user task will run to completion and return
// This task can not spin block loop sleep or kill
// This task can call issue OS_Signal, it can call OS_AddThread
// This task does not have a Thread ID
// In lab 2, this function can be ignored
// In lab 3, this command will be called will be called 0 or 1 times
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddSW2Task(void(*task)(void), uint32_t priority){
  // put Lab 2 (and beyond) solution here
    
  return 0; // replace this line with solution
};


// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// You are free to select the time resolution for this function
// OS_Sleep(0) implements cooperative multitasking
void OS_Sleep(uint32_t sleepTime){
  // put Lab 2 (and beyond) solution here
  

};  

// ******** OS_Kill ************
// kill the currently running thread, release its TCB and stack
// input:  none
// output: none
void OS_Kill(void){
  // put Lab 2 (and beyond) solution here
 
  EnableInterrupts();   // end of atomic section 
  for(;;){};        // can not return
    
}; 

// ******** OS_Suspend ************
// suspend execution of currently running thread
// scheduler will choose another thread to execute
// Can be used to implement cooperative multitasking 
// Same function as OS_Sleep(0)
// input:  none
// output: none
void OS_Suspend(void){
	// Trigger PendSV interrupt
	INTCTRL |= 1<<28;
};
  
// ******** OS_Fifo_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void OS_Fifo_Init(uint32_t size){
  // put Lab 2 (and beyond) solution here
   
  
};

// ******** OS_Fifo_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
//  this function can not disable or enable interrupts
int OS_Fifo_Put(uint32_t data){
  // put Lab 2 (and beyond) solution here

    return 0; // replace this line with solution
};  

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
uint32_t OS_Fifo_Get(void){
  // put Lab 2 (and beyond) solution here
  
  return 0; // replace this line with solution
};

// ******** OS_Fifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
int32_t OS_Fifo_Size(void){
  // put Lab 2 (and beyond) solution here
   
  return 0; // replace this line with solution
};


// ******** OS_MailBox_Init ************
// Initialize communication channel
// Inputs:  none
// Outputs: none
void OS_MailBox_Init(void){
  // put Lab 2 (and beyond) solution here
  

  // put solution here
};

// ******** OS_MailBox_Send ************
// enter mail into the MailBox
// Inputs:  data to be sent
// Outputs: none
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received 
void OS_MailBox_Send(uint32_t data){
  // put Lab 2 (and beyond) solution here
  // put solution here
   

};

// ******** OS_MailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty 
uint32_t OS_MailBox_Recv(void){
  // put Lab 2 (and beyond) solution here
 
  return 0; // replace this line with solution
};

// ******** OS_Time ************
// return the system time 
// Inputs:  none
// Outputs: time in 12.5ns units, 0 to 4294967295
// The time resolution should be less than or equal to 1us, and the precision 32 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_TimeDifference have the same resolution and precision 
uint32_t OS_Time(void){
  // put Lab 2 (and beyond) solution here

  return 0; // replace this line with solution
};

// ******** OS_TimeDifference ************
// Calculates difference between two times
// Inputs:  two times measured with OS_Time
// Outputs: time difference in 12.5ns units 
// The time resolution should be less than or equal to 1us, and the precision at least 12 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_Time have the same resolution and precision 
uint32_t OS_TimeDifference(uint32_t start, uint32_t stop){
  // put Lab 2 (and beyond) solution here

  return 0; // replace this line with solution
};



// Private helper function to increment reset counter
// This will be called every 1ms
// no overflow checking will be implemented to save a few clock cycles
// Overflow will happen at around 50hrs of runtime, which is ok
void MsTime_Helper(void) {
	//Increment counter
	OS_MsCount += 1;
}

# define OS_MS_TIMER_INIT 80000
// ********** OS_MsTime_Init **********
// Initializes the system clock in ms, through timer0A
// Inputs: None
// Outputs: None
void OS_MsTime_Init(void) {
	Timer0A_Init(&MsTime_Helper, OS_MS_TIMER_INIT, 1, 1);
}

// ******** OS_ClearMsTime ************
// Sets the system time to 0ms
// Inputs:  none
// Outputs: none
void OS_ClearMsTime(void){
	// Reset timer and counter to their initial config
  OS_MsCount = 0;
	TIMER0_TAV_R = OS_MS_TIMER_INIT;
};

// ******** OS_MsTime ************
// reads the current time in msec
// Inputs:  none
// Outputs: time in ms units
// You are free to select the time resolution for this function
// For Labs 2 and beyond, it is ok to make the resolution to match the first call to OS_AddPeriodicThread
uint32_t OS_MsTime(void){
	// TODO: Replace the MsTime function with a system timer on us resolution, and convert to ms here
	return OS_MsCount;
};


//******** OS_Launch *************** 
// start the scheduler, enable interrupts
// Inputs: number of 12.5ns clock cycles for each time slice
//         you may select the units of this parameter
// Outputs: none (does not return)
// In Lab 2, you can ignore the theTimeSlice field
// In Lab 3, you should implement the user-defined TimeSlice field
// It is ok to limit the range of theTimeSlice to match the 24-bit SysTick
void OS_Launch(uint32_t theTimeSlice){
			if(theTimeSlice > (1 << 24)) {
		return; // TODO Change to some kind of fault?
	}
	
  SysTick_Init(theTimeSlice);
	OS_ClearMsTime();

	// Prep first thread for entry into context switch
	__asm__(
		"LDR R0, =RunPt\n"
		"LDR R1, [R0]\n"
		"LDR SP, [R1, #12]\n"
		"POP {R4-R11}\n"
		//"SUBS SP, #8"		// Not sure why but entering context switch increments SP by 8 bytes?????
	);
	
	EnableInterrupts();
	INTCTRL |= 0x10000000;
	
	// Note: The OS will crash if it is not initialized with at least one thread
};

//************** I/O Redirection *************** 
// redirect terminal I/O to UART or file (Lab 4)

int StreamToDevice=0;                // 0=UART, 1=stream to file (Lab 4)

int fputc (int ch, FILE *f) { 
  if(StreamToDevice==1){  // Lab 4
    if(eFile_Write(ch)){          // close file on error
       OS_EndRedirectToFile(); // cannot write to file
       return 1;                  // failure
    }
    return 0; // success writing
  }
  
  // default UART output
  UART_OutChar(ch);
  return ch; 
}

int fgetc (FILE *f){
  char ch = UART_InChar();  // receive from keyboard
  UART_OutChar(ch);         // echo
  return ch;
}

//int putc (int ch, FILE *f) { 
//  if(StreamToDevice==1){  // Lab 4
//    if(eFile_Write(ch)){          // close file on error
//       OS_EndRedirectToFile(); // cannot write to file
//       return 1;                  // failure
//    }
//    return 0; // success writing
//  }
//  
//  // default UART output
//  UART_OutChar(ch);
//  return ch; 
//}

//int getc (FILE *f){
//  char ch = UART_InChar();  // receive from keyboard
//  UART_OutChar(ch);         // echo
//  return ch;
//}

int OS_RedirectToFile(const char *name){  // Lab 4
  eFile_Create(name);              // ignore error if file already exists
  if(eFile_WOpen(name)) return 1;  // cannot open file
  StreamToDevice = 1;
  return 0;
}

int OS_EndRedirectToFile(void){  // Lab 4
  StreamToDevice = 0;
  if(eFile_WClose()) return 1;    // cannot close file
  return 0;
}

int OS_RedirectToUART(void){
  StreamToDevice = 0;
  return 0;
}

int OS_RedirectToST7735(void){
  
  return 1;
}

