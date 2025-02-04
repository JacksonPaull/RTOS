#ifndef __SCHEDULER__
#define __SCHEDULER__

#include "../RTOS_Labs_common/OS.h"

TCB_t* round_robin_scheduler(void);
void scheduler_init(TCB_t *first_thread);

#endif