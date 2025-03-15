

#include "Bitmap.h"
#include <stdio.h>
#include "../RTOS_Labs_common/eFile.h"
#include "../RTOS_Labs_common/eDisk.h"


#define BITMAP_SECTORS 10
#define BITMAP_INIT_VALUE 0

#define BitmapStart 0
uint32_t BitmapEnd = 0;

uint32_t loaded_sector = 0;	// TODO Implement for large bitmaps
uint32_t cursor = 0;	// TODO implement cursor for more effecient searching 
uint8_t BitmapBuf[BLOCK_SIZE];


// ******** Bitmap_Reset ************
// Reset the bitmap buffer to that of a newly formatted drive
// input:  none
// output: none
void Bitmap_Reset(void) {
	loaded_sector = 0;
	cursor = 0;
	for(uint32_t i; i < BLOCK_SIZE; i++) {
		BitmapBuf[i] = 0x00;
	}
	// 0. - bitmap
	// 1. - root dir header
	BitmapBuf[0] = 0b00000011;
}


// ******** Bitmap_Write_Out ************
// Write the current bitmap buffer out to disk
// input:  none
// output: none
void Bitmap_Write_Out(void) {
	printf("Writing out bitmap!\r\n");
	eDisk_WriteBlock(BitmapBuf, loaded_sector);
}

// ******** Bitmap_Read_In ************
// Write a new bitmap buffer to disk
// input:  none
// output: none
void Bitmap_Read_In(uint32_t sector) {
	eDisk_ReadBlock(BitmapBuf, sector);
}

// ******** Bitmap_Mount ************
// Load the bitmap from disk
// input:  none
// output: none
void Bitmap_Mount(void) {
	// TODO Support general bitmap
	Bitmap_Read_In(0);
}


// ******** Bitmap_Init ************
// Initialize the multi-sector bitmap
// input:  uint32_t size - the size in bytes of the bitmap
// output: none
void Bitmap_Init(uint32_t size) {
	// TODO Support bitmap with size bigger than 512
	if(size != BLOCK_SIZE) {
		printf("Bitmap size unsupported, crash ensuing\r\n");
	}
	
	BitmapEnd = (size+BLOCK_SIZE-1) / BLOCK_SIZE; // Effectively math.ceil(size/block_size)
	
	Bitmap_Reset();
}



// ******** Bitmap_AllocOne ************
// Allocate one free sector from the bitmap
// input:  none
// output: 
//	uint32_t - The sector number on disk 
//							that has been allocated
uint32_t Bitmap_AllocOne(void) {
	// TODO Support general bitmap
	uint32_t i = cursor, j = 0;
	while(BitmapBuf[i] == 0xFF) {
		i = (i+1) % BLOCK_SIZE;
		
		// Didn't find any valid sectors
		if(i == cursor) {
			return -1;
		}
	}
	
	cursor = i;
	while((BitmapBuf[i] >> j) & 0x1) {
		j++;
	}
		
	BitmapBuf[i] |= 0x1 << j;
	
	return i*8 + j;
}


// ******** Bitmap_AllocN ************
// Allocate N free sectors from the bitmap
// input:  uint32_t *buf - result buffer
//				 uint32_t    N - number of sectors to allocate
// output: buffer contains N allocated sectors
void Bitmap_AllocN(uint32_t *buf, uint32_t N) {
	for(uint32_t i = 0; i < N; i++) {
		buf[i] = Bitmap_AllocOne();
	}
	
}


// ******** Bitmap_isFree ************
// Check whether a specific sector is free
// input:  uint32_t idx - the sector number to inspect
// output: 1 if free, 0 if allocated
uint32_t Bitmap_isFree(uint32_t idx) {
	// TODO support general bitmap
	return 1-Bitmap_isAllocd(idx);
}


// ******** Bitmap_isAllocd ************
// Check whether a specific sector is allocated
// input:  uint32_t idx - the sector number to inspect
// output: 0 if free, 1 if allocated
uint32_t Bitmap_isAllocd(uint32_t idx) {
	// TODO support general bitmap
	return (BitmapBuf[idx/8] >> (idx % 8)) & 0x1;
}


// ******** Bitmap_free ************
// Free a specific sector from the bitmap
// input:  uint32_t idx - the sector number to free
// output: none
void Bitmap_free(uint32_t idx) {
	BitmapBuf[idx/8] &= ~(0x1 << (idx % 8));
}



