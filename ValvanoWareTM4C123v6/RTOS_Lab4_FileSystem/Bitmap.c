

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

void Bitmap_Format(void) {
	uint8_t buf[BLOCK_SIZE];
	for(int i = 0; i < BitmapEnd; i++) {
		eDisk_WriteBlock(buf, i);
	}
	
}

void Bitmap_Write_Out(void) {
	eDisk_WriteBlock(BitmapBuf, loaded_sector);
}

void Bitmap_Read_In(uint32_t sector) {
	eDisk_ReadBlock(BitmapBuf, sector);
}

void Bitmap_Mount(void) {
	// TODO Support general bitmap
	Bitmap_Read_In(0);
}

void Bitmap_Init(uint32_t size) {
	// TODO Support bitmap with size bigger than 512
	if(size != BLOCK_SIZE) {
		printf("Bitmap size unsupported, crash ensuing\r\n");
	}
	
	BitmapEnd = (size+BLOCK_SIZE-1) / BLOCK_SIZE; // Effectively math.ceil(size/block_size)
	
	for(int i = 0; i < BLOCK_SIZE; i++) {
		BitmapBuf[i] = 0x00;
	}
}


uint32_t Bitmap_AllocOne(void) {
	// TODO Support general bitmap
	uint32_t i = cursor, j;
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


void Bitmap_AllocN(uint32_t *buf, uint32_t N) {
	for(uint32_t i = 0; i < N; i++) {
		buf[i] = Bitmap_AllocOne();
	}
	
}

uint32_t Bitmap_isFree(uint32_t idx) {
	// TODO support general bitmap
	return 1-Bitmap_isAllocd(idx);
}


uint32_t Bitmap_isAllocd(uint32_t idx) {
	// TODO support general bitmap
	return (BitmapBuf[idx/8] >> (idx % 8)) & 0x1;
}




