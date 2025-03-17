// filename *************************heap.c ************************
// Implements memory heap for dynamic memory allocation.
// Follows standard malloc/calloc/realloc/free interface
// for allocating/unallocating memory.

// Jacob Egner 2008-07-31
// modified 8/31/08 Jonathan Valvano for style
// modified 12/16/11 Jonathan Valvano for 32-bit machine
// modified August 10, 2014 for C99 syntax

/* This example accompanies the book
   "Embedded Systems: Real Time Operating Systems for ARM Cortex M Microcontrollers",
   ISBN: 978-1466468863, Jonathan Valvano, copyright (c) 2015

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains

 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
 
 
 /*
 Knuth's buddy heap algorithm.
 
 Blocks are always allocated in powers of 2.
 Large blocks can be cut in half to form two smaller blocks, which are "buddies"
 The first byte in a block defines its order
 The order is the power of 2 multiple of the base block size that this block is
 A negative order implies a free block
 A positive order implies a used block
 
 Blocks are iteratively merged with free buddies when they are free
 */


#include <stdint.h>
#include <stdlib.h>
#include "../RTOS_Labs_common/heap.h"

int8_t HEAP[HEAP_SIZE];

// TODO Fix this
void memset(void* base, int v, size_t num_bytes) {
	for(uint32_t i = 0; i < num_bytes; i++) {
		*((int8_t*)base+i) = v;
	}
}

void memcpy(int* dst, int *src, uint32_t num_bytes) {
	for(uint32_t i = 0; i < num_bytes; i++) {
		*(dst+i) = *(src+i);
	}
}

void* malloc(size_t size) {
	return Heap_Malloc(size);
}

void free(void* ptr) {
	Heap_Free(ptr);
}

// Take the (ceil of the) base 2 log (can be replaced with CLZ asm though...)
int8_t logbase2(uint32_t x) {
	int8_t i = 0;
	while(x > 0) {
		i++;
		x = x >> 1;
	}
	return i;
}

void* BuddyAdd(void* block_ptr, uint32_t block_size) {
	// Use caseting to force the bitwise XOR
	return (void *) ((uint32_t) block_ptr ^ block_size);
}


// Allows for an order of 128 = roughly 7MB of heap storage. 
// More than enough for this implementation, changing to a 32 bit integer increases this astronmically
// (i.e. on the order of nonabytes of potential storage)
uint32_t Order2Size(int8_t order) {
	if(order < 0) {
		return order *= -1;
	}
	return (1 << order) * BASE_ORDER_BYTES;
}

int8_t Size2Order(uint32_t size) {
	return logbase2(size) - logbase2(BASE_ORDER_BYTES);
}

//******** Heap_Init *************** 
// Initialize the Heap
// input: none
// output: always 0
// notes: Initializes/resets the heap to a clean state where no memory
//  is allocated.
int32_t Heap_Init(void){
	int8_t order = -1 * Size2Order(HEAP_SIZE);
	HEAP[0] = order;
  return 0; 
}


//******** Heap_Malloc *************** 
// Allocate memory, data not initialized
// input: 
//   desiredBytes: desired number of bytes to allocate
// output: void* pointing to the allocated memory or will return NULL
//   if there isn't sufficient space to satisfy allocation request
void* Heap_Malloc(int32_t desiredBytes){
	int8_t min_order = Size2Order(desiredBytes); // Relative to the base order
	uint32_t bs = Order2Size(min_order); 					// Effectively just rounds up to the nearest power of 2
	
	int8_t* current_block = HEAP;
	
	// We can jump by blocks of size min_order
	// Because blocks are always aligned based on their order
	while(current_block - HEAP < HEAP_SIZE) {
		if(*current_block > 0) {
			// Block is allocated, continue to next
			current_block += Order2Size(min_order);
			
		}
		else if(*current_block < -1 * min_order) {
			// Unnalocated large block, can split (don't advance)
			*current_block += 1;
			int8_t* buddy = BuddyAdd(current_block, Order2Size(*current_block));
			*buddy = *current_block;
		}
		if(*current_block == -1 * min_order) {
			// Mark as allocated
			*current_block *= -1;
			return current_block+1;
		}
	}
	return 0;
}


//******** Heap_Calloc *************** 
// Allocate memory, data are initialized to 0
// input:
//   desiredBytes: desired number of bytes to allocate
// output: void* pointing to the allocated memory block or will return NULL
//   if there isn't sufficient space to satisfy allocation request
//notes: the allocated memory block will be zeroed out
void* Heap_Calloc(int32_t desiredBytes){  
	void *ptr = Heap_Malloc(desiredBytes); // Alloc
	
	if(ptr)
		memset(ptr, 0, desiredBytes);
	
  return ptr;
}


//******** Heap_Realloc *************** 
// Reallocate buffer to a new size
//input: 
//  oldBlock: pointer to a block
//  desiredBytes: a desired number of bytes for a new block
// output: void* pointing to the new block or will return NULL
//   if there is any reason the reallocation can't be completed
// notes: the given block may be unallocated and its contents
//   are copied to a new block if growing/shrinking not possible
void* Heap_Realloc(void* oldBlock, int32_t desiredBytes){
	void* ptr = 0;
	// Check to see if we can grow this sector (critical section)
		// If possible, allocate away
		// Check buddy, then new levels of buddys until its confirmed that we can grow the block
		// Note: The buddies must be successive, not precursors
	
	// Else
		// Alloc new block
		ptr = Heap_Malloc(desiredBytes);
		memcpy(ptr, oldBlock, desiredBytes); // Copy extra garbage over but thats ok, this isnt calloc
		Heap_Free(oldBlock);
	
  return ptr;   // NULL
}



//******** Heap_Free *************** 
// return a block to the heap
// input: pointer to memory to unallocate
// output: 0 if everything is ok, non-zero in case of error (e.g. invalid pointer
//     or trying to unallocate memory that has already been unallocated
int32_t Heap_Free(void* pointer){
	//(note: pointers start after the block order)
	int8_t* ptr = (int8_t *) (pointer-1);
	
	// 1. Find the buddy
	int8_t* buddy = BuddyAdd(ptr, Order2Size(*ptr)); // TODO Fix
	
	
	
	// While buddy is free
	while(buddy[0] < 0) {
		// And is of the same size 
		if(buddy[0] == ptr[0]) {
			// Merge
			buddy[0] -= 1;
			ptr[0] -= 1;
			
			// Find new buddy
			buddy = HEAP; // TODO Fix
		}
	}
	
  return 0;
}


//******** Heap_Stats *************** 
// return the current status of the heap
// input: reference to a heap_stats_t that returns the current usage of the heap
// output: 0 in case of success, non-zeror in case of error (e.g. corrupted heap)
int32_t Heap_Stats(heap_stats_t *stats){
	stats->size = HEAP_SIZE;
	int8_t* currentBlock = HEAP;
	int8_t order = HEAP[0];
	
	// Go through the whole heap
	while(currentBlock - HEAP < HEAP_SIZE) {
		if(*currentBlock < 0) {
			// Block is free
			uint32_t n_bytes = MIN_SIZE << -(*currentBlock);
			stats->free += n_bytes - 1; // Account for the byte of overhead
			currentBlock += n_bytes;
		}
		else {
			// Block is used
			uint32_t n_bytes = MIN_SIZE << *currentBlock;
			stats->free += n_bytes - 1; // Account for the byte of overhead
			currentBlock += n_bytes;
		}
		
	}
  return 0;   // replace
}
