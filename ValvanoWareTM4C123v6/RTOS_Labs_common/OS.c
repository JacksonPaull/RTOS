// *************os.c**************
// EE445M/EE380L.6 Labs 1, 2, 3, and 4 
// High-level OS functions
// Students will implement these functions as part of Lab
// Runs on LM4F120/TM4C123
// Jonathan W. Valvano 
// Jan 12, 2020, valvano@mail.utexas.edu


#include <stdio.h>
#include <stdlib.h>

#include "../inc/PLL.h"
#include "../inc/LaunchPad.h"
#include "../inc/Timer3A.h"
#include "../inc/Timer4A.h"
#include "../inc/Timer5A.h"
#include "../inc/WTimer0A.h"
#include "../RTOS_Labs_common/OS.h"
#include "../RTOS_Labs_common/ST7735.h"
#include "../RTOS_Labs_common/eDisk.h"
#include "../RTOS_Labs_common/eFile.h"
#include "../RTOS_Labs_common/heap.h"
#include "../RTOS_Lab4_FileSystem/iNode.h"

#include "../inc/ADCT0ATrigger.h"
#include "../RTOS_Labs_common/UART0int.h"

#include "../RTOS_Lab2_RTOSkernel/LinkedList.h"
#include "../RTOS_Lab2_RTOSkernel/scheduler.h"



extern void ContextSwitch(void);
extern void StartOS(void);
extern void OSThreadReset(void);
void PortFEdge_Init(void);
extern void SVC_ContextSwitch(void);

// For use with OS_time and related functions
#define TRIGGERS_TO_MS 53687
#define TRIGGERS_TO_S 53
#define TRIGGERS_TO_US 53687091

#define BUS_TO_MS 80000
#define BUS_TO_US 80
#define BUS_TO_S 80000000

uint32_t OS_timer_triggers = 0;

// Performance Measurements 
#define MAX_JITTER_TRACKERS 2
Jitter_t Jitters[MAX_JITTER_TRACKERS];
uint32_t os_int_time_enabled = 0;				// In 1us units
uint32_t os_int_time_disabled = 0;			// In 1us units

// OS Mailbox and FIFIO
# define MAX_FIFO_SIZE 64

Mailbox_t OS_mailbox;
uint32_t OS_FIFO_data[MAX_FIFO_SIZE];
FIFO_t OS_FIFO;


uint16_t thread_cnt = 0;
uint16_t thread_cnt_alive = 0;
uint16_t num_processes = 0;
uint16_t num_processes_alive = 0;

// Thread control structures
// TODO Remove both of these, move to dynamic allocation
TCB_t threads[MAX_NUM_THREADS];

TCB_t *RunPt = 0; // Currently running thread
TCB_t *inactive_thread_list_head = 0;
TCB_t *sleeping_thread_list_head = 0;



/** OS_get_num_threads
 * @details  Get the total number of allocated threads.
 * This is different than the number of active threads.
 * @param  none
 * @return Total number of allocated threads
 */
uint16_t OS_get_num_threads(void) {
	return thread_cnt_alive;
}

void OS_init_Jitter(uint8_t id, uint32_t period, uint32_t resolution, char unit[]) {
	Jitters[id].maxJitter = 0;
	Jitters[id].last_time = 0;
	Jitters[id].period = period;
	Jitters[id].resolution = resolution;
	for(int i = 0; i < JITTER_UNITSIZE; i++) {
		Jitters[id].unit[i] = unit[i];
	}
	
	for(int i = 0; i < JITTERSIZE; i++) {
		Jitters[id].JitterHistogram[i] = 0;
	}
}

uint32_t OS_Jitter(uint8_t id) {	
	uint32_t jitter;
	Jitter_t* J = &Jitters[id];
	uint32_t time = OS_Time();
	
	// Ignore first jitter (system setup)
	if(J->last_time == 0) {
		J->last_time = time;
		return 0;
	}

	int diff = OS_TimeDifference(J->last_time, time);
	J->last_time = time;
	
	if(diff < J->period) {
		jitter = (J->period - diff + 4)/J->resolution; //in 0.1us
	}
	else {
		jitter = (diff - J->period + 4)/J->resolution;
	}
	
	if(jitter > J->maxJitter) {
		J->maxJitter = jitter;
	}
	
	if(jitter < JITTERSIZE) {
		J->JitterHistogram[jitter]++;
	}
//	else {
//		printf("Extreme jitter recorded! (%d)", jitter);
//	}
	
	J->last_time = time;
	return jitter;
}


Jitter_t* OS_get_jitter_struct(uint8_t id) {
	return &Jitters[id];
}


double OS_get_percent_time_ints_disabled(void) {
	return (double)(os_int_time_disabled)/(os_int_time_disabled + os_int_time_enabled);
}

double OS_get_percent_time_ints_enabled(void) {
	return (double)(os_int_time_enabled)/(os_int_time_disabled + os_int_time_enabled);
}

uint32_t OS_get_time_ints_disabled(void) {
	return os_int_time_disabled;
}

uint32_t OS_get_time_ints_enabled(void) {
	return os_int_time_enabled;
}

void OS_reset_int_time(void) {
	os_int_time_enabled = 0;
	os_int_time_disabled = 0;
}


// Track the time 
void OS_track_ints(uint32_t I) {
	static uint32_t last_time = 0;
	static uint32_t last_I = 0;
	
	uint32_t time = OS_Time();
	
	uint32_t time_diff = (time - last_time) / TIME_1US; // In units of microseconds
	if(last_I == 0) { // Interrupts were enabled, mark the time they were enabled for
		os_int_time_enabled += time_diff;
	}
	else {
		os_int_time_disabled += time_diff;
	}
	
	last_I = I;
	last_time = time;
};


TCB_t* OS_get_current_TCB(void) {
	return RunPt;
}

/** DecrementSleepCounters
 * @details Decrease the timer on sleeping threads at every invocation of systick
 * Dynamically calculates the systick period based on the reload value
 * @param  none
 * @return none
 * @brief Handle sleeping threads
 */
void DecrementSleepCounters(void) {
	const uint32_t period = 1;
	
	// Loop through counters and decrement by period
	int i = StartCritical();
	TCB_t *node = sleeping_thread_list_head;
	while(node != 0) {
		TCB_t *next_node = node->next_ptr;
		if(node->sleep_count <= period) {
			node->sleep_count = 0; 
			
			// remove node from list and reshedule it
			LL_remove((LL_node_t **)&sleeping_thread_list_head, (LL_node_t *)node);
			scheduler_schedule(node);
		}
		else {
			node->sleep_count -= period;
		}
		node = next_node;
	}
	EndCritical(i);
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

void BackgroundThreadExit(void) {
	scheduler_unlock(); // Force unlock
	thread_cnt_alive--;
	#if EFILE_H
	iNode_close(RunPt->currentDir);
	#endif
	ContextSwitch();
	EnableInterrupts(); // Force interrupt enable
}

unsigned long OS_LockScheduler(void){
  // lab 4 might need this for disk formating
  scheduler_lock();
	return 0;
}
void OS_UnLockScheduler(unsigned long previous){
 scheduler_unlock();
}


/** SysTick_init
 * @details Sets up the systick to trigger repeatedly, once per period
 * This is used for the frequency at which preemption occurs.
 * @param  unsigned long period (in 12.5ns units)
 * @return none
 * @brief Initialize the SysTick timer
 */
void SysTick_Init(unsigned long period){
	STCTRL &= ~0x1; //Disable systick during setup
  STRELOAD = period-1;
	STCURRENT = 0;
	SYSPRI3 = (SYSPRI3 & ~0xE0000000) | 0x20000000; // Set to priority 1
	STCTRL|= 0x7; 	//Enable Systick (src=clock, interrupts enabled, systick enabled)
}


/** thread_init_stack
 * @details Initialize the stack for a new thread. Initializes LR to point to return task, 
 * and PC to point to the birth task. The PSR is set with thumb mode active, but the PSR control bits are all 0
 * Registers are initialized to a (fixed) garbage value.
 * A magic value is placed on the bottom of a stack to identify stack corruption
 * @param thread control block pointer
 * @param pointer to task the thread will execute after birth
 * @param pointer to task the thread will execute upon death (usually OS_Kill)
 * @return none
 * @brief Set up thread stack before launching
 */
void thread_init_stack(TCB_t* thread, void(*task)(void), void(*return_task)(void), uint32_t stack_size) {	
	thread->sp = ((void *)thread->stack_base + stack_size - 16*sizeof(unsigned long)); // Start at bottom of stack and init registers on stack 
	thread->stack_base[0] = MAGIC;
	
	thread->sp[0]  = 0x04040404; //R4
	thread->sp[1]  = 0x05050505; //R5
	thread->sp[2]  = 0x06060606; //R6
	thread->sp[3]  = 0x07070707; //R7
	thread->sp[4]  = 0x08080808; //R8
	if(thread->process) {
		thread->sp[5]  = (unsigned long) thread->process->data; //R9
	}
	else {
		thread->sp[5]  = 0x09090909; //R9
	}
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
}


/** OS_thread_init
 * @details Preallocate TCBs and add new threads to the inactive pool
 * @param none
 * @return none
 * @brief Set up pool of pre-allocated threads
 */
void OS_thread_init(void) {
	TCB_t* thread = 0;
	TCB_t* prev_thread=0;
	
	for(int i = 0; i < MAX_NUM_THREADS; i++) {
		prev_thread = thread;
		thread = &threads[i];

		LL_append_linear((LL_node_t **) &inactive_thread_list_head, (LL_node_t *)thread);

		thread->sleep_count = 0;
		thread->priority = 8;
		thread->stack_base = 0;
		thread->next_ptr = 0;
		thread->prev_ptr = 0;
		
		// TODO init anything else from the thread
		// Note: Stacks are initialized when making the thread
			// This also ensures that any program which exits without 
			// first clearing the stack won't mess up any new threads
	}
}

void fs_init_task(void) {
	eDisk_Init(0);
	eFile_Init();
	eFile_Mount();
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
	
	// Init anything else used by OS
	Heap_Init();
	OS_MsTime_Init();
	PortFEdge_Init();
	Timer5A_Init(&DecrementSleepCounters, TIME_1MS, 1);
	UART_Init();
	DisableInterrupts();	// Disable after the OS clock is init so that we can track time ints disabled
	OS_thread_init();
	ST7735_InitR(INITR_REDTAB);

	// Set PendSV priority to 7;
	SYSPRI3 = (SYSPRI3 &0xFF0FFFFF) | 0x00E00000;
	
	// Set SVC priority to 6
	SYSPRI2 = (SYSPRI2 &0x0FFFFFFF) | 0xC0000000;
	/* Note: Setting SVC to priority 6 means that PendSV is guaranteed non-reentrant and will always return to a thread context when context switching
		 a BX LR while in handler mode without 0xFFFFFFE9 as the LR would not properly pop the stack.
		 In practice this means that threads will be non-preemtive while executing OS routines */
	
	#if USEFILESYS
	OS_AddPeriodicThread(&disk_timerproc, TIME_1MS, 0);
	OS_AddThread(&fs_init_task, 512, 0);	// Note: OS_Init should always be called before adding any threads so this should always execute first. Adding threads to a non-initialized OS is undefined behavior
	#endif
	
	//TODO  any OS controlled ADCs, etc

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
	DisableInterrupts();
	
	semaPt->Value -= 1;
	if(semaPt->Value < 0) {
		// Add to semaphore's blocked list and unschedule
		// This thread will later be rescheduled when OS_Signal is called
		TCB_t *thread = RunPt;
		scheduler_unschedule(thread);
		PrioQ_insert((PrioQ_node_t **) &semaPt->blocked_threads_head, (PrioQ_node_t *)thread);
		SVC_ContextSwitch(); // Trigger PendSV
	}
	
	EnableInterrupts();
}; 

// ******** OS_Wait_noblock ************
// input:  pointer to a counting semaphore
// output: 1 if successful, 0 if failure
int OS_Wait_noblock(Sema4Type *semaPt){
	int i = StartCritical();
	
	if(semaPt->Value <= 0) {
		EndCritical(i);
		return 0;
	}
	semaPt->Value--;
	EndCritical(i);
	return 1;
}; 

// ******** OS_Signal ************
// increment semaphore 
// Lab2 spinlock
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a counting semaphore
// output: none
void OS_Signal(Sema4Type *semaPt){
	int i = StartCritical();
	// If value <= 0, then awaken a blocked thread
	semaPt->Value++;
	if(semaPt->Value <= 0) {
		TCB_t *thread = (TCB_t *) PrioQ_pop((PrioQ_node_t **)&semaPt->blocked_threads_head);
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

	scheduler_unschedule(RunPt);
	PrioQ_insert((PrioQ_node_t **)&semaPt->blocked_threads_head, (PrioQ_node_t *) RunPt);
	ContextSwitch();
	EnableInterrupts();
}; 

// ******** OS_bSignal ************
// input:  pointer to a binary semaphore
// output: none
void OS_bSignal(Sema4Type *semaPt){
	int i = StartCritical();
	TCB_t *thread = (TCB_t *)PrioQ_pop((PrioQ_node_t **)&semaPt->blocked_threads_head);
	if(thread != 0) {
		scheduler_schedule(thread);
		semaPt->Value = 0;
	}
	else {
		semaPt->Value = 1;
	}
	
	EndCritical(i);
}; 

// TODO Update from popping from pool and turn into malloc
TCB_t* SpawnThread(uint8_t isBackgroundThread, uint8_t priority, uint32_t stack_size) {
	
	
	int i = StartCritical();
	void* stack_base = malloc(stack_size);
	if(!stack_base) {
		EndCritical(i);
		return 0;
	}
	TCB_t *thread = (TCB_t *) LL_pop_head_linear((LL_node_t **)&inactive_thread_list_head);
	
	if(thread == 0) {
		EndCritical(i);
		return 0; // Cannot pull anything from list
	}
	
	// Init all TCB attributes
	thread->id = ++thread_cnt;
	thread->stack_base = stack_base;
	thread->isBackgroundThread = isBackgroundThread;
	thread->sleep_count = 0;
	thread->priority = priority;
	thread->currentDir = 0;
	thread->process = 0;
	
	// Inherit the RunPt process if possible. Defaults to 0 (base OS process)
	if(RunPt) {
		thread->process = RunPt->process;
	}
	
	EndCritical(i);
	return thread;
}



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
		 if(stackSize < 512) {
			 stackSize = 512;
		 }
		 
	// Allocate a foreground thread
	TCB_t *thread = SpawnThread(0, priority, stackSize);
	if(thread == 0) {
		return 0;
	}
	
	int I = StartCritical();
	thread_init_stack(thread, task, &OS_Kill, stackSize);
	scheduler_schedule(thread);
  EndCritical(I);
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
	if(stackSize < 512) {
			 stackSize = 512;
		 }
	
	// Create new PCB
	PCB_t *PCB = malloc(sizeof(PCB_t));
	if(!PCB) {
		return 0;
	}
		
	PCB->text = text;
	PCB->data = data;
	PCB->id = ++num_processes;
	++num_processes_alive;
	PCB->heap_size = PROCESS_HEAP_SIZE;
	PCB->heap = malloc(PROCESS_HEAP_SIZE);
	PCB->parent = RunPt->process; // Will be 0 for the base OS process, and nonzero for any user added process.
	if(!PCB->heap) {
		free(PCB);
		return 0;
	}
	
	/*
		Doing the above nesting allows for user proceses to create other processes without inflating the total memory allocation they are given.
		Any process must maintain its total allocation on disk including any additional threads and other processes it chooses to create.
		The OS is the base process and therefore has access to the entire heap space.
	*/
		
	int I = StartCritical();
	
	
	// Do stuff in the context of the new proccess heap
	RunPt->process = PCB;
	
	// Allocate a foreground thread and link to the PCB
	Heap_Init(); // initialize the new heap
	TCB_t *thread = SpawnThread(0, priority, stackSize);
	
	// Return the heap context to the parent process
	RunPt->process = PCB->parent;
	
	if(thread == 0) {
		free(PCB->heap);
		free(PCB);
		EndCritical(I);
		return 0;
	}
	
	thread->process = PCB;
	PCB->numThreadsAlive++;
	thread_init_stack(thread, entry, &OS_Kill, stackSize);
	scheduler_schedule(thread);
  EndCritical(I);
     
  return 1; // replace this line with Lab 5 solution
}


//******** OS_Id *************** 
// returns the thread ID for the currently running thread
// Inputs: none
// Outputs: Thread ID, number greater than zero 
uint32_t OS_Id(void){
  return RunPt->id;
};


// 25 bytes long - not very expensive
typedef struct Periodic_TCB {
	struct Periodic_TCB *next, *prev;
	uint8_t priority;
	uint32_t period; // in bus cycles
	uint32_t cnt;
	TCB_t *TCB;
	void (*task)(void);
} Periodic_TCB_t;

// TODO Update to be malloc'd and free'd (Can use linked list)
uint8_t NumPeriodicThreads = 0;
Periodic_TCB_t Periodic_Threads[MAX_PERIODIC_THREADS];

void PeriodicThreadHandler() {
	DisableInterrupts();
	uint32_t min_cnt = 0xFFFFFFFF;
	// Check all periodic tasks and launch them as needed
	for( int i = 0; i < NumPeriodicThreads; i++) {
		Periodic_TCB_t *node = &Periodic_Threads[i];
		
		// counter -= timer period
		node->cnt -= TIMER4_TAILR_R+1; 
		
		if(node->cnt == 0) {
			node->cnt = node->period;
			
			// Schedule thread (need to init the stack each time)
			thread_init_stack(node->TCB, node->task, &BackgroundThreadExit, BACKGROUND_STACK_SIZE);
			scheduler_schedule(node->TCB);
			ContextSwitch();	// Switch to scheduled task asap
			
			//node->task();
		}
		
		if(node->cnt < min_cnt) {
			min_cnt = node->cnt;
		}
	}
	
	// Reset timer based on min cnt value
	Timer4A_RestartOneShot(min_cnt);
	EnableInterrupts();
}


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

// TODO Add stack size parameter? Note: Updating this requires updating thread_init_stack above
int OS_AddPeriodicThread(void(*task)(void), 
   uint32_t period, uint32_t priority){
		 
  int i = StartCritical();
	TCB_t *thread = SpawnThread(1, priority, BACKGROUND_STACK_SIZE);
	if(thread == 0) {
		// Can't allocate thread / stack
		EndCritical(i);
		return 0;
	}
	

	Periodic_TCB_t *t = &Periodic_Threads[NumPeriodicThreads++];
	t->period = period;
	t->TCB = thread;
	t->task = task;
	t->cnt = period;
	
	if(RunPt->process) {
		t->TCB->process = RunPt->process;
		t->TCB->process->numThreadsAlive++; // Note: adding a periodic thread to a process means it will never die. This makes sense when its considered that periodic tasks return and are scheduled again
	}

	if(NumPeriodicThreads == 1) {
		// Set the timer to start running on first added thread
		Timer4A_InitOneShot(&PeriodicThreadHandler, period, PERIODIC_TIMER_PRIO);
	}
	EndCritical(i);
  return 1; // replace this line with solution
};




/*----------------------------------------------------------------------------
  PF1 Interrupt Handler
 *----------------------------------------------------------------------------*/

// TODO update to malloc and free
// 8 bytes long - not very expensive
typedef struct SW_Task {
	TCB_t *TCB;
	void (*task)(void);
	// ack flag (x10, x01, or 0x11) -- allows you to register to either switch or both and uses same pool
} SW_Task_t;

SW_Task_t sw1_tasks[MAX_SWITCH_TASKS];
SW_Task_t sw2_tasks[MAX_SWITCH_TASKS];
uint8_t num_sw1_tasks = 0;
uint8_t num_sw2_tasks = 0;


//******** GPIOPortF_Handler *************** 
// Schedule all thread tasks to run when a PortF interrupt is triggered
// Inputs: none
// Outputs: none
void GPIOPortF_Handler(void){
	
	// If switch 1 pressed
	if(GPIO_PORTF_RIS_R & 0x10) {
		// Schedule all switch 1 tasks
		DisableInterrupts();
		for(int i = 0; i < num_sw1_tasks; i++) {
			SW_Task_t *sw = &sw1_tasks[i];
			thread_init_stack(sw->TCB, sw->task, &BackgroundThreadExit, BACKGROUND_STACK_SIZE);
			scheduler_schedule(sw->TCB);
			ContextSwitch();
			
			//sw->task();
		}		
		GPIO_PORTF_ICR_R |= 0x10;
		EnableInterrupts();
	}
	
	// If switch 2 pressed
	if(GPIO_PORTF_RIS_R & 0x01) {
		// Schedule all switch 2 tasks
		DisableInterrupts();
		for(int i = 0; i < num_sw2_tasks; i++) {
			SW_Task_t *sw = &sw2_tasks[i];
			thread_init_stack(sw->TCB, sw->task, &BackgroundThreadExit, BACKGROUND_STACK_SIZE);
			scheduler_schedule(sw->TCB);
			ContextSwitch();
			
			//sw->task();
		}		
		GPIO_PORTF_ICR_R |= 0x01;
		EnableInterrupts();
	}
	
}

//******** PortFEdge_Init *************** 
// Set up PortF to trigger an interrupt on falling edge of Sw1
// Inputs: none
// Outputs: none
void PortFEdge_Init(void) {
	SYSCTL_RCGCGPIO_R |= 0x00000020; // 1) activate clock for port F
  GPIO_PORTF_LOCK_R = 0x4C4F434B;   // 2) unlock GPIO Port F
  GPIO_PORTF_CR_R = 0x1F;           // allow changes to PF4-0
  GPIO_PORTF_DIR_R |=  0x0E;    // output on PF3,2,1 
  GPIO_PORTF_DIR_R &= ~0x11;    // (c) make PF4,0 in (built-in button)
  GPIO_PORTF_AFSEL_R &= ~0x11;  //     disable alt funct on PF4,0
  GPIO_PORTF_DEN_R |= 0x1F;     //     enable digital I/O on PF4,3,2,1,0  
  GPIO_PORTF_PCTL_R &= ~0x000FFFFF; // configure PF4,3,2,1,0 as GPIO
  GPIO_PORTF_AMSEL_R = 0;       //     disable analog functionality on PF
  GPIO_PORTF_PUR_R |= 0x11;     //     enable weak pull-up on PF4
  GPIO_PORTF_IS_R &= ~0x11;     // (d) PF4/0 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x11;    //     PF4/0 is not both edges
  GPIO_PORTF_IEV_R &= ~0x11;    //     PF4/0 falling edge event
  GPIO_PORTF_ICR_R = 0x11;      // (e) clear flag4/0
  GPIO_PORTF_IM_R |= 0x11;      // (f) arm interrupt on PF4,0 *** No IME bit as mentioned in Book ***
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00200000; // (g) priority 1
  NVIC_EN0_R = 0x40000000;      // (h) enable interrupt 30 in NVIC
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
	TCB_t *thread = SpawnThread(1, priority, BACKGROUND_STACK_SIZE);
	if(thread == 0) {
		return 0; // Can't allocate a thread
	}
	
	SW_Task_t *sw = &sw1_tasks[num_sw1_tasks];
	sw->task = task;
	sw->TCB = thread;
	
	if(RunPt->process) {
		sw->TCB->process = RunPt->process;
		sw->TCB->process->numThreadsAlive++; // Note: adding a background thread to a process means it will never die.
	}

	num_sw1_tasks++;
  return 1;
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
  TCB_t *thread = SpawnThread(1, priority, BACKGROUND_STACK_SIZE);
	if(thread == 0) {
		return 0; // Can't allocate a thread
	}
	
	SW_Task_t *sw = &sw2_tasks[num_sw2_tasks];
	sw->task = task;
	sw->TCB = thread;

	if(RunPt->process) {
		sw->TCB->process = RunPt->process;
		sw->TCB->process->numThreadsAlive++; // Note: adding a background thread to a process means it will never die.
	}
	
	num_sw2_tasks++;
  return 1;
};


// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// You are free to select the time resolution for this function
// OS_Sleep(0) implements cooperative multitasking
void OS_Sleep(uint32_t sleepTime){ 
	DisableInterrupts(); // Disable Interrupts while we mess with the TCBs
	TCB_t *thread = RunPt;
	thread->sleep_count = sleepTime;
	
	scheduler_unschedule(thread); // Unschedule current thread
	LL_append_linear((LL_node_t **) &sleeping_thread_list_head, (LL_node_t *)thread); // Add to sleeping list
	ContextSwitch();
	EnableInterrupts();
};  

// ******** OS_Kill ************
// kill the currently running thread, release its TCB and stack
// input:  none
// output: none
void OS_Kill(void){
	
	DisableInterrupts();
	ContextSwitch();
	
	TCB_t *node = RunPt;
	PCB_t *proc = node->process;
	scheduler_unschedule(node);
	thread_cnt_alive--;

	
	// Reset TCB properties
	free(node->stack_base);
	node->sleep_count = 0;
	node->isBackgroundThread = 0;
	#if EFILE_H
	iNode_close(RunPt->currentDir);
	#endif
	
	// Note: We don't need to mess with the SP 
	//				as it will be automatically reset when a new thread is added
	
	// Adjust PCB // TODO Fix this
	if(proc) {
		// Node was a part of a user-loaded process
		if(--proc->numThreadsAlive == 0) {
			// Need to free the process resources from the base heap (or other parent's heap)
			RunPt->process = proc->parent; // Since we are killing the thread anyways this is fine
			free(proc->data);
			free(proc->text);
			free(proc->heap);
			free(proc);
			
			num_processes_alive--;
		}
	}
	
	
	
	LL_append_linear((LL_node_t **) &inactive_thread_list_head, (LL_node_t *)node);
	
	EnableInterrupts();  
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
	OS_FIFO.max_size = size;
	OS_FIFO.head = 0;
	OS_FIFO.tail = 0;
	OS_InitSemaphore(&OS_FIFO.RxDataAvailable, 0);
	OS_InitSemaphore(&OS_FIFO.TxRoomLeft, size);
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
	if(!OS_Wait_noblock(&OS_FIFO.TxRoomLeft)) {
		return 0;
	}
	
	// place at tail, increment tail
	OS_FIFO.data[OS_FIFO.tail] = data;
	OS_FIFO.tail = (OS_FIFO.tail+1) % OS_FIFO.max_size;

	OS_Signal(&OS_FIFO.RxDataAvailable);
  return 1;
};  

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
uint32_t OS_Fifo_Get(void){
  // put Lab 2 (and beyond) solution here
	OS_Wait(&OS_FIFO.RxDataAvailable);	// Wait for data
	
	// Read from head, increment head
	uint32_t data = OS_FIFO.data[OS_FIFO.head];
	OS_FIFO.head = (OS_FIFO.head+1) % OS_FIFO.max_size;
  
	OS_Signal(&OS_FIFO.TxRoomLeft);
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
  return OS_FIFO.RxDataAvailable.Value;
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
uint32_t OS_Time(void){
  return ~TIMER3_TAV_R;
};

// ******** OS_TimeDifference ************
// Calculates difference between two times
// Inputs:  two times measured with OS_Time
// Outputs: time difference in 12.5ns units 
uint32_t OS_TimeDifference(uint32_t start, uint32_t stop){

	return stop - start; 

};
	
uint32_t OS_ms_reset_time = 0;
// Triggers once every 53.687 seconds
void MsTime_Helper(void) {
	OS_timer_triggers++;
	OS_ms_reset_time = 0;
}


// ********** OS_MsTime_Init **********
// Initializes the system clock in ms, through timer3A
// Inputs: None
// Outputs: None
void OS_MsTime_Init(void) {
	Timer3A_Init(&MsTime_Helper, 0, 0);
}

// ******** OS_ClearMsTime ************
// Sets the system time to 0ms
// Inputs:  none
// Outputs: none
void OS_ClearMsTime(void){
	// Reset timer and counter to their initial config
  OS_timer_triggers = 0;
	OS_ms_reset_time = OS_Time();
};

// ******** OS_MsTime ************
// reads the current time in msec
// Inputs:  none
// Outputs: time in ms units
uint32_t OS_MsTime(void){
	// Note: We can take the logical not of the timer as this gives us the time elapsed in bus cycles
	uint32_t clk_time = OS_TimeDifference(OS_ms_reset_time, ~TIMER3_TAV_R);
	return OS_timer_triggers * TRIGGERS_TO_MS + clk_time / BUS_TO_MS;
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


#if EFILE_H
int StreamToDevice=0;                // 0=UART, 1=stream to file (Lab 4)
File_t file;

int fputc (int ch, FILE *f) { 
  if(StreamToDevice==1){  // Lab 4
    if(eFile_F_write(&file, &ch, 1)){          // close file on error
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

int putc (int ch, FILE *f) { 
  if(StreamToDevice==1){  // Lab 4
    if(eFile_F_write(&file, &ch, 1)){          // close file on error
       OS_EndRedirectToFile(); // cannot write to file
       return 1;                  // failure
    }
    return 0; // success writing
  }
  
  // default UART output
  UART_OutChar(ch);
  return ch; 
}

int getc (FILE *f){
  char ch = UART_InChar();  // receive from keyboard
  UART_OutChar(ch);         // echo
  return ch;
}

int OS_RedirectToFile(const char *name){  // Lab 4
  eFile_Create(name);              // ignore error if file already exists
  if(eFile_Open(name, &file)) return 1;  // cannot open file
  StreamToDevice = 1;
  return 0;
}

int OS_EndRedirectToFile(void){  // Lab 4
  StreamToDevice = 0;
  if(eFile_F_close(&file)) return 1;    // cannot close file
  return 0;
}

int OS_RedirectToUART(void){
  StreamToDevice = 0;
  return 0;
}

int OS_RedirectToST7735(void){
  
  return 1;
}

#else

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

int putc (int ch, FILE *f) { 
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

int getc (FILE *f){
  char ch = UART_InChar();  // receive from keyboard
  UART_OutChar(ch);         // echo
  return ch;
}

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

#endif

