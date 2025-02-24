/*
	contains implementations for a variety of different scheduler types
*/

#include "./scheduler.h"


TCB_t INIT_TCB;		// TCB Used upon first entry into the scheduler

// A separate linked list for each priority level
// +1 for inclusive max_priority (i.e. max_prio = 8 -> thread.priority = 8 is valid)
// +2 for a list for background threads (priority queue)
TCB_t *Priority_Levels[MAX_THREAD_PRIORITY+2];
volatile uint8_t locked = 0;
Sema4Type sch_mutex;

// TODO Add Mutex for scheduler 

void scheduler_lock(void) {
	locked = 1;
}

void scheduler_unlock(void) {
	locked = 0;
}


void scheduler_init(TCB_t **RunPt) {
	// Back up one spot so that on first context switch,
	// the first thread to be scheduled runs (all prio levels except background threads)
	for(int i = 0; i < MAX_THREAD_PRIORITY; i++) {
		TCB_t *t = Priority_Levels[i];
		if(t != 0 && i!=0) {
			Priority_Levels[i] = t->prev_ptr;
		}
	}
	
	// Set initial RunPt to be OS backup thread
	*RunPt = &INIT_TCB;
}

void scheduler_unschedule(TCB_t *thread) {
	// Find appropriate list and remove
	TCB_t **head = &Priority_Levels[thread->priority+1];
	LL_remove((LL_node_t **) head, (LL_node_t *) thread);
}

void scheduler_schedule(TCB_t *thread) {
	// Find appropriate list and insert
	if(thread->isBackgroundThread) {
		PrioQ_insert((PrioQ_node_t **) &Priority_Levels[0], (PrioQ_node_t *) thread);
	}
	else {
		TCB_t **head = &Priority_Levels[thread->priority+1];
		LL_append_circular((LL_node_t **) head, (LL_node_t *) thread);
	}
}

// Schedule something to run next, instead of last in line.
// This can eventually be removed when a priority scheduler is introduced
void scheduler_schedule_immediate(TCB_t *thread) {
	// Find appropriate list and insert
	if(thread->isBackgroundThread) {
		PrioQ_insert((PrioQ_node_t **) &Priority_Levels[0], (PrioQ_node_t *) thread);
	}
	else {
		TCB_t **head = &Priority_Levels[thread->priority+1];
		LL_insert_circular((LL_node_t **) head, (LL_node_t *) thread);
	}
}




/* scheduler_update_priority
 * Updates scheduler lists with changes to thread priority
 */
void scheduler_update_priority(TCB_t *thread, uint8_t new_priority) {
	// TODO
}

/* scheduler_next
Simple round robin scheduler which just goes in order around the list

Inputs: None
Outputs: pointer to next thread that should be run
*/
TCB_t* scheduler_next(void) {
	TCB_t *thread_to_schedule;
	// Note: This is where priority elevation can occur
	
	// If locked (by a background thread) no preemption occurs
	if(locked == 1){
		return RunPt;
	}
	
	TCB_t *head = 0;
	uint8_t i = 0;
	// Find first LL with a non-null pointer
	while((head == 0) && (i < MAX_THREAD_PRIORITY)) {
		head = Priority_Levels[i];
		i++;
	}
	i--;
	
	if(head == 0) { // Nothing is scheduled, use the base OS program
		return &INIT_TCB;
	}
	
	// For background threads only
	if(i == 0) {
		locked = 1;
		thread_to_schedule = (TCB_t *)PrioQ_pop((PrioQ_node_t **) &Priority_Levels[0]);
		return thread_to_schedule;
	}
	
	// Execute round robin scheduling on that list
	thread_to_schedule = head->next_ptr;
	Priority_Levels[i] = thread_to_schedule;
	return thread_to_schedule;
}


