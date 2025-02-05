/*
	contains implementations for a variety of different scheduler types
*/

#include "./scheduler.h"


/* round_robin_scheduler
Simple round robin scheduler which just goes in order around the list

Inputs: None
Outputs: pointer to next thread that should be run
*/

TCB_t INIT_TCB;		// TCB Used upon first entry into the scheduler
TCB_t *scheduled_thread = 0;

void scheduler_init(TCB_t *first_thread, TCB_t **RunPt) {
	INIT_TCB.next_ptr = first_thread;
	*RunPt = &INIT_TCB;
}

void scheduler_unschedule(TCB_t *thread) {
	TCB_LL_remove(&scheduled_thread, thread);
}

void scheduler_schedule(TCB_t *thread) {
	TCB_LL_append_circular(&scheduled_thread, thread);
}


TCB_t* round_robin_scheduler(void) {
	scheduled_thread = scheduled_thread->next_ptr;
	return scheduled_thread;
}


