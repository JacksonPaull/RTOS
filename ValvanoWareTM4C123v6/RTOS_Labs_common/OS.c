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


extern void ContextSwitch(void);
extern void StartOS(void);
extern void OSThreadReset(void);
void PortFEdge_Init(void);


// Performance Measurements 
int32_t MaxJitter;             // largest time jitter between interrupts in usec
#define JITTERSIZE 64
uint32_t const JitterSize=JITTERSIZE;
uint32_t JitterHistogram[JITTERSIZE]={0,};

// OS Mailbox and FIFIO
# define MAX_FIFO_SIZE 64

Mailbox_t OS_mailbox;
uint32_t OS_FIFO_data[MAX_FIFO_SIZE];
FIFO_t OS_FIFO;


//Note: current max run time will be 2^16 * 1000s ~= 2 years, which seems reasonable
//uint16_t OS_MsTimeResetCount = 0; // Number of times the OS timer has rolled over, allowing for greater run-times
uint32_t OS_MsCount = 0;
uint16_t thread_cnt = 0;

// Thread control structures
TCB_t threads[MAX_NUM_THREADS];
unsigned long stacks[MAX_NUM_THREADS][STACK_SIZE];

TCB_t *RunPt = 0; // Currently running thread
TCB_t *inactive_thread_list_head = 0;
TCB_t *sleeping_thread_list_head = 0;

void DecrementSleepCounters(void) {
	uint32_t systick_period = (STRELOAD+1) / 80000; // Period of systick in ms
	
	// Loop through counters and decrement by period
	TCB_t *node = sleeping_thread_list_head;
	while(node != 0) {
		TCB_t *next_node = node->next_ptr;
		if(node->sleep_count <= systick_period) {
			node->sleep_count = 0; 
			
			// remove node from list and reshedule it
			LL_remove((LL_node_t **)&sleeping_thread_list_head, (LL_node_t *)node);
			scheduler_schedule(node);
		}
		else {
			node->sleep_count -= systick_period;
		}
		node = next_node;
	}
}

/*------------------------------------------------------------------------------
  Systick Interrupt Handler
  SysTick interrupt happens every 10 ms
  used for preemptive thread switch
 *------------------------------------------------------------------------------*/
void SysTick_Handler(void) {
	DecrementSleepCounters();
	ContextSwitch();
}

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
	SYSPRI3 = (SYSPRI3 & ~0xE0000000) | 0x20000000; // Set to priority 1
	STCTRL|= 0x7; 	//Enable Systick (src=clock, interrupts enabled, systick enabled)
}



void thread_init_stack(TCB_t* thread, void(*task)(void), void(*return_task)(void)) {
	int id = thread->id;
	unsigned long* stack = stacks[id];
	
	thread->sp = stack+STACK_SIZE-18*sizeof(unsigned long); // Start at bottom of stack and init registers on stack 
	
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
	thread->sp[11] = 0x03030303; //R3
	thread->sp[12] = 0x12121212; //R12
	thread->sp[13] = (uint32_t) return_task; //LR - Note: any exiting thread should call OS_Kill
	thread->sp[14] = (uint32_t) task; // PC, should point to thread main task
	thread->sp[15] = 0x01000000;	// Set thumb mode in PSR
	thread->sp[16] = MAGIC;
	thread->sp[17] = MAGIC;
}


void OS_thread_init(void) {
	TCB_t* thread = 0;
	TCB_t* prev_thread=0;
	
	for(int i = 0; i < MAX_NUM_THREADS; i++) {
		prev_thread = thread;
		thread = &threads[i];

		LL_append_linear((LL_node_t **) &inactive_thread_list_head, (LL_node_t *)thread);
		
		thread->id=++thread_cnt;
		thread->sleep_count = 0;
		thread->priority = 8;
		
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
	PortFEdge_Init();
	
	DisableInterrupts();
	
	// Init anything else used by OS
	OS_MsTime_Init();
	OS_thread_init();
	
	// Set PendSV priority to 7;
	SYSPRI3 = (SYSPRI3 &0xFF0FFFFF) | 0x00E00000;
	
	//TODO Init UART, any OS controlled ADCs, etc

}; 

// ******** OS_InitSemaphore ************
// initialize semaphore 
// input:  pointer to a semaphore
// output: none
void OS_InitSemaphore(Sema4Type *semaPt, int32_t value){
  // Note: Assumes that a semaphore is initialized once, and this is not called again
	semaPt->Value = value;
}; 

// ******** OS_Wait ************
// decrement semaphore 
// Lab2 spinlock
// Lab3 block if less than zero
// input:  pointer to a counting semaphore
// output: none
void OS_Wait(Sema4Type *semaPt){
  // put Lab 2 (and beyond) solution here
	DisableInterrupts();
	
	semaPt->Value -= 1;
	if(semaPt->Value < 0) {
		// Add to semaphore's blocked list and unschedule
		// This thread will later be rescheduled when OS_Signal is called
		scheduler_unschedule(RunPt);
		LL_append_linear((LL_node_t **) &semaPt->blocked_threads_head, (LL_node_t *)RunPt);
	}
	ContextSwitch(); // Trigger PendSV
	EnableInterrupts();
}; 

// ******** OS_Signal ************
// increment semaphore 
// Lab2 spinlock
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a counting semaphore
// output: none
void OS_Signal(Sema4Type *semaPt){
	int i = StartCritical();
	semaPt->Value += 1;
	
	// If value <= 0, then awaken a blocked thread
	if(semaPt->Value <= 0) {
		TCB_t *thread = (TCB_t *) LL_pop_head_linear((LL_node_t **)&semaPt->blocked_threads_head);
		scheduler_schedule(thread);
	}
	
	EndCritical(i);
}; 

// ******** OS_bWait ************
// Lab2 spinlock, set to 0
// Lab3 block if less than zero
// input:  pointer to a binary semaphore
// output: none
void OS_bWait(Sema4Type *semaPt){
  // TODO Write in ASM with LDREX and STREX?
	DisableInterrupts();
	if(semaPt->Value == 1) {
		semaPt->Value = 0;
		EnableInterrupts();
		return;
	}
	
	// semaPt->Value == 0
	scheduler_unschedule(RunPt);
	LL_append_linear((LL_node_t **)&semaPt->blocked_threads_head, (LL_node_t *) RunPt);
	ContextSwitch();
	EnableInterrupts();
}; 

// ******** OS_bSignal ************
// Lab2 spinlock, set to 1
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a binary semaphore
// output: none
void OS_bSignal(Sema4Type *semaPt){

	TCB_t *thread = (TCB_t *)LL_pop_head_linear((LL_node_t **)&semaPt->blocked_threads_head);
	int i = StartCritical();
	if(thread != 0) {
		scheduler_schedule(thread);
		semaPt->Value = 0;
	}
	else {
		semaPt->Value = 1;
	}
	
	EndCritical(i);
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
	// Take first thread from active list
	TCB_t *thread = (TCB_t *) LL_pop_head_linear((LL_node_t **)&inactive_thread_list_head);
	if(thread == 0) 
			return 0; // Cannot pull anything from list
		 
	// TODO Add a thread_init function here instead of os_thread_init?
	thread_init_stack(thread, task, &OS_Kill);
	scheduler_schedule(thread);
     
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
  // For lab 2 this system works
  
	// TODO (future labs) modify systick
		// counter that goes from 0 to uintmax
		// counter % period == 0 --> trigger task / schedule thread
		// First task is context switch at specified freq
		// all user tasks are just scheduled with high priority
		 

	Timer4A_Init(task, period, priority);	// Task basically just adds another thread that runs once and dies
     
  return 1; // replace this line with solution
};




/*----------------------------------------------------------------------------
  PF1 Interrupt Handler
 *----------------------------------------------------------------------------*/
LL_node_t sw1_threads[MAX_NUM_THREADS]; // Every thread gets its own struct here (conservative)
LL_node_t *sw1_ll_head = 0;

void GPIOPortF_Handler(void){
	GPIO_PORTF_ICR_R = 0x10;      // acknowledge flag4
	
	// TODO Sleep 1ms to debounce?
	
	//Schedule thread(s)
	LL_node_t *node = sw1_ll_head;
	while(node != 0) {
		void (*f)(void) = node->data;
		f();
		node = node->next_ptr;
	}
}

void PortFEdge_Init(void) {
	SYSCTL_RCGCGPIO_R |= 0x00000020; // 1) activate clock for port F
  GPIO_PORTF_LOCK_R = 0x4C4F434B;   // 2) unlock GPIO Port F
  GPIO_PORTF_CR_R = 0x1F;           // allow changes to PF4-0
  GPIO_PORTF_DIR_R |=  0x0E;    // output on PF3,2,1 
  GPIO_PORTF_DIR_R &= ~0x11;    // (c) make PF4,0 in (built-in button)
  GPIO_PORTF_AFSEL_R &= ~0x1F;  //     disable alt funct on PF4,0
  GPIO_PORTF_DEN_R |= 0x1F;     //     enable digital I/O on PF4   
  GPIO_PORTF_PCTL_R &= ~0x000FFFFF; // configure PF4 as GPIO
  GPIO_PORTF_AMSEL_R = 0;       //     disable analog functionality on PF
  GPIO_PORTF_PUR_R |= 0x11;     //     enable weak pull-up on PF4
  GPIO_PORTF_IS_R &= ~0x10;     // (d) PF4 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x10;    //     PF4 is not both edges
  GPIO_PORTF_IEV_R &= ~0x10;    //     PF4 falling edge event
  GPIO_PORTF_ICR_R = 0x10;      // (e) clear flag4
  GPIO_PORTF_IM_R |= 0x10;      // (f) arm interrupt on PF4 *** No IME bit as mentioned in Book ***
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00200000; // (g) priority 1
  NVIC_EN0_R = 0x40000000;      // (h) enable interrupt 30 in NVIC
	
	for(int i = 0; i < MAX_NUM_THREADS; i++) {
		LL_node_t* n = &sw1_threads[i];
		n->next_ptr = 0;
		n->prev_ptr = 0;
		n->data = 0;
	}
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
//	TCB_t *thread = (TCB_t *) LL_pop_head_linear((LL_node_t **) &inactive_thread_list_head);
//	if(thread == 0) 
//			return 0; // Cannot pull anything from list
		 
//	thread->removeAfterScheduling = 1;
//	thread->initial_task = task;
//	thread_init_stack(thread, task, &OSThreadReset);
	static int i = 0;
	if(i >= MAX_NUM_THREADS)
		return 0;
	
	LL_node_t* ll_node = &sw1_threads[i++]; // Get the corresponding node
	ll_node->data = task;
	LL_append_linear(&sw1_ll_head, ll_node);	// Add to portF controller This is where we can implement thread priority for switch tasks

  return 1; // replace this line with solution
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
	TCB_t *thread = RunPt;
	thread->sleep_count = sleepTime;
	
	// Disable Interrupts while we mess with the TCBs
	int i = StartCritical();
	scheduler_unschedule(thread); // Unschedule current thread
	LL_append_linear((LL_node_t **) &sleeping_thread_list_head, (LL_node_t *)thread); // Add to sleeping list
	ContextSwitch();
	EndCritical(i);
};  

// ******** OS_Kill ************
// kill the currently running thread, release its TCB and stack
// input:  none
// output: none
void OS_Kill(void){
	TCB_t *node = RunPt;
	
	// Reset TCB properties
	node->sleep_count = 0;
	node->priority = 8;
	
	// Note: We don't need to mess with the SP 
	//				as it will be automatically reset when a new thread is added
	
	
	DisableInterrupts(); // Don't let another thread take over while this thread is in limbo
	scheduler_unschedule(node);
	LL_append_linear((LL_node_t **) &inactive_thread_list_head, (LL_node_t *)node);
	ContextSwitch();
	EnableInterrupts();
	
  for(;;){};        // can not return, just wait for interrupt
    
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
	ContextSwitch();
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
  OS_FIFO.data = OS_FIFO_data;	// TODO replace with dynamic allocation (amortized doubling too)
	OS_FIFO.head = 0;
	OS_FIFO.tail = 0;
	OS_FIFO.max_size = size+1; // The fifo is essentially null terminated, hence the extra spot
	OS_InitSemaphore(&OS_FIFO.flag, 0);
	OS_InitSemaphore(&OS_FIFO.mutex, 1);
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
	if((OS_FIFO.tail + 1) % OS_FIFO.max_size == OS_FIFO.head) {
		// FIFO is full, return 0
		return 0;
	}
	
	// FIFO has room
	// place at tail, increment tail
	OS_FIFO.data[OS_FIFO.tail] = data;
	OS_FIFO.tail = (OS_FIFO.tail+1) % OS_FIFO.max_size;

	OS_Signal(&OS_FIFO.flag);
  return 1;
};  

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
uint32_t OS_Fifo_Get(void){
  // put Lab 2 (and beyond) solution here
	OS_Wait(&OS_FIFO.flag);	// Wait for data
	OS_bWait(&OS_FIFO.mutex);	// Mutex when reading data from FIFO
	
	// Read from head, increment head
	uint32_t data = OS_FIFO.data[OS_FIFO.head];
	OS_FIFO.head = (OS_FIFO.head+1) % OS_FIFO.max_size;
  
	OS_bSignal(&OS_FIFO.mutex);
  return data;
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
// Inputs:  None
// Outputs: None
void OS_MailBox_Init(void){
	OS_InitSemaphore(&OS_mailbox.data_ready, 0); //Init flag to 1="ready to read"
	OS_InitSemaphore(&OS_mailbox.data_received, 1); //Init flag to 1="ready to send"
};

// ******** OS_MailBox_Send ************
// enter mail into the MailBox
// Inputs:  data to be sent
// Outputs: none
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received 
void OS_MailBox_Send(uint32_t data){
  // Set flag down and place data
	OS_bWait(&OS_mailbox.data_received);
	OS_mailbox.data = data;
	OS_bSignal(&OS_mailbox.data_ready);
	
	// Note: if two threads call this, the second thread will 
	// spin on bWait until the mail is receieved
};

// ******** OS_MailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty 
uint32_t OS_MailBox_Recv(void){
	OS_bWait(&OS_mailbox.data_ready);
  uint32_t data = OS_mailbox.data;
	OS_bSignal(&OS_mailbox.data_received);
	return data;
};

// ******** OS_Time ************
// return the system time 
// Inputs:  none
// Outputs: time in 12.5ns units, 0 to 4294967295 = 2**32-1
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
	
	scheduler_init(&RunPt);
	StartOS(); // Never returns
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

