/* LinkedList.c

Collection of some useful functions for a general linked list in c.

Note: These implementations are for a TCB_t linked list, 
and there is no templating in C.

*/


#include "../RTOS_Labs_common/OS.h"

// TODO Should these disable interrupts at all?

void TCB_LL_create_linear(TCB_t **head, TCB_t *node) {
	node->prev_ptr = 0;
	node->next_ptr = 0;
	*head = node;
}

void TCB_LL_create_circular(TCB_t **head, TCB_t *node) {
	node->prev_ptr = node;
	node->next_ptr = node;
	*head = node;
}


void TCB_LL_append_linear(TCB_t *head, TCB_t *node_to_add) {
	TCB_t *node = head;
	
	// Move to the end of the list
	while(node->next_ptr != 0) {
		node = node->next_ptr;
	}
	
	// Append
	node->next_ptr = node_to_add;
	node_to_add->prev_ptr = node;
	node_to_add->next_ptr = 0;
}

void TCB_LL_append_circular(TCB_t *head, TCB_t *node_to_add) {
	// Insert right behind the head
	TCB_t *node = head->prev_ptr;
	
	node_to_add->prev_ptr = node;
	node_to_add->next_ptr = head;
	head->prev_ptr = node_to_add;
	node->next_ptr = node_to_add;
}

void TCB_LL_remove(TCB_t *node) {
	TCB_t *prev = node->prev_ptr;
	TCB_t *next = node->next_ptr;
	
	prev->next_ptr = next;
	next->prev_ptr = prev;
	
}

TCB_t* TCB_LL_pop_head_linear(TCB_t **head) {
	TCB_t *node = *head;
	if(node == 0)
		return 0;
	
	*head = node->next_ptr;
	(*head)->prev_ptr=0;
	node->next_ptr = 0;
	node->prev_ptr = 0;
	
	return node;
}

TCB_t* TCB_LL_pop_tail_linear(TCB_t **head) {
	TCB_t *prev_node = *head;
	if(prev_node == 0)
		return 0;
	
	TCB_t *node = prev_node->next_ptr;
	
	while(node != 0) {
		prev_node = node;
		node = prev_node->next_ptr;
	}
	
	prev_node->next_ptr = 0;
	
	node->prev_ptr = 0;
	node->next_ptr = 0;
	
	return node;
}