// ************************** LinkedList.c **************************
// Collection of some useful functions for a general linked list in C.
// Author: Jackson Paull
// jackson.paull@utexas.edu
// v1.0

/*
	NOTE: All of these functions require the nodes to adhere to the struct standard
				outlined in LinkedList.h
				
				This means that all structs MUST have the first two members as pointers
				
				If this standard is not met, the casting will not work and the functions will fail
*/


#include "../RTOS_Labs_common/OS.h"
#include "LinkedList.h"

//******** LL_create_linear *************** 
// Create a linear linked list at *(head), beginning with node
// Linear means that the list is null terminated
// Inputs: 
//		LL_node_t **head: Pointer to the list head
//											i.e. location where the LL head pointer is stored
//		LL_node_t  *node: Pointer to the node to initialize the list with
// Outputs: none 
void LL_create_linear(LL_node_t **head, LL_node_t *node) {
	node->prev_ptr = 0;
	node->next_ptr = 0;
	*head = node;
}

//******** LL_create_circular *************** 
// Create a circular linked list at *(head), beginning with node
// Circular means that the last node points back to the first node
// Inputs: 
//		LL_node_t **head: Pointer to the list head
//											i.e. location where the LL head pointer is stored
//		LL_node_t  *node: Pointer to the node to initialize the list with
//											
// Outputs: none 
void LL_create_circular(LL_node_t **head, LL_node_t *node) {
	node->prev_ptr = node;
	node->next_ptr = node;
	*head = node;
}

//******** LL_append_linear *************** 
// Add a node to a linear linked list after the final node
// Inputs: 
//		LL_node_t **head: Pointer to the list head
//											i.e. location where the LL head pointer is stored
//		LL_node_t  *node: Pointer to the node to append onto the list
//											
// Outputs: none 
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

//******** LL_append_circular *************** 
// Add a node to a circular linked list after the final node, and before the head
// Inputs: 
//		LL_node_t **head: Pointer to the list head
//											i.e. location where the LL head pointer is stored
//		LL_node_t  *node: Pointer to the node to append onto the list
//											
// Outputs: none 
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

//******** LL_insert_circular *************** 
// Add a node to a circular linked list immediately after the head
// Inputs: 
//		LL_node_t **head: Pointer to the list head
//											i.e. location where the LL head pointer is stored
//		LL_node_t  *node: Pointer to the node to insert into the list
//											
// Outputs: none 
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

//******** LL_remove *************** 
// Remove a specific node from a linked list
// The head is automatically updated if the head node is removed
// This functions works for both circular and linear linked lists
// Inputs: 
//		LL_node_t **head: Pointer to the list head
//											i.e. location where the LL head pointer is stored
//		LL_node_t  *node: Pointer to the node which should be removed.
//											
// Outputs: none 
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
		else {
			*head = 0;
		}
	}
	
	// Unlink node from list completely
	node->next_ptr = 0;
	node->prev_ptr = 0;
}

//******** LL_pop_head_linear *************** 
// Remove the head of a linear linked list and return a pointer to it
// The head is automatically updated to the next node in the list
// If no further nodes exist, the head will be a null pointer
// The returned node will have its link pointers disconnected from the list
//
// Inputs: 
//		LL_node_t **head: Pointer to the list head
//											i.e. location where the LL head pointer is stored
//											
// Outputs: LL_node_t *node: pointer to the popped node 
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


//******** LL_LL_pop_tail_linear *************** 
// Remove the last node in a linear linked list and return a pointer to it
// If the head is removed, it is automatically updated to be a null pointer
// The returned node will have its link pointers disconnected from the list
//
// Inputs: 
//		LL_node_t **head: Pointer to the list head
//											i.e. location where the LL head pointer is stored
//											
// Outputs: LL_node_t *node: pointer to the popped node 
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