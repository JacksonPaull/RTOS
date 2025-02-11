/*
	contains implementations for a variety of different scheduler types
*/

#include "./scheduler.h"


TCB_t INIT_TCB;		// TCB Used upon first entry into the scheduler
TCB_t *scheduled_thread = 0;

void scheduler_init(TCB_t **RunPt) {
	// Back up one spot so that on first context switch the first thread runs
	scheduled_thread = scheduled_thread->prev_ptr;
	*RunPt = &INIT_TCB;
}

void scheduler_unschedule(TCB_t *thread) {
	TCB_LL_remove(&scheduled_thread, thread);
}

void scheduler_schedule(TCB_t *thread) {
	TCB_LL_append_circular(&scheduled_thread, thread);
}

// Schedule something to run next, instead of last in line.
// This can eventually be removed when a priority scheduler is introduced
void scheduler_schedule_immediate(TCB_t *thread) {
	TCB_LL_insert_circular(&scheduled_thread, thread);
}

/* round_robin_scheduler
Simple round robin scheduler which just goes in order around the list

Inputs: None
Outputs: pointer to next thread that should be run
*/
TCB_t* round_robin_scheduler(void) {
	if(scheduled_thread == 0)
		return &INIT_TCB;
	scheduled_thread = scheduled_thread->next_ptr;
	if(scheduled_thread->removeAfterScheduling) {
		scheduler_unschedule(scheduled_thread);
	}
	return scheduled_thread;
}


