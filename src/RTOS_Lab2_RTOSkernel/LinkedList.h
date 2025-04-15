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


#ifndef __TCB_LL__H__
#define __TCB_LL__H__

#include "../RTOS_Labs_common/OS.h"

/*
NOTE: ALL structs which will be used in a linked list must have the next_ptr and prev_ptr as their first entries.
This assumes that the compiler will place all structs with their data in a row (which may not always be the case)
*/
typedef struct LL_NODE {
	struct LL_NODE *next_ptr, *prev_ptr;
	void *data;
} LL_node_t;


// TODO Update linked lists to use this head and create mutual exclusivity
//typedef struct LL_HEAD {
//	Sema4Type mutex;
//	LL_node_t *head_node;
//} LL_Head_t;

// ************************************** FUNCTIONS **************************************

//******** LL_append_linear *************** 
// Add a node to a linear linked list after the final node
// Inputs: 
//		LL_node_t **head: Pointer to the list head
//											i.e. location where the LL head pointer is stored
//		LL_node_t  *node: Pointer to the node to append onto the list
//											
// Outputs: none 
void LL_append_linear(LL_node_t **head, LL_node_t *node_to_add);



//******** LL_append_circular *************** 
// Add a node to a circular linked list after the final node, and before the head
// Inputs: 
//		LL_node_t **head: Pointer to the list head
//											i.e. location where the LL head pointer is stored
//		LL_node_t  *node: Pointer to the node to append onto the list
//											
// Outputs: none 
void LL_append_circular(LL_node_t **head, LL_node_t *node_to_add);



//******** LL_insert_circular *************** 
// Add a node to a circular linked list immediately after the head
// Inputs: 
//		LL_node_t **head: Pointer to the list head
//											i.e. location where the LL head pointer is stored
//		LL_node_t  *node: Pointer to the node to insert into the list
//											
// Outputs: none 
void LL_insert_circular(LL_node_t **head, LL_node_t *node_to_add);



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
void LL_remove(LL_node_t **head, LL_node_t *node);



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
LL_node_t* LL_pop_head_linear(LL_node_t **head);



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
LL_node_t* TCB_LL_pop_tail_linear(LL_node_t **head);
	
#endif