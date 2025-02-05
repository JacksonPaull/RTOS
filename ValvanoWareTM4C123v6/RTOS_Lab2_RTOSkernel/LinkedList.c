/* LinkedList.c

Collection of some useful functions for a general linked list in c.

Note: These implementations are for a TCB_t linked list, 
and there is no templating in C.

*/


#include "../RTOS_Labs_common/OS.h"

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


void TCB_LL_append_linear(TCB_t **head, TCB_t *node_to_add) {
	if(*head == 0)
		return TCB_LL_create_linear(head, node_to_add);
	
	TCB_t *node = *head;
	
	// Move to the end of the list
	while(node->next_ptr != 0) {
		node = node->next_ptr;
	}
	
	// Append
	node->next_ptr = node_to_add;
	node_to_add->prev_ptr = node;
	node_to_add->next_ptr = 0;
}

void TCB_LL_append_circular(TCB_t **head, TCB_t *node_to_add) {
	if(head == 0) {
		return TCB_LL_create_circular(head, node_to_add);
	}
	
	// Find adjacent nodes
	TCB_t *head_node = *head;
	TCB_t *node = head_node->prev_ptr;
	
	// Link this node to other nodes
	node_to_add->prev_ptr = node;
	node_to_add->next_ptr = head_node;
	
	// Link other nodes to this node (finish adding to the list)
	head_node->prev_ptr = node_to_add;
	node->next_ptr = node_to_add;
}

void TCB_LL_remove(TCB_t **head, TCB_t *node) {
	TCB_t *prev = node->prev_ptr;
	TCB_t *next = node->next_ptr;
	
	// Adjust other nodes (if they exist)
	if(prev != 0){
		prev->next_ptr = next;
	}
	if(next != 0) {
		next->prev_ptr = prev;
	}
	
	// Adjust head if we are removing the head
	if(*head == node) {
		if(next != 0) {
			*head = next;
		}
		else if(prev != 0) {
			*head = prev;
		}
		else {
			*head = 0;
		}
	}
}

TCB_t* TCB_LL_pop_head_linear(TCB_t **head) {
	TCB_t *node = *head;
	if(node == 0)
		return 0;
	
	// Set new head
	*head = node->next_ptr;
	
	if(*head != 0) {
		// Adjust new head
		(*head)->prev_ptr=0;
	}
	
	// Remove references
	node->next_ptr = 0;
	node->prev_ptr = 0;
	
	return node;
}

TCB_t* TCB_LL_pop_tail_linear(TCB_t **head) {
	TCB_t *node = *head;
	
	// Return nothing in case of empty list
	if(node == 0)
		return 0;
	
	// Move to end of list
	while(node->next_ptr != 0) {
		node = node->next_ptr;
	}
	
	if(node->prev_ptr != 0) {
		// Adjust previous node
		(node->prev_ptr)->next_ptr = 0;
	}
	
	// Remove references
	node->prev_ptr = 0;
	node->next_ptr = 0;
	
	return node;
}