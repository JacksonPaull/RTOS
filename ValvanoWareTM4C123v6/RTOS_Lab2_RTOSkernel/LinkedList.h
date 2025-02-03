


#ifndef __TCB_LL__
#define __TCB_LL__

#include "../RTOS_Labs_common/OS.h"

// TODO Should these disable interrupts at all?

void TCB_LL_create_linear(TCB_t **head, TCB_t *node);
void TCB_LL_create_circular(TCB_t **head, TCB_t *node);
void TCB_LL_append_linear(TCB_t *head, TCB_t *node_to_add);
void TCB_LL_append_circular(TCB_t *head, TCB_t *node_to_add);
void TCB_LL_remove(TCB_t *node);

TCB_t* TCB_LL_pop_head_linear(TCB_t **head);
TCB_t* TCB_LL_pop_tail_linear(TCB_t **head);
	
#endif