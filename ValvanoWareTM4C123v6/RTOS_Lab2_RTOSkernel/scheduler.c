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
TCB_t* round_robin_scheduler(TCB_t *run_pt) {
	static int init = 0;
	if(init == 0) {
		init = 1;
		return run_pt;
	}
	return run_pt->next_ptr;
}


