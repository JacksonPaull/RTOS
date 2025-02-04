/*
	contains implementations for a variety of different scheduler types
*/

#include "./scheduler.h"


/* round_robin_scheduler
Simple round robin scheduler which just goes in order around the list

Inputs: 
	TCB_t *run_pt: pointer to currently running thread.
								 Assumes this list is a proper doubly linked list
Outputs: pointer to next thread that should be run
*/

TCB_t *scheduled_pt;

void scheduler_init(TCB_t *first_thread) {
		scheduled_pt = first_thread->prev_ptr;
}

TCB_t* round_robin_scheduler(void) {
	scheduled_pt = scheduled_pt->next_ptr;
	return scheduled_pt;
}


