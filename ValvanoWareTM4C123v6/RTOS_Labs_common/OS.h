/**
 * @file      OS.h
 * @brief     Real Time Operating System for Labs 1, 2, 3, 4 and 5
 * @details   EE445M/EE380L.6
 * @version   V1.0
 * @author    Valvano
 * @copyright Copyright 2020 by Jonathan W. Valvano, valvano@mail.utexas.edu,
 * @warning   AS-IS
 * @note      For more information see  http://users.ece.utexas.edu/~valvano/
 * @date      Feb 13, 2023

 ******************************************************************************/



 
#ifndef __OS_H
#define __OS_H  1
#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/CortexM.h"

/**
 * \brief Times assuming a 80 MHz
 */      
#define TIME_1MS    80000          
#define TIME_2MS    (2*TIME_1MS)  
#define TIME_10MS   (10*TIME_1MS) 
#define TIME_500US  (TIME_1MS/2)  
#define TIME_250US  (TIME_1MS/4)  
#define TIME_100US	8000
#define TIME_10US		800
#define TIME_1US		80
#define TIME_100NS	8

// Thread control stuff
#define MAX_NUM_THREADS 10

// Note: Periodic threads and switch tasks DO have their own stack
//			 and therefore they take away from the total pool of threads (when allocated)
#define MAX_PERIODIC_THREADS 2
#define PERIODIC_TIMER_PRIO 2
#define MAX_SWITCH_TASKS 4
#define MAX_THREAD_PRIORITY 10
#define STACK_SIZE 128
#define MAGIC 0x12312399


typedef struct TCB {
	struct TCB *next_ptr, *prev_ptr; 	// For use in linked lists
	uint8_t priority;
	uint8_t id;
	uint8_t isBackgroundThread;				// Boolean for whether the thread is background (i.e. periodic or switch)
	uint8_t stack_id;									// Remove when memory manager is implemented.
	unsigned long *sp; 								// Stack pointer
	uint32_t sleep_count;							// In ms
	
} TCB_t;


extern TCB_t *RunPt;
extern uint16_t thread_cnt;
extern uint16_t thread_cnt_alive;


/**
 * \brief Semaphore structure. Feel free to change the type of semaphore, there are lots of good solutions
 */  
struct  Sema4{
  int32_t Value;   // >0 means free, otherwise means busy        
	TCB_t *blocked_threads_head;
// add other components here, if necessary to implement blocking
};
typedef struct Sema4 Sema4Type;

typedef struct Mailbox {
	Sema4Type data_ready;
	Sema4Type data_received;
	uint32_t data;
} Mailbox_t;

typedef struct FIFO {
	uint32_t max_size;
	uint32_t *data;
	uint32_t head;
	uint32_t tail;
	
	Sema4Type RxDataAvailable;
	Sema4Type TxRoomLeft;
} FIFO_t;


#define JITTERSIZE 32
#define JITTER_UNITSIZE 5
typedef struct Jitter {
	uint32_t maxJitter;
	uint32_t JitterHistogram[JITTERSIZE];
	uint32_t last_time;
	uint32_t period;
	uint32_t resolution;
	char unit[JITTER_UNITSIZE];
} Jitter_t;


/** OS_get_num_threads
 * @details  Get the total number of allocated threads.
 * This is different than the number of active threads.
 * @param  none
 * @return Total number of allocated threads
 */
uint16_t OS_get_num_threads(void);

/** OS_get_max_jitter
 * @details  Return the max jitter as calculated by the OS
 * @param  none
 * @return Maximum measured jitter
 */
uint32_t OS_Jitter(uint8_t id);
Jitter_t* OS_get_jitter_struct(uint8_t id);
void OS_init_Jitter(uint8_t id, uint32_t period, uint32_t resolution, char unit[]);
void OS_track_ints(uint8_t I);

double OS_get_percent_time_ints_disabled(void);
double OS_get_percent_time_ints_enabled(void);
uint32_t OS_get_time_ints_disabled(void);
uint32_t OS_get_time_ints_enabled(void);
void OS_reset_int_time(void);

/**
 * @details  Initialize operating system, disable interrupts until OS_Launch.
 * Initialize OS controlled I/O: serial, ADC, systick, LaunchPad I/O and timers.
 * Interrupts not yet enabled.
 * @param  none
 * @return none
 * @brief  Initialize OS
 */
void OS_Init(void); 


// ********** OS_MsTime_Init **********
// Initializes the system clock in ms, through timer0A
// Inputs: None
// Outputs: None
void OS_MsTime_Init(void);


// ******** OS_InitSemaphore ************
// initialize semaphore 
// input:  pointer to a semaphore
// output: none
void OS_InitSemaphore(Sema4Type *semaPt, int32_t value); 

// ******** OS_Wait ************
// decrement semaphore 
// Lab2 spinlock
// Lab3 block if less than zero
// input:  pointer to a counting semaphore
// output: none
void OS_Wait(Sema4Type *semaPt); 

// ******** OS_Wait_noblock ************
// input:  pointer to a counting semaphore
// output: 1 if successful, 0 if failure
int OS_Wait_noblock(Sema4Type *semaPt); 

// ******** OS_Signal ************
// increment semaphore 
// Lab2 spinlock
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a counting semaphore
// output: none
void OS_Signal(Sema4Type *semaPt); 

// ******** OS_bWait ************
// Lab2 spinlock, set to 0
// Lab3 block if less than zero
// input:  pointer to a binary semaphore
// output: none
void OS_bWait(Sema4Type *semaPt); 

// ******** OS_bSignal ************
// Lab2 spinlock, set to 1
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a binary semaphore
// output: none
void OS_bSignal(Sema4Type *semaPt); 

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
   uint32_t stackSize, uint32_t priority);

//******** OS_Id *************** 
// returns the thread ID for the currently running thread
// Inputs: none
// Outputs: Thread ID, number greater than zero 
uint32_t OS_Id(void);

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
// In lab 2, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, this command will be called 0 1 or 2 times
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddPeriodicThread(void(*task)(void), 
   uint32_t period, uint32_t priority);

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
int OS_AddSW1Task(void(*task)(void), uint32_t priority);

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
int OS_AddSW2Task(void(*task)(void), uint32_t priority);

// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// You are free to select the time resolution for this function
// OS_Sleep(0) implements cooperative multitasking
void OS_Sleep(uint32_t sleepTime); 

// ******** OS_Kill ************
// kill the currently running thread, release its TCB and stack
// input:  none
// output: none
void OS_Kill(void); 

// ******** OS_Suspend ************
// suspend execution of currently running thread
// scheduler will choose another thread to execute
// Can be used to implement cooperative multitasking 
// Same function as OS_Sleep(0)
// input:  none
// output: none
void OS_Suspend(void);

// temporarily prevent foreground thread switch (but allow background interrupts)
unsigned long OS_LockScheduler(void);
// resume foreground thread switching
void OS_UnLockScheduler(unsigned long previous);
 
// ******** OS_Fifo_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void OS_Fifo_Init(uint32_t size);

// ******** OS_Fifo_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
//  this function can not disable or enable interrupts
int OS_Fifo_Put(uint32_t data);  

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
uint32_t OS_Fifo_Get(void);

// ******** OS_Fifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
int32_t OS_Fifo_Size(void);

// ******** OS_MailBox_Init ************
// Initialize communication channel
// Inputs:  none
// Outputs: none
void OS_MailBox_Init(void);

// ******** OS_MailBox_Send ************
// enter mail into the MailBox
// Inputs:  data to be sent
// Outputs: none
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received 
void OS_MailBox_Send(uint32_t data);

// ******** OS_MailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty 
uint32_t OS_MailBox_Recv(void);

// ******** OS_Time ************
// return the system time 
// Inputs:  none
// Outputs: time in 12.5ns units, 0 to 4294967295
// The time resolution should be less than or equal to 1us, and the precision 32 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_TimeDifference have the same resolution and precision 
uint32_t OS_Time(void);

// ******** OS_TimeDifference ************
// Calculates difference between two times
// Inputs:  two times measured with OS_Time
// Outputs: time difference in 12.5ns units 
// The time resolution should be less than or equal to 1us, and the precision at least 12 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_Time have the same resolution and precision 
uint32_t OS_TimeDifference(uint32_t start, uint32_t stop);

// ******** OS_ClearMsTime ************
// Sets the system time to 0ms
// Inputs:  none
// Outputs: none
void OS_ClearMsTime(void);

// ******** OS_MsTime ************
// reads the current time in msec
// Inputs:  none
// Outputs: time in ms units
// You are free to select the time resolution for this function
// For Labs 2 and beyond, it is ok to make the resolution to match the first call to OS_AddPeriodicThread
uint32_t OS_MsTime(void);

//******** OS_Launch *************** 
// start the scheduler, enable interrupts
// Inputs: number of 12.5ns clock cycles for each time slice
//         you may select the units of this parameter
// Outputs: none (does not return)
// In Lab 2, you can ignore the theTimeSlice field
// In Lab 3, you should implement the user-defined TimeSlice field
// It is ok to limit the range of theTimeSlice to match the 24-bit SysTick
void OS_Launch(uint32_t theTimeSlice);

/**
 * @details open the file for writing, redirect stream I/O (printf) to this file
 * @note if the file exists it will append to the end<br>
 If the file doesn't exist, it will create a new file with the name
 * @param  name file name is an ASCII string up to seven characters
 * @return 0 if successful and 1 on failure (e.g., can't open)
 * @brief  redirect printf output into this file (Lab 4)
 */
int OS_RedirectToFile(const char *name);

/**
 * @details close the file for writing, redirect stream I/O (printf) back to the UART
 * @param  none
 * @return 0 if successful and 1 on failure (e.g., trouble writing)
 * @brief  Stop streaming printf to file (Lab 4)
 */
int OS_EndRedirectToFile(void);

/**
 * @details redirect stream I/O (printf) to the UART0
 * @return 0 if successful and 1 on failure 
 * @brief  redirect printf output to the UART0
 */
int OS_RedirectToUART(void);

/**
 * @details redirect stream I/O (printf) to the ST7735 LCD
 * @return 0 if successful and 1 on failure 
 * @brief  redirect printf output to the ST7735
 */
 int OS_RedirectToST7735(void);

#endif
