#ifndef __SCHEDULER__
#define __SCHEDULER__

#include "../RTOS_Labs_common/OS.h"
#include "../RTOS_Lab2_RTOSkernel/LinkedList.h"
#include "../RTOS_Lab3_RTOSpriority/PriorityQueue.h"

TCB_t* scheduler_next(void);
void scheduler_init(TCB_t **RunPt);
void scheduler_unschedule(TCB_t *thread);
void scheduler_schedule(TCB_t *thread);
void scheduler_schedule_immediate(TCB_t *thread);
void scheduler_update_priority(TCB_t *thread, uint8_t new_priority);

// Note: The scheduler locks itself, and 
// background threads unlock it when they finish running
void scheduler_unlock(void);
void scheduler_lock(void);


#endif