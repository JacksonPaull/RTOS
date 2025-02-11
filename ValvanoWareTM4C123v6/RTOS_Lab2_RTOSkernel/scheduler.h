#ifndef __SCHEDULER__
#define __SCHEDULER__

#include "../RTOS_Labs_common/OS.h"
#include "../RTOS_Lab2_RTOSkernel/LinkedList.h"

TCB_t* round_robin_scheduler(void);
void scheduler_init(TCB_t **RunPt);
void scheduler_unschedule(TCB_t *thread);
void scheduler_schedule(TCB_t *thread);
void scheduler_schedule_immediate(TCB_t *thread);
#endif