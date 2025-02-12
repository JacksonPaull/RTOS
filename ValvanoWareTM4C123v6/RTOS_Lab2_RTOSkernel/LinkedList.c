/* LinkedList.c

Collection of some useful functions for a general linked list in c.

Note: These implementations are for a LL_node_t linked list, 
and there is no templating in C.

*/


#include "../RTOS_Labs_common/OS.h"
#include "LinkedList.h"

void LL_create_linear(LL_node_t **head, LL_node_t *node) {
	node->prev_ptr = 0;
	node->next_ptr = 0;
	*head = node;
}

void LL_create_circular(LL_node_t **head, LL_node_t *node) {
	node->prev_ptr = node;
	node->next_ptr = node;
	*head = node;
}


void LL_append_linear(LL_node_t **head, LL_node_t *node_to_add) {
	if(*head == 0)
		return LL_create_linear(head, node_to_add);
	
	LL_node_t *node = *head;
	
	// Move to the end of the list
	while(node->next_ptr != 0) {
		node = node->next_ptr;
	}
	
	// Append
	node->next_ptr = node_to_add;
	node_to_add->prev_ptr = node;
	node_to_add->next_ptr = 0;
}

void LL_append_circular(LL_node_t **head, LL_node_t *node_to_add) {
	if(*head == 0) {
		return LL_create_circular(head, node_to_add);
	}
	
	// Find adjacent nodes
	LL_node_t *head_node = *head;
	LL_node_t *node = head_node->prev_ptr;
	
	// Link this node to other nodes
	node_to_add->prev_ptr = node;
	node_to_add->next_ptr = head_node;
	
	// Link other nodes to this node (finish adding to the list)
	head_node->prev_ptr = node_to_add;
	node->next_ptr = node_to_add;
}

void LL_insert_circular(LL_node_t **head, LL_node_t *node_to_add) {
	if(*head == 0) {
		return LL_create_circular(head, node_to_add);
	}
	
	// Find adjacent nodes
	LL_node_t *head_node = *head;
	LL_node_t *node = head_node->next_ptr;
	
	// Link this node to other nodes
	node_to_add->prev_ptr = head_node;
	node_to_add->next_ptr = node;
	
	// Link other nodes to this node (finish adding to the list)
	head_node->next_ptr = node_to_add;
	node->prev_ptr = node_to_add;
}

void LL_remove(LL_node_t **head, LL_node_t *node) {
	LL_node_t *prev = node->prev_ptr;
	LL_node_t *next = node->next_ptr;
	
	// Adjust other nodes (if they exist)
	if(prev != 0 && prev != node){
		prev->next_ptr = next;
	}
	if(next != 0 && next != node) {
		next->prev_ptr = prev;
	}
	
	// Adjust head if we are removing the head
	if(*head == node) {
		if(next != 0) {
			*head = next;
		}
//		else if(prev != 0) {
//			*head = prev;
//		}
		else {
			*head = 0;
		}
	}
	
	// Unlink node from list completely
	node->next_ptr = 0;
	node->prev_ptr = 0;
}

LL_node_t* LL_pop_head_linear(LL_node_t **head) {
	LL_node_t *node = *head;
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

LL_node_t* LL_LL_pop_tail_linear(LL_node_t **head) {
	LL_node_t *node = *head;
	
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