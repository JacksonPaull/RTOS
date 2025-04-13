

#ifndef PRIO_Q_H
#define PRIO_Q_H

#include "../RTOS_Labs_common/OS.h"
#include "../RTOS_Lab2_RTOSkernel/LinkedList.h"

typedef struct PrioQ_Node {
	struct PrioQ_Node *next_ptr, *prev_ptr;
	uint32_t priority;
	void *data;
} PrioQ_node_t;




// Insert
void PrioQ_insert(PrioQ_node_t **head, PrioQ_node_t *node);

// Pop Head
PrioQ_node_t* PrioQ_pop(PrioQ_node_t **head);

void PrioQ_remove(PrioQ_node_t **head, PrioQ_node_t *node);


// Return a pointer to the first element of specific priority
PrioQ_node_t* PrioQ_find(PrioQ_node_t **head, uint32_t priority);

#endif