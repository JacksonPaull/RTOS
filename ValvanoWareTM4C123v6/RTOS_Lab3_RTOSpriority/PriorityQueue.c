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
		// Insert at head (can't be duplicate)
		prev_node->prev_ptr = node;
		node->next_ptr = prev_node;
		*head = node;
		return;
	}
	
	
	while(prev_node->next_ptr != 0) {
		PrioQ_node_t *n = prev_node->next_ptr;
		
		if(prev_node == node) // no duplicates
			return;
		
		if((prev_node->priority >= node->priority) && (node->priority <  n->priority)) {
			// Insert here
			node->next_ptr = n;
			node->prev_ptr = prev_node;
			n->prev_ptr = node;
			prev_node->next_ptr = node;
			return;
		}
			
		prev_node = prev_node->next_ptr;
	}
	
	if(prev_node == node) // no duplicates
			return;
	
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
		//unlink
		head_node->next_ptr = 0;
		head_node->prev_ptr = 0;
	}
	return head_node;
}


void PrioQ_remove(PrioQ_node_t **head, PrioQ_node_t *node) {
	LL_remove((LL_node_t **) head, (LL_node_t *) node);	
}

PrioQ_node_t* PrioQ_find(PrioQ_node_t **head, uint32_t priority) {
	PrioQ_node_t *node = *head;
	while(node) {
		if(node->priority == priority)
			break;
		node = node->next_ptr;
	}
	return node;
}