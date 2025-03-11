

/*
Definitions for a bitmap intended to be used specifically with the FS.
because the bitmap can grow very large, and we do not have VM, the entire BM is not kept in RAM,
hence this implementation is not usable for anything except the filesys bitmap

*/



#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>


void Bitmap_Reset(void);
void Bitmap_Mount(void);
void Bitmap_Init(uint32_t size);	// Initialize bitmap to 0
uint32_t Bitmap_AllocOne(void);	// Find first available sector
void Bitmap_AllocN(uint32_t *buf, uint32_t N); // Allocate N sectors

uint32_t Bitmap_isFree(uint32_t idx);
uint32_t Bitmap_isAllocd(uint32_t idx);
void Bitmap_free(uint32_t idx);

void Bitmap_Write_Out(void);
void Bitmap_Read_In(uint32_t sector);


#endif