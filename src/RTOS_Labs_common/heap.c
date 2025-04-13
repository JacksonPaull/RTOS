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

// TODO: Update to use knuth buddy allocation instead of this

#include <stdint.h>
#include <stdlib.h>
#include "../RTOS_Labs_common/heap.h"
#include "../RTOS_Labs_common/OS.h"


extern int8_t HeapMem[HEAP_SIZE];	// Align to full word addresses

uint32_t getHeapSize(void) {
	TCB_t *r = OS_get_current_TCB();
	if(r && r->process) {
		return r->process->heap_size;
	}
	return HEAP_SIZE;
}

int8_t* getHeapBase(void) {
	TCB_t *r = OS_get_current_TCB();
	if(r && r->process) {
		return r->process->heap;
	}
	return HeapMem;
}

void* memset(void* dst, int v, size_t num_bytes) {
	for(uint32_t i = 0; i < num_bytes; i++) {
		*((int8_t*)dst+i) = v;
	}
	return dst;
}

void* memcpy(void* dst, const void *src, size_t num_bytes) {
	for(uint32_t i = 0; i < num_bytes; i++) {
		*((int* ) dst+i) = *((int *) src+i);
	}
	return dst;
}

void* malloc(size_t size) {
	// Get current process heap, then pass to Heap_Malloc
	return Heap_Malloc(size);
}

void free(void* ptr) {
	Heap_Free(ptr);
}

//// Take the (ceil of the) base 2 log (can be replaced with CLZ asm though...)
//int8_t logbase2(uint32_t x) {
//	int8_t i = 0;
//	while(x > 0) {
//		i++;
//		x = x >> 1;
//	}
//	return i;
//}

//int8_t* BuddyAdd(int8_t* block_ptr, uint32_t block_size) {
//	uint32_t hs = getHeapSize();
//	int8_t* heap = getHeapBase();
//	
//	// Use casting to force the bitwise XOR
//	return (int8_t *) ((uint32_t) (block_ptr-heap) ^ block_size + (uint32_t) heap);
//}


//// Allows for an order of 128 = roughly 7MB of heap storage. 
//// More than enough for this implementation, changing to a 32 bit integer increases this astronmically
//// (i.e. on the order of nonabytes of potential storage)
//uint32_t Order2Size(int8_t order) {
//	if(order < 0) { // Take abs so this works for allocated or unallocated blocks
//		order *= -1;
//	}
//	return (1 << order) * BASE_ORDER_BYTES;
//}

//int8_t Size2Order(uint32_t size) {
//	return logbase2(size) - logbase2(BASE_ORDER_BYTES);
//}



//******** Heap_Init *************** 
// Initialize the Heap
// input: none
// output: always 0
// notes: Initializes/resets the heap to a clean state where no memory
//  is allocated.
int32_t Heap_Init(void){
	uint32_t hs = getHeapSize();
	int32_t* heap = (int32_t*) getHeapBase();
	
	// set header and footer
	heap[0] = 8-hs;
	heap[hs/4-1] = 8-hs;
	
  return 0; 
}


//******** Heap_Malloc *************** 
// Allocate memory, data not initialized
// input: 
//   desiredBytes: desired number of bytes to allocate
// output: void* pointing to the allocated memory or will return NULL
//   if there isn't sufficient space to satisfy allocation request
void* Heap_Malloc(int32_t desiredBytes){
	desiredBytes = (desiredBytes+3)/4 * 4; // Round up to nearest word
	
	int I = StartCritical();
	uint32_t hs = getHeapSize();
	void* heap = getHeapBase();
	
	void* currentBlock = heap;
	
	while(currentBlock - heap < hs) {
		int32_t block_size = *((int32_t*) currentBlock);
		
		if(block_size < 0 && block_size == -1 * desiredBytes) {
			// Large enough free block to allocate directly
			*(int32_t *) currentBlock 			 					= desiredBytes;		// Allocated header
			*(int32_t *)(currentBlock-block_size+4)   = desiredBytes;	// Fragment footer
			
			EndCritical(I);
			return currentBlock+4;
		}
		
		if(block_size < 0 && block_size + 8 <= -1 * desiredBytes) {
			// Large enough free block to split and allocate
			int32_t frag_size = block_size + desiredBytes + 8; // Size of the new block accounting for the new header and footer in the middle
			
			*(int32_t *) currentBlock 			 					= desiredBytes;		// Allocated header
			*(int32_t *)(currentBlock+desiredBytes+4) = desiredBytes;		// Allocated footer
			*(int32_t *)(currentBlock+desiredBytes+8) = frag_size;	// Frament header
			*(int32_t *)(currentBlock-block_size+4)   = frag_size;	// Fragment footer
			
			EndCritical(I);
			return currentBlock+4;
		}
		
		if(block_size < 0) {
			currentBlock += 8 - block_size;
		}
		else {
			currentBlock += block_size + 8;
		}
	}
	
	EndCritical(I);
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
	desiredBytes = (desiredBytes+3)/4 * 4;	// Round up to the nearest word
	void* ptr = 0;
	
	int I = StartCritical();
	int32_t* block_header = (int32_t*) (oldBlock-4);
	int32_t block_size = *block_header;
	int32_t* nextBlockHeader = (int32_t*) (oldBlock + block_size + 4);
	int32_t next_block_size = *nextBlockHeader;
	
	if(desiredBytes > block_size) { // Potential for growth
			
			if(next_block_size < 0 && 															// Next Block Free
				block_size - next_block_size  + 8 == desiredBytes) { 	// Account for the fact that we can remove the header and footer in the middle potentially
				// Merge both of these blocks together as one
				*block_header = desiredBytes;
				*(nextBlockHeader+next_block_size/4+1) 	= desiredBytes;
				ptr = oldBlock;
			}
			else if (next_block_size < 0 &&
				block_size-next_block_size >= desiredBytes) {
				// Next page is free and large enough to grow this sector instead of reallocating , but we still need the headers and footers
				*block_header 													= desiredBytes;								// Alloc header
				*(block_header+desiredBytes/4+1)  			= desiredBytes; 							// Alloc footer
				*(block_header+desiredBytes/4+2)  			= next_block_size-block_size + desiredBytes; 		// Frag header
				*(nextBlockHeader+next_block_size/4+1) 	= next_block_size-block_size + desiredBytes ; 	// Frag footer
				
				ptr = oldBlock;
				// Note: if the next block isnt free, then nextBlockHeader will be positive, and therefore the subtraction will be less than the desired bytes
			}
			else { // Alloc new block
				ptr = Heap_Malloc(desiredBytes);
				memcpy(ptr, oldBlock, block_size);
				Heap_Free(oldBlock);
			}
	}
	else if(desiredBytes < block_size - 8) {
		// shrinkage
		
		*block_header 									= desiredBytes;								// Alloc header
		*(block_header+desiredBytes/4+1)= desiredBytes; 							// Alloc footer
		*(block_header+desiredBytes/4+2)= desiredBytes-block_size+8; 	// Frag header
		*(block_header+block_size/4+1) 	= desiredBytes-block_size+8; 	// Frag footer
		
		ptr = oldBlock;
	}	
	
	// if the desired bytes is exactly what we have allocated already then who cares
	// Additionally, if we are shrinking, we need to insert a new header and footer
	
	EndCritical(I);
  return ptr;   // NULL
}



//******** Heap_Free *************** 
// return a block to the heap
// input: pointer to memory to unallocate
// output: 0 if everything is ok, non-zero in case of error (e.g. invalid pointer
//     or trying to unallocate memory that has already been unallocated
int32_t Heap_Free(void* pointer){
	if(!pointer) 
		return 0; // Freeing null pointer OK
	
	int I = StartCritical();
	
	uint32_t hs = getHeapSize();
	int8_t* heap = getHeapBase();
	int32_t* block_header = (int32_t*) (pointer-4);
	int32_t* block_footer = (int32_t*) (pointer + *block_header);
	
	if(*block_header != *block_footer) {
		EndCritical(I);
		return 1; // uh oh
	}
	
	// Mark block as free
	*block_footer *= -1;
	*block_header *= -1;
	
	
	// Check to see if we can merge with the next and previous block (and also make sure they exist before doing so)
	// Also reclaim space for header and footer
	if(((int8_t*)block_header - heap >= 8) 
			&& *(block_header-1) < 0) {
		// Merge
		int32_t* new_header = block_header + *(block_header-1)/4 - 2;
		
		*new_header 	= *(block_header-1) + *(block_header)-8;
		*block_footer = *(block_header-1) + *(block_header)-8;
		
		block_header = new_header;
		
	}
	if(((int8_t*)block_footer - heap + 8 < hs) && *(block_footer+1) < 0) {
		// Merge
		int32_t* footer = block_footer - *(block_footer+1)/4 + 2;
		
		*block_header = *(block_footer) + *(block_footer+1)-8;
		*footer 			= *(block_footer) + *(block_footer+1)-8;
	}
	
	EndCritical(I);
  return 0;
}


//******** Heap_Stats *************** 
// return the current status of the heap
// input: reference to a heap_stats_t that returns the current usage of the heap
// output: 0 in case of success, non-zeror in case of error (e.g. corrupted heap)
int32_t Heap_Stats(heap_stats_t *stats){
	int I = StartCritical();
	uint32_t hs = getHeapSize();
	int8_t* heap = getHeapBase();
	
	stats->size = hs;
	stats->free = 0;
	stats->used = 0;
	int8_t* currentBlock = heap;
	
	
	// Go through the whole heap
	while(currentBlock - heap < hs) {
		int32_t n_bytes = *((int32_t*) currentBlock);
		
		if(n_bytes < 0) {
			// Block is free
			n_bytes *= -1;
			stats->free += n_bytes; 
		}
		else {
			// Block is used
			stats->used += n_bytes; 
		}
		
		// Check that the footer matches the header
		int32_t check = *((int32_t*) (currentBlock+4+n_bytes));
		if(check < 0) check *= -1;
		if(check != n_bytes) {
			EndCritical(I);
			return 1;
		}
		
		currentBlock += n_bytes+8; // Account for header / footer
	}
	EndCritical(I);
  return 0;
}
