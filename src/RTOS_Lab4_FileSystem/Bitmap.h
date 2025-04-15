

/*
Definitions for a bitmap intended to be used specifically with the FS.
because the bitmap can grow very large, and we do not have VM, the entire BM is not kept in RAM,
hence this implementation is not usable for anything except the filesys bitmap

*/



#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>

// ******** Bitmap_Reset ************
// Reset the bitmap buffer to that of a newly formatted drive
// input:  none
// output: none
void Bitmap_Reset(void);

// ******** Bitmap_Mount ************
// Load the bitmap from disk
// input:  none
// output: none
void Bitmap_Mount(void);

// ******** Bitmap_Init ************
// Initialize the multi-sector bitmap
// input:  uint32_t size - the size in bytes of the bitmap
// output: none
void Bitmap_Init(uint32_t size);	

// ******** Bitmap_AllocOne ************
// Allocate one free sector from the bitmap
// input:  none
// output: 
//	uint32_t - The sector number on disk 
//							that has been allocated
uint32_t Bitmap_AllocOne(void);	

// ******** Bitmap_AllocN ************
// Allocate N free sectors from the bitmap
// input:  uint32_t *buf - result buffer
//				 uint32_t    N - number of sectors to allocate
// output: buffer contains N allocated sectors
void Bitmap_AllocN(uint32_t *buf, uint32_t N); 

// ******** Bitmap_isFree ************
// Check whether a specific sector is free
// input:  uint32_t idx - the sector number to inspect
// output: 1 if free, 0 if allocated
uint32_t Bitmap_isFree(uint32_t idx);

// ******** Bitmap_isAllocd ************
// Check whether a specific sector is allocated
// input:  uint32_t idx - the sector number to inspect
// output: 0 if free, 1 if allocated
uint32_t Bitmap_isAllocd(uint32_t idx);

// ******** Bitmap_free ************
// Free a specific sector from the bitmap
// input:  uint32_t idx - the sector number to free
// output: none
void Bitmap_free(uint32_t idx);

// ******** Bitmap_Unmount ************
// Write the bitmap to disk
// input:  none
// output: none
void Bitmap_Unmount(void);
#endif