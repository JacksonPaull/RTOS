/* iNode.h

iNodes are the structures that are actually stored on disk
*/

#ifndef INODE_H
#define INODE_H

#include <stdint.h>
#include "../RTOS_Labs_common/OS.h"


#define NUM_DIRECT_SECTORS 124
#define NUM_INDIRECT_SECTORS BLOCK_SIZE/4


// Note - iNode's must be exactly BLOCKSIZE in length

// Note - This supports ~8MB file length. 
// 				To increase, allow for TIP (~1.08GB filesize)
//														or QuadIP (~125GB filesize)
//														or QuintIP (~15TB filesize)
typedef struct iNodeDisk {
	uint8_t isDir;
	uint16_t magicHW;
	uint8_t magicByte;
	uint32_t size;
	uint32_t DP[NUM_DIRECT_SECTORS];
	uint32_t SIP;
	uint32_t DIP;
} iNodeDisk_t;

typedef struct iNode {
	struct PrioQ_Node *next_ptr, *prev_ptr;
	uint32_t sector_num;	// Used as the priority level in the iNode list
	uint8_t numOpen;
	uint8_t numReaders;
	uint8_t removed;
	Sema4Type NodeLock;
	iNodeDisk_t iNode;
	// Parent sector?
} iNode_t;



// ------------------------ Function definitions ------------------------------


uint32_t num_direct_sectors_occupied(iNode_t *node);
uint32_t num_direct_sectors_free(iNode_t *node);

uint32_t num_indirect_sectors_occupied(iNode_t *node);
uint32_t num_indirect_sectors_free(iNode_t *node);

uint32_t num_doubly_indirect_sectors_occupied(iNode_t *node);
uint32_t num_doubly_indirect_sectors_free(iNode_t *node);

int allocate_direct(iNode_t *iNode, uint32_t num_sectors);
int allocate_indirect(iNode_t *iNode, uint32_t num_sectors);
int allocate_doubly_indirect(iNode_t *iNode, uint32_t num_sectors);
int allocate_space(iNode_t *iNode, uint32_t num_bytes);
int iNode_create(uint32_t sector, uint32_t length, uint8_t isDir);
iNode_t* iNode_open(uint32_t sector);
iNode_t* iNode_reopen(iNode_t *node);
uint32_t iNode_get_sector(iNode_t *node);
int iNode_close(iNode_t *node);
void iNode_remove(iNode_t *node);
int iNode_read_at(iNode_t *node, void *buff, uint32_t size, uint32_t offset);
int iNode_write_at(iNode_t *node, const void *buff, uint32_t size, uint32_t offset);
uint32_t iNode_size(iNode_t *node);
void iNode_lock_read(iNode_t *node);
void iNode_lock_write(iNode_t *node);
void iNode_unlock_read(iNode_t *node);
void iNode_unlock_write(iNode_t *node);


#endif