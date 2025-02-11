


#ifndef __TCB_LL__H__
#define __TCB_LL__H__

#include "../RTOS_Labs_common/OS.h"

/*
NOTE: ALL structs which will be used in a linked list must have the next_ptr and prev_ptr as their first entries.
This assumes that the compiler will place all structs with their data in a row (which seems to be the case)
*/
typedef struct LL_NODE {
	struct LL_NODE *next_ptr, *prev_ptr;
	void *data;
} LL_node_t;

void LL_append_linear(LL_node_t **head, LL_node_t *node_to_add);
void LL_append_circular(LL_node_t **head, LL_node_t *node_to_add);
void LL_insert_circular(LL_node_t **head, LL_node_t *node_to_add);
void LL_remove(LL_node_t **head, LL_node_t *node);

LL_node_t* LL_pop_head_linear(LL_node_t **head);
LL_node_t* TCB_LL_pop_tail_linear(LL_node_t **head);
	
#endif