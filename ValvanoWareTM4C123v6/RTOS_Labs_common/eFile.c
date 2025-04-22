// filename ************** eFile.c *****************************
// High-level routines to implement a solid-state disk 
// Students implement these functions in Lab 4
// Jonathan W. Valvano 1/12/20
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "../RTOS_Labs_common/eDisk.h"
#include "../RTOS_Labs_common/eFile.h"
#include "../RTOS_Lab3_RTOSpriority/PriorityQueue.h"
#include "../RTOS_Lab4_FileSystem/Bitmap.h"
#include "../RTOS_Lab4_FileSystem/iNode.h"
#include "../RTOS_Lab5_ProcessLoader/svc.h"

uint32_t NumSectors = 4096;
uint32_t SectorSize = 512;

uint8_t zeros[BLOCK_SIZE];
iNode_t DISK_INODES[MAX_NODES_OPEN];
PrioQ_node_t *Open_Nodes_Head;

uint8_t buff1[BLOCK_SIZE];
uint8_t buff2[BLOCK_SIZE];
char pathbuff[BLOCK_SIZE];

Sema4Type buff1_lock;
Sema4Type buff2_lock;
Sema4Type pathbuff_lock;

// TODO Make work with general bitmap
#define ROOTDIR_INODE 1

// TODO allocate two global buffers, and provide mutex access to them for all iNode buffering

// -------------------- ------------ Utility Functions -------------------------------------- //



//size_t strlen(const char *str) {
//	size_t i;
//	while(str[i] != 0) {i++;}
//	return i;
//}

char* strcpy(char *dst, const char *src) {
	for(size_t i = 0; 1; i++) {
		((char *) dst)[i] = ((char *) src)[i];
		if(src[i] == 0) {
			return dst;
		}
	}
	return 0; // should never execute
}



int32_t min(int32_t a, int32_t b) {
	if(a < b) {
		return a;
	}
	return b;
}

int32_t max(int32_t a, int32_t b) {
	if(a > b) {
		return a;
	}
	return b;
}

/* Bytes2Sectors - gives the number of sectors that <bytes> will take up */
uint32_t Bytes2Sectors(uint32_t bytes) {
	return (bytes+BLOCK_SIZE-1)/BLOCK_SIZE;
}

/* FilePos2Sector - returns the sector that a given file pos will be in*/
uint32_t FilePos2Sector(iNode_t *node, uint32_t pos) {
	uint32_t r;
	if(pos > node->iNode.size) {
		// The position exists outside the file
		return 0;
	}
	
	uint32_t n = pos / BLOCK_SIZE; // The (file) sector that the position resides in
	if(n < NUM_DIRECT_SECTORS) {
		// The pos is in a direct data sector
		return node->iNode.DP[n];
	}
	
	SVC_Wait(&buff1_lock);
	uint8_t* buf = buff1;
	n -= NUM_DIRECT_SECTORS;
	if(n < NUM_DIRECT_SECTORS) {
		// The pos is in an indirect data sector
		eDisk_ReadBlock(buf, node->iNode.SIP);
		r = buf[n];
		SVC_Signal(&buff1_lock);
		return r;
	}
	
	n -= NUM_DIRECT_SECTORS;
	if(n < NUM_DIRECT_SECTORS*NUM_DIRECT_SECTORS) {
		// The pos is in one of the doubly indirect sectors
		eDisk_ReadBlock(buf, node->iNode.DIP);		// Load doubly indirect data sector
		eDisk_ReadBlock(buf, buf[n/BLOCK_SIZE]);	// Load the (singly) indirect data sector
		r = buf[n % BLOCK_SIZE];
		SVC_Signal(&buff1_lock);
		return r;
	}
	
	// The position is outside the max file length
	SVC_Signal(&buff1_lock);
	return 0;
}


// ---------------------------------- iNode Functions -------------------------------------- //
uint32_t num_direct_sectors_occupied(iNode_t *node) {
	uint32_t num_sectors = Bytes2Sectors(node->iNode.size);
	return min(NUM_DIRECT_SECTORS, num_sectors);
}

uint32_t num_direct_sectors_free(iNode_t *node) {
	return NUM_DIRECT_SECTORS - num_direct_sectors_occupied(node);
}


uint32_t num_indirect_sectors_occupied(iNode_t *node) {
	int32_t num_sectors = max(Bytes2Sectors(node->iNode.size) - NUM_DIRECT_SECTORS, 0);
	return min(num_sectors, NUM_INDIRECT_SECTORS);
}

uint32_t num_indirect_sectors_free(iNode_t *node) {
	return NUM_INDIRECT_SECTORS - num_indirect_sectors_occupied(node);
}

uint32_t num_doubly_indirect_sectors_occupied(iNode_t *node) {
	int32_t num_sectors = max(Bytes2Sectors(node->iNode.size) - NUM_DIRECT_SECTORS - NUM_INDIRECT_SECTORS, 0);
	return min(num_sectors, NUM_INDIRECT_SECTORS*NUM_INDIRECT_SECTORS);
}

uint32_t num_doubly_indirect_sectors_free(iNode_t *node) {
	return NUM_DIRECT_SECTORS*NUM_INDIRECT_SECTORS - num_doubly_indirect_sectors_occupied(node);
}

// Returns 1 TRUE on success, FALSE on fail
int allocate_direct(iNode_t *iNode, uint32_t num_sectors) {
	
	uint32_t base = num_direct_sectors_occupied(iNode);
	if(num_sectors + base > NUM_DIRECT_SECTORS) {
		// Unable to allocate all the necessary sectors
		return 0;
	}
	
	SVC_Wait(&buff1_lock);
	uint32_t* buff = (uint32_t *) buff1;
	Bitmap_AllocN(buff, num_sectors);
	
	for(int i = 0; i < num_sectors; i++) {
		eDisk_WriteBlock(zeros, buff[i]);		// Erase block on disk
		iNode->iNode.DP[base+i] = buff[i];
	}
	SVC_Signal(&buff1_lock);
	return 1;
}

int allocate_indirect(iNode_t *iNode, uint32_t num_sectors) {
	uint32_t base = num_indirect_sectors_occupied(iNode);
	if(num_sectors + base > NUM_INDIRECT_SECTORS) {
		return 0;
	}
	
	SVC_Wait(&buff1_lock);
	SVC_Wait(&buff2_lock);
	
	uint32_t* buff = (uint32_t *) buff1;
	uint32_t* SIP_data = (uint32_t *) buff2;
	
	Bitmap_AllocN(buff, num_sectors); // Allocate the sectors (not necessarily continuous, but usually will be)
	eDisk_ReadBlock(SIP_data, iNode->iNode.SIP);
	
	
	for(int i = 0; i < num_sectors; i++) {
		eDisk_WriteBlock(zeros, buff[i]);		// Erase block on disk
		SIP_data[base+i] = buff[i];
	}
	eDisk_WriteBlock(SIP_data, iNode->iNode.SIP);
	
	SVC_Signal(&buff2_lock);
	SVC_Signal(&buff1_lock);
	return 1;
}

int allocate_doubly_indirect(iNode_t *iNode, uint32_t num_sectors) {
	
	uint32_t base = num_doubly_indirect_sectors_occupied(iNode);
	if(num_sectors + base > NUM_INDIRECT_SECTORS*NUM_DIRECT_SECTORS) {
		return 0;
	}
	
	// TODO fix
	uint32_t buff[10];
	Bitmap_AllocN(buff, num_sectors); // Allocate the sectors (not necessarily continuous, but usually will be)
	SVC_Wait(&buff1_lock);
	SVC_Wait(&buff2_lock);
	uint32_t* DIP1_data = (uint32_t *) buff1;
	uint32_t* DIP2_data = (uint32_t *) buff1;
	eDisk_ReadBlock(DIP1_data, iNode->iNode.DIP);
	
	for(int i = 0; i < num_sectors; i++) {
		eDisk_WriteBlock(zeros, buff[i]);		// Erase block on disk
		
		// Read in second indirect block
		uint32_t n1 = (base+i)/NUM_INDIRECT_SECTORS; 
		eDisk_ReadBlock(DIP2_data, DIP1_data[n1]);
		for(uint32_t n2 = (base+i)%NUM_INDIRECT_SECTORS; n2 < NUM_INDIRECT_SECTORS; n2++) {
			DIP2_data[n2] = buff[i++]; // Mark the allocated sector and move the pointer
		}
		eDisk_WriteBlock(DIP2_data, DIP1_data[n1]);
	}
	eDisk_WriteBlock(DIP1_data, iNode->iNode.DIP);
	
	SVC_Signal(&buff2_lock);
	SVC_Signal(&buff1_lock);
	return 1;
}


// Allocates enough space for num_bytes additional bytes (and increases file size)
int allocate_space(iNode_t *iNode, uint32_t num_bytes) {
	uint32_t num_current_sectors = Bytes2Sectors(iNode->iNode.size);
	uint32_t num_sectors = Bytes2Sectors(iNode->iNode.size + num_bytes);
	num_sectors -= num_current_sectors;
	uint32_t num_allocated = 0;
	
	if(num_current_sectors < NUM_DIRECT_SECTORS) {
		// Can allocate direct sectors
		num_allocated = min(num_direct_sectors_free(iNode), num_sectors);
	
		if(num_allocated == 0) {
			// We can use the space in the sectors already allocated
			iNode->iNode.size += num_bytes;
			num_bytes = 0;
		}
		else {
			if(!allocate_direct(iNode, num_allocated)) {
				return 0; // failed to allocate
			}
			
			uint32_t b = min(num_bytes, BLOCK_SIZE*num_allocated);
			num_bytes -= b;
			iNode->iNode.size += b;
			num_current_sectors += num_allocated;
			num_sectors -= num_allocated;
		}
	}
	
	if(num_current_sectors < NUM_INDIRECT_SECTORS + NUM_DIRECT_SECTORS) {
		// Can allocate indirect
		num_allocated = min(num_indirect_sectors_free(iNode), num_sectors);
		if(num_allocated == 0) {
			iNode->iNode.size += num_bytes;
			num_bytes = 0;
		}
		else {
			if(!allocate_indirect(iNode, num_allocated)) {
				return 0;
			}
			
			uint32_t b = min(num_bytes, BLOCK_SIZE*num_allocated);
			num_bytes -= b;
			iNode->iNode.size += b;
			num_current_sectors += num_allocated;
			num_sectors -= num_allocated;
		}
		
		
	}
	
	if(num_current_sectors < NUM_DIRECT_SECTORS*NUM_INDIRECT_SECTORS + NUM_INDIRECT_SECTORS + NUM_DIRECT_SECTORS) {
		// Can allocate doubly indirect
		num_allocated = min(num_doubly_indirect_sectors_free(iNode), num_sectors);
		if(num_allocated == 0) {
			iNode->iNode.size += num_bytes;
			num_bytes = 0;
		}
		else {
			if(!allocate_doubly_indirect(iNode, num_allocated)) {
				return 0;
			}
			
			uint32_t b = min(num_bytes, BLOCK_SIZE*num_allocated);
			num_bytes -= b;
			iNode->iNode.size += b;
			num_current_sectors += num_allocated;
			num_sectors -= num_allocated;
		}
	}
	
	eDisk_WriteBlock(&iNode->iNode, iNode->sector_num);
	if(num_bytes != 0) {
		return 0; // Failed somewhere
	}
	
	return 1; // Success
}

iNode_t* iNode_find(uint32_t sector) {
	// Check if already in memory
	int I = StartCritical();
	iNode_t *node = (iNode_t*) PrioQ_find(&Open_Nodes_Head, sector);
	EndCritical(I);
	return node;
}

iNode_t* iNode_spawn(uint32_t sector) {
	iNode_t *node = 0;
	int I = StartCritical();
	for(int i = 0; i < MAX_NODES_OPEN; i++) {
		iNode_t *n = &DISK_INODES[i];
		if(n->sector_num == 0) {
			// this node is unused, use this one
			node = n;
			break;
		}
	}
	
	EndCritical(I);
	if(!node) {
		return 0;
	}
	memset(node, 0, sizeof(iNode_t));
	
	I = StartCritical();
	PrioQ_insert(&Open_Nodes_Head, (PrioQ_node_t *) node);
	EndCritical(I);
	return node;
}

int iNode_create(uint32_t sector, uint32_t length, uint8_t isDir) {
	volatile int status = 0;
	iNode_t *node = iNode_find(sector);
	if(!node) {
		node = iNode_spawn(sector);
	}
	
	
	// Note - creating a node is not equivalent to opening it,
	// Here we only write out the data to disk to create the iNode
	node->sector_num = sector;
	node->iNode.isDir = isDir;
	node->iNode.magicByte = 0x12;
	node->iNode.magicHW = 0x3456;

	if(allocate_space(node, length)) {
		eDisk_WriteBlock(&node->iNode, node->sector_num);
		status = 1;
	}
	
	// Remove from memory list (i.e. free the buffer)
	PrioQ_remove(&Open_Nodes_Head, (PrioQ_node_t *) node);
	node->sector_num = 0;
	return status;
}

iNode_t* iNode_open(uint32_t sector) {
	// Check if already in memory
	iNode_t *node = iNode_find(sector);
	if(node) {
		return iNode_reopen(node);
	}
	node = iNode_spawn(sector);
	
	// Read in from disk
	eDisk_ReadBlock(&node->iNode, sector);
	node->sector_num = sector;
	node->numOpen=1;
	SVC_InitSemaphore(&node->NodeLock, 1);

	return node;
}


iNode_t* iNode_reopen(iNode_t *node) {
	// incerase the num_open count
	int i = StartCritical();
	node->numOpen++;
	EndCritical(i);
	return node;
}

uint32_t iNode_get_sector(iNode_t *node) {
	return node->sector_num;
}

int iNode_close(iNode_t *node) {
	int r = 1;
	
	if(node == 0) {
		return 1; // closing nothing is ok
	}
	
	// decrement the num_open count
	if(--(node->numOpen) == 0) {
		// Nothing has this open anymore
		if(node->removed) {
			// Free from disk entirely (reclaim bitmap space)
			uint32_t s = Bytes2Sectors(node->iNode.size);
			
			// Direct sectors
			uint32_t l = min(NUM_DIRECT_SECTORS, s);
			for(uint32_t i = 0; i < l; i++) {
				Bitmap_free(node->iNode.DP[i]);
			}
			s -= l;
			
			if(s > 0) {
				SVC_Wait(&buff1_lock);
				uint32_t *b1 = (uint32_t *)buff1;
				// Indirect sectors
				l = min(NUM_INDIRECT_SECTORS, s);
				r &= !eDisk_ReadBlock(b1, node->iNode.SIP);
				for(uint32_t i = 0; i < l; i++) {
					Bitmap_free(b1[i]);
				}
				s -= l;
				
				if(s > 0) {
					SVC_Wait(&buff2_lock);
					uint32_t *b2 = (uint32_t *) buff2;
					
					// Doubly indirect sectors
					uint32_t nds = (s+NUM_INDIRECT_SECTORS-1)/NUM_INDIRECT_SECTORS;
					r &= !eDisk_ReadBlock(b1, node->iNode.DIP);
					for(uint32_t i = 0; i < nds; i++) {
						r &= !eDisk_ReadBlock(b2, b1[i]);
						
						l = min(s, NUM_INDIRECT_SECTORS);
						for(uint32_t j = 0; j < l; j++) {
							Bitmap_free(b2[j]);
						}
						s -= l;
					}
					SVC_Signal(&buff2_lock);
				}
				SVC_Signal(&buff1_lock);
			}
		}
		
		// Remove from list in RAM
		int I = StartCritical();
		PrioQ_remove(&Open_Nodes_Head, (PrioQ_node_t *) node);
		node->sector_num = 0; // Mark it as unused for the list
		// free(node);
		EndCritical(I);
		
	}
	return r;
}


void iNode_remove(iNode_t *node) {
	// Mark iNode to be removed when it is closed by the last caller who has it open
	node->removed = 1;
}


// Must be called when the node lock is held!
int iNode_read_at(iNode_t *node, void* buff, uint32_t size, uint32_t offset) {
	
	// Read from appropriate data sector and place in the buffer
	uint32_t iNode_left = node->iNode.size - offset;
	uint32_t bytes_read = 0;
	if(size > iNode_left) {
		return 0;
	}
	
	while( size > 0) {
		iNode_left = node->iNode.size - offset;
		uint32_t block_ofs = offset % BLOCK_SIZE;
		uint32_t block_left = BLOCK_SIZE - block_ofs;
		uint32_t s = FilePos2Sector(node, offset);
		
		uint32_t toRead = min(iNode_left, block_left);
		toRead = min(toRead, size);
		if(block_ofs == 0 && toRead == BLOCK_SIZE) {
			// Read entire block
			eDisk_ReadBlock(buff+bytes_read, s);
			
		}
		else {
			// Read only a portion of the block
			SVC_Wait(&buff1_lock);
			eDisk_ReadBlock(buff1, s);
			memcpy(buff+bytes_read, buff1+block_ofs, toRead);
			SVC_Signal(&buff1_lock);
			
		}
		
		offset += toRead;
		size -= toRead;
		bytes_read += toRead;
	}
	
	// return read operation status
	return bytes_read;
}


int iNode_write_at(iNode_t *node, const void* buff, uint32_t size, uint32_t offset) {
	// Allocate additional sectors if necessary
	if(node->iNode.size < offset) {
		// Allocate up to offset we need to write at
		if(!allocate_space(node, offset - node->iNode.size)) {
				return 0;
		}
	}
	if(node->iNode.size < offset + size) {
		// Allocate all the space we will need to write
		if(!allocate_space(node, size)) {
				return 0;
		}
	}
	
	uint32_t bytes_written = 0;
	while(size > 0) {
		uint32_t s = FilePos2Sector(node, offset);
		uint32_t sector_ofs = offset%BLOCK_SIZE;
	
		uint32_t space_left = min(BLOCK_SIZE - sector_ofs, iNode_size(node)-offset); // space left in sector (or iNode)
		uint32_t count = min(size, space_left);	// number of bytes to write
		
		if(sector_ofs == 0 && count == BLOCK_SIZE) {
			// Write whole block
			eDisk_WriteBlock(buff+bytes_written, s);
		}
		else {
			SVC_Wait(&buff1_lock);
			eDisk_ReadBlock(buff1, s);
			memcpy(buff1+sector_ofs, buff+bytes_written, count);
			eDisk_WriteBlock(buff1, s);
			SVC_Signal(&buff1_lock);
		}
		
		offset += count;
		bytes_written += count;
		size -= count;
	}
	
	return bytes_written;
}


uint32_t iNode_size(iNode_t *node) {
	return node->iNode.size;
}

void iNode_lock_read(iNode_t *node) {
	SVC_Wait(&node->NodeLock);
	node->numReaders++;
	SVC_Signal(&node->NodeLock);
}


void iNode_lock_write(iNode_t *node) {
	do {
		SVC_Wait(&node->NodeLock);
		if(node->numReaders == 0){
			break;
		}
		SVC_Signal(&node->NodeLock);
		SVC_Suspend();
	} while (1);
}

void iNode_unlock_read(iNode_t *node) {
	SVC_Wait(&node->NodeLock);
	node->numReaders--;
	SVC_Signal(&node->NodeLock);
}

void iNode_unlock_write(iNode_t *node) {
	SVC_Signal(&node->NodeLock);
}

// ----------------------------------- File Functions -------------------------------------- //

// TODO Replace with dynamic allocation instead of buffers and add return codes
int eFile_F_open(iNode_t *node, File_t* buff) {
	buff->iNode = node;
	buff->pos = 0;
	
	return 1;
}


int eFile_F_reopen(File_t *file, File_t* buff) {
	return eFile_F_open(iNode_reopen(file->iNode), buff);
}


int eFile_F_close(File_t *file) {
	return iNode_close(file->iNode);
}


iNode_t* eFile_F_get_iNode(File_t *file) {
	return file->iNode;
}

uint32_t eFile_F_read(File_t *file, void* buffer, uint32_t size) {
	iNode_lock_read(file->iNode);
	uint32_t r = iNode_read_at(file->iNode, buffer, size, file->pos);
	file->pos += size;
	iNode_unlock_read(file->iNode);
	return r;
}

uint32_t eFile_F_read_at(File_t *file, void* buffer, uint32_t size, uint32_t pos) {
	iNode_lock_read(file->iNode);
	uint32_t r = iNode_read_at(file->iNode, buffer, size, pos);
	iNode_unlock_read(file->iNode);
	return r;
}


uint32_t eFile_F_write(File_t *file, const void* buffer, uint32_t size) {
	iNode_lock_write(file->iNode);
	uint32_t r = iNode_write_at(file->iNode, buffer, size, file->pos);
	file->pos += size;
	iNode_unlock_write(file->iNode);
	return r;
}

uint32_t eFile_F_write_at(File_t *file, const void* buffer, uint32_t size, uint32_t pos) {
	iNode_lock_write(file->iNode);
	uint32_t r = iNode_write_at(file->iNode, buffer, size, pos);
	iNode_unlock_write(file->iNode);
	return r;
}

uint32_t eFile_F_length(File_t *file) {
	return file->iNode->iNode.size;
}
uint32_t eFile_F_seek(File_t *file, uint32_t pos) {
	file->pos = pos;
	return 0;
}

uint32_t eFile_F_tell(File_t *file) {
	return file->pos;
}

// ------------------------------ Directory Functions -------------------------------------- //

// TODO Add return code(s) and dynamic allocation instead of buffers
int eFile_D_create(uint32_t parent_sector, uint32_t dir_sector, uint32_t entry_cnt) {
	Dir_t buff;
	int r = 1;
	if(parent_sector == 0) {
		parent_sector = ROOTDIR_INODE; // Root dir sector
	}
	
	r &= iNode_create(dir_sector, entry_cnt * sizeof(DirEntry_t), 1);
	
	r &= eFile_D_open(iNode_open(dir_sector), &buff);
	
	r &=eFile_D_add(&buff, ".", dir_sector, 1);
	r &=eFile_D_add(&buff, "..", parent_sector, 1);
	
	r &=eFile_D_close(&buff);
	return r;
}

int eFile_D_open(iNode_t *node, Dir_t *buff) {
	if(node == 0 || buff == 0) {
		iNode_close(node);
		return 0;
	}
	buff->iNode = node;
	buff->pos = 2 * sizeof(DirEntry_t);
	return 1;
}

int eFile_D_open_root(Dir_t *buff) {
	return eFile_D_open(iNode_open(ROOTDIR_INODE), buff);
}

int eFile_D_reopen(Dir_t *dir, Dir_t* buff) {
	return eFile_D_open(iNode_reopen(dir->iNode), buff);
}

int eFile_D_close(Dir_t *dir) {
	return iNode_close(dir->iNode);
}

iNode_t* eFile_D_get_iNode(Dir_t *dir) {
	return dir->iNode;
}

int eFile_D_dir_from_path(const char path[], Dir_t *buff) {
	uint32_t i = 0;
	Dir_t dir;
	if(path[i] == '/') {
		// Absolute path, open root dir
		eFile_D_open_root(&dir);
		i++;
	}
	else {
		// Relative path, open current dir
		// Note: opening an empty string is treated as a relative access
		// i.e. open(".") will yield dir= "" and file="."
		// The dir is therefore a relative access to "."
		eFile_OpenCurrentDir(&dir);
	}
	
	char fn[MAX_FILE_NAME_LENGTH+1];
	for(uint32_t j = 0; ; i++) {
		fn[j++] = path[i];
		if((path[i] == '/') || (path[i] == 0)) {
			// We have found a complete dir name - change dir
			Dir_t newDir;
			if(!eFile_D_lookup(&dir, fn, &newDir)) {
				eFile_D_close(&dir);
				return 0;
			}
			
			eFile_D_close(&dir);
			dir = newDir;
			
			// Reset filename bufer
			memset(fn, 0, MAX_FILE_NAME_LENGTH+1);
			j = 0;
		}
		
		// Breka once we reach the end of the string
		if(path[i] == 0) {
			break;
		}
	}
	
	
	memcpy(buff, &dir, sizeof(dir));
	return 1;
}

int lookup(Dir_t *dir, const char name[], DirEntry_t *buff, uint32_t *offset) {
	DirEntry_t de;
	
	if(name[0] == 0) {
		// Empty strings will return the dir entry for . (which is always entry 0)
		iNode_lock_read(dir->iNode);
		iNode_read_at(dir->iNode, &de, sizeof de, 0);
		iNode_unlock_read(dir->iNode);
		memcpy(buff, &de, sizeof de);
		return 1;
	}
	
	iNode_lock_read(dir->iNode);
	for(uint32_t ofs = 0; ofs < dir->iNode->iNode.size; ofs += sizeof de) {
		iNode_read_at(dir->iNode, &de, sizeof de, ofs);
		if(strcmp(de.name, name) == 0) {
			*offset = ofs;
			memcpy(buff, &de, sizeof de);
			iNode_unlock_read(dir->iNode);
			return 1;
		}
	}
	
	iNode_unlock_read(dir->iNode);
	return 0;
}

int eFile_D_lookup_by_sector(Dir_t *dir, uint32_t sector, DirEntry_t *buff) {
	int ret = 0;
	
	iNode_lock_read(dir->iNode);
	DirEntry_t de;
	for(uint32_t ofs = 0; ofs < dir->iNode->iNode.size; ofs += sizeof de) {
		iNode_read_at(dir->iNode, &de, sizeof de, ofs);
		if(de.Header_Sector == sector) {
			ret = 1;
			memcpy(buff, &de, sizeof de);
		}
	}
	
	iNode_unlock_read(dir->iNode);
	return ret;
}

int eFile_D_lookup(Dir_t *dir, const char name[], File_t *buff) {
		
	DirEntry_t de;
	uint32_t ofs = 0;
	
	if (lookup(dir, name, &de, &ofs)) {
		if(de.isDir) {
			eFile_D_open(iNode_open(de.Header_Sector), buff);
			
		}
		else {
			eFile_F_open(iNode_open(de.Header_Sector), buff);
		}
		
		return 1;
	} 
	return 0;
}

int eFile_D_add(Dir_t *dir, const char name[], uint32_t iNode_header_sector, uint8_t isDir) {
	if(strlen(name) > MAX_FILE_NAME_LENGTH+1) {
		return 0;
	}
	
	DirEntry_t de;
	DirEntry_t buff;
	uint32_t ofs = 0;
	if(lookup(dir, name, &de, &ofs)) {
		return 0;
	}
	
	de.isDir = isDir;
	strcpy(de.name, name);
	de.Header_Sector = iNode_header_sector;
	de.in_use = 1;
	
	iNode_lock_write(dir->iNode);
	
	for (ofs = 0; ofs < dir->iNode->iNode.size; ofs += sizeof(DirEntry_t)) {
		iNode_read_at(dir->iNode, &buff, sizeof(DirEntry_t), ofs);
		if (!buff.in_use) {
			break;
		}
	}

	int i = iNode_write_at(dir->iNode, &de, sizeof(DirEntry_t), ofs);
	iNode_unlock_write(dir->iNode);
	
	return i == sizeof(de);
}

int eFile_D_remove(Dir_t *dir, const char name[]) {

	DirEntry_t de;
	uint32_t ofs;
	if(!lookup(dir, name, &de, &ofs)) {
		return 0;
	}
	
	iNode_t* node = iNode_open(dir->iNode->sector_num);
	
	de.in_use = 0;
	iNode_write_at(dir->iNode, &de, sizeof de, ofs);
	
	iNode_remove(node);
	iNode_close(node);
	return 1;
}

int eFile_D_read_next(Dir_t *dir, char buff[], uint32_t *sizeBuffer) {
	DirEntry_t de;
	
	while(dir->pos < dir->iNode->iNode.size) {
		iNode_read_at(dir->iNode, &de, sizeof(DirEntry_t), dir->pos);
		dir->pos += sizeof(DirEntry_t);
		if(de.in_use) {
			strcpy(buff, de.name);
			
			iNode_t *n = iNode_open(de.Header_Sector);
			*sizeBuffer = n->iNode.size;
			iNode_close(n);
			return 1;
		}
	}
	return 0;
}


// -------------------------------- Filesys Functions -------------------------------------- //
iNode_t* eFile_getCurrentDirNode(void) {
	
	if(RunPt->currentDir) {
		return iNode_reopen(RunPt->currentDir);
	}
	
	RunPt->currentDir = iNode_open(eFile_get_root_sector());
	iNode_open(eFile_get_root_sector()); // Open twice to fake that the thread had this open already
	// Note: This is OK because currentDir never gets fully closed until the thread dies, where it closes it
	// The only exception is when the thread never opens a directory, in which case this isnt called anyways
	return RunPt->currentDir;
}


int eFile_OpenCurrentDir(File_t *buff) {
	eFile_D_open(eFile_getCurrentDirNode(), buff);
	return 1;
}


// Note: This only accepts FILE paths - 
// passing a directory path to this will technically work, but won't open the last level unless proceeded by a '/'
int eFile_parse_filepath(const char path[], Dir_t* dirBuff, char **fn_buff) {
	int i;	
	for(i = strlen(path)-1; path[i] != '/' && i >= 0; --i) { }
	*fn_buff = (char *) path+i+1;
	
	SVC_Wait(&pathbuff_lock);
	memcpy(pathbuff, path, i+1);
	pathbuff[i+1] = 0;
	i = eFile_D_dir_from_path(pathbuff, dirBuff);
	
	return i;
}

int eFile_Create(const char path[]) { 
	int i;
	Dir_t d;
	char *fn;
	eFile_parse_filepath(path, &d, &fn);
	
	// Create a file of zero size
	uint32_t s = Bitmap_AllocOne();
	i = iNode_create(s, 128, 0);
	i &= eFile_D_add(&d, fn, s, 0);
	SVC_Signal(&pathbuff_lock);
	
	i &= eFile_D_close(&d);
	
	return i;
}

int eFile_CreateDir(const char path[]) { 
	int i;
	Dir_t d;
	char *fn;
	
	eFile_parse_filepath(path, &d, &fn);
	
	// Create a file of zero size
	uint32_t s = Bitmap_AllocOne();
	i = eFile_D_create(d.iNode->sector_num, s, 16);
	i &= eFile_D_add(&d, fn, s, 1);
	SVC_Signal(&pathbuff_lock);
	i &= eFile_D_close(&d);
	
	return i;
}

int eFile_CD(const char path[]) {
	Dir_t d;
	eFile_D_dir_from_path(path, &d);
	iNode_close(RunPt->currentDir);
	RunPt->currentDir = eFile_D_get_iNode(&d); // Don't need to reopen because dir_from_path already does
	return 1;
}

int eFile_Open(const char path[], File_t *buff) {
	int i;
	Dir_t d;
	char *fn;
	
	eFile_parse_filepath(path, &d, &fn);
	
	i = eFile_D_lookup(&d, fn, buff);
	SVC_Signal(&pathbuff_lock);
	i &= eFile_D_close(&d);
	return i;
}

int eFile_Remove(const char path[]) {
	int i;
	Dir_t d;
	char *fn;
	
	eFile_parse_filepath(path, &d, &fn);
	
	i = eFile_D_remove(&d, fn);
	SVC_Signal(&pathbuff_lock);
	i &= eFile_D_close(&d);
	return i;
}


/* Return success value */
int eFile_Init(void) {
	// Initialize the list of iNode(s)
		// TODO Update with dynamic memory allocation after lab 5
	SVC_InitSemaphore(&buff1_lock, 1);
	SVC_InitSemaphore(&buff2_lock, 1);
	SVC_InitSemaphore(&pathbuff_lock, 1);
	
	// Initialize Bitmap
	Bitmap_Init(BLOCK_SIZE);
	
	return 1;
}

int eFile_Format(void) {
	// Format drive
	int r = 1;
	for(uint32_t i = 0; i < NumSectors; i++) {
		r &= eDisk_WriteBlock(zeros, i);
	}
	
	// Reset bitmap
	Bitmap_Reset();
	
	// Create root dir
	r &= eFile_D_create(ROOTDIR_INODE, ROOTDIR_INODE, 16);
	
	Bitmap_Unmount();
	//return r;
	return 0;
}

uint32_t eFile_get_root_sector(void) {
	return ROOTDIR_INODE;
}

int eFile_Mount(void) {
	// Read in bitmap
	Bitmap_Mount();
	
	return 1;
}

int eFile_Unmount(void) {
	// Write out bitmap
	Bitmap_Unmount();
	
	// Could also close all iNodes if we wanted to but tbh that's on the callee
	return 1;
}	
