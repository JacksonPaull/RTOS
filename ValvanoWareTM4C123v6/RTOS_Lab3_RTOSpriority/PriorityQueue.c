/***************************************************************************
 * PriorityQueue.c																												 *
 * Author - Jackson Paull																									 *
 * Description - Implementation of a basic priority queue 								 *
 *               (as a linear linked list)																 *
 ****************************************************************************/


#include "PriorityQueue.h"


// Insert
void PrioQ_insert(PrioQ_node_t **head, PrioQ_node_t *node) {
	if(*head == 0) {
		// Create list
		*head = node;
		
		// Disconnect from any other nodes just in case
		node->next_ptr = 0;
		node->prev_ptr = 0;
		return;
	}
	
	// Check if need to redefine head
	PrioQ_node_t* prev_node = *head;
	if(prev_node->priority > node->priority) {
		// Insert at head
		prev_node->prev_ptr = node;
		node->next_ptr = prev_node;
		*head = node;
		return;
	}
	
	
	while(prev_node->next_ptr != 0) {
		PrioQ_node_t *n = prev_node->next_ptr;
		if((prev_node->priority >= node->priority) && (node->priority >  n->priority)) {
			// Insert here
			node->next_ptr = n;
			node->prev_ptr = prev_node;
			n->prev_ptr = node;
			prev_node->next_ptr = node;
			return;
		}
			
		prev_node = prev_node->next_ptr;
	}
	
	// Reached the end of the list, insert at the end
	prev_node->next_ptr = node;
	
	node->prev_ptr = prev_node;
	node->next_ptr = 0;
}

// Pop Head
PrioQ_node_t* PrioQ_pop(PrioQ_node_t **head) {
	PrioQ_node_t *head_node = *head;
	if(head_node != 0) {
		*head = head_node->next_ptr;
	}
	return head_node;
}