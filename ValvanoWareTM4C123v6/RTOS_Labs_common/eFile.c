// filename ************** eFile.c *****************************
// High-level routines to implement a solid-state disk 
// Students implement these functions in Lab 4
// Jonathan W. Valvano 1/12/20
#include <stdint.h>
#include <string.h>
#include "../RTOS_Labs_common/OS.h"
#include "../RTOS_Labs_common/eDisk.h"
#include "../RTOS_Labs_common/eFile.h"
#include "../RTOS_Lab3_RTOSpriority/PriorityQueue.h"
#include "../RTOS_Lab4_FileSystem/Bitmap.h"
#include <stdio.h>

uint32_t NumSectors = 4096;
uint32_t SectorSize = 512;


FileWrapper_t DISK_INODES[MAX_FILES_OPEN];
PrioQ_node_t *Open_Nodes_Head;
uint32_t RootDir_iNode = 1;	// TODO Make work with general bitmap

uint32_t FileIdxToSector(File_t *f, uint32_t idx) {
	uint8_t D[BLOCK_SIZE];
	if(idx < 124) {
		return f->iNodeDisk->iNode.DP[idx];
	}
	
	// Use SIP
	if(idx < 128 + 124) {
		uint32_t n = (idx-124);
		eDisk_ReadBlock(D, f->iNodeDisk->iNode.SIP);
		return D[n];
	}
	
	// Use DIP
	uint32_t n1 = (idx-124-128)/128;
	uint32_t n2 = (idx-124-128) % 128;
	eDisk_ReadBlock(D, f->iNodeDisk->iNode.DIP);
	eDisk_ReadBlock(D, D[n1]);
	return D[n2];
}



void eFile_LoadBlock(File_t *f, uint32_t idx) {
	
	// Use DP
	if(idx < 124) {
		eDisk_ReadBlock(f->DataBuf, f->iNodeDisk->iNode.DP[idx]);
		f->currently_open = idx;
		return;
	}
	
	// Use SIP
	if(idx < 128 + 124) {
		uint32_t n = (idx-124);
		eDisk_ReadBlock(f->DataBuf, f->iNodeDisk->iNode.SIP);
		uint32_t *D = (uint32_t *) f->DataBuf;
		eDisk_ReadBlock(f->DataBuf, D[n]);
		f->currently_open = idx;
		
		return;
	}
	
	// Use DIP
	uint32_t n1 = (idx-124-128)/128;
	uint32_t n2 = (idx-124-128) % 128;
	eDisk_ReadBlock(f->DataBuf, f->iNodeDisk->iNode.DIP);
	uint32_t *D = (uint32_t *) f->DataBuf;
	eDisk_ReadBlock(f->DataBuf, D[n1]);
	eDisk_ReadBlock(f->DataBuf, D[n2]);
	f->currently_open = idx;
	
	return;
}

void eFile_AddSector(FileWrapper_t *node) {
	uint32_t sector = Bitmap_AllocOne();
	uint32_t idx = node->iNode.size/512+1;
	
	// Use DP
	if(idx < 124) {
		node->iNode.DP[idx] = sector;
		eDisk_WriteBlock((const uint8_t *) &node->iNode, node->sector_num);
		return;
	}
	
	uint8_t buf[512];
	// Use SIP
	if(idx < 128 + 124) {
		uint32_t n = (idx-124);
		if(n == 0) {
			// Allocate the SIP
			uint32_t sector_SIP = Bitmap_AllocOne();
			node->iNode.SIP = sector_SIP;
			eDisk_WriteBlock((const uint8_t *) &node->iNode, node->sector_num);
		}
		
		eDisk_ReadBlock(buf, node->iNode.SIP);
		buf[n] = sector;
		eDisk_WriteBlock(buf, node->iNode.SIP);
		
		return;
	}
	
	// Use DIP
	uint32_t n1 = (idx-124-128)/128;
	if(n1 == 0) {
		// Allocate DIP1
		uint32_t sector_DIP = Bitmap_AllocOne();
		node->iNode.DIP = sector_DIP;
		eDisk_WriteBlock((const uint8_t *) &node->iNode, node->sector_num);
	}
	
	uint32_t n2 = (idx-124-128) % 128;
	eDisk_ReadBlock(buf, node->iNode.DIP);
	if(n2 == 0) {
		// Allocate DIP2
		uint32_t sector_DIP = Bitmap_AllocOne();
		buf[n1] = sector_DIP;
		eDisk_WriteBlock(buf, node->iNode.DIP);
	}
	
	eDisk_ReadBlock(buf, buf[n1]);
	buf[n2] = sector;
	eDisk_WriteBlock(buf, sector);
}

void eFile_ReOpen_iNode(FileWrapper_t *node) {
	// Reopen a node already in memory
	OS_Wait(&node->ReaderLock);
	node->numOpen += 1;
	OS_Signal(&node->ReaderLock);
}

void eFile_ReOpen_iNodeRead(FileWrapper_t *node) {
	// Reopen a node already in memory
	OS_Wait(&node->ReaderLock);
	if(node->numReaders == 0) {
		OS_Wait(&node->WriterLock);
	}
	node->numReaders++;
	node->numOpen++;
	OS_Signal(&node->ReaderLock);
}

void eFile_ReOpen_iNodeWrite(FileWrapper_t *node) {
	// Reopen a node already in memory
	OS_Wait(&node->ReaderLock);
	OS_Wait(&node->WriterLock);
	node->numOpen += 1;
	OS_Signal(&node->ReaderLock);
}


FileWrapper_t* eFile_Open_RootiNode(void) {
	return eFile_Open_iNode(RootDir_iNode);
}

// Reopen files if necessary
FileWrapper_t* eFile_Open_iNode(uint32_t sector_num) {
	FileWrapper_t *node;
	node = (FileWrapper_t *) PrioQ_find(&Open_Nodes_Head, sector_num);
	if(node) {
		eFile_ReOpen_iNode(node);
		return node;
	}
	
	for(int i = 0; i < MAX_FILES_OPEN; i++) {
		if(DISK_INODES[i].prev_ptr == 0 && DISK_INODES[i].next_ptr == 0) {
			
			// Found free iNode alloc
			node = &DISK_INODES[i];
			eDisk_ReadBlock((uint8_t *) &(node->iNode), sector_num);
			node->priority = sector_num;
			node->numReaders = 0;
			node->numOpen = 1;
			node->sector_num = sector_num;
			OS_InitSemaphore(&node->WriterLock, 1);
			OS_InitSemaphore(&node->ReaderLock, 1);
			
			PrioQ_insert(&Open_Nodes_Head, (PrioQ_node_t *) node);
			
			break;
		}
	}
	
	return node;
}


void eFile_Close_iNode(FileWrapper_t *iNode) {
	OS_Wait(&iNode->ReaderLock); // Ensure mutex on the node
	iNode->numOpen--;
	if(iNode->numOpen == 0) {
		// Write back to disk and remove from open list
		eDisk_WriteBlock((uint8_t *) &iNode->iNode, iNode->sector_num);
		PrioQ_remove(&Open_Nodes_Head, (PrioQ_node_t *) iNode);
	} else {
		OS_Signal(&iNode->ReaderLock);
	}
}


FileWrapper_t* DirLookup(File_t *dir, char *filename) {
	// pointer to matching iNode (reopens it as well)
	return 0;
}


//---------- eFile_ParsePath-----------------
// Parse a pathstring, open directory and return file name
// Input: Path string (null terminated)
//         e.g. "/absolute/file/path.txt" or "relative/file/path.txt"
//				dirBuf: buffer for the opened directory
//				filenameBuf: buffer for the name of the file
// Output: 0 if successful and 1 on failure (already initialized)
int eFile_ParsePath(const char *path, File_t *dirBuf, char *filenameBuf) {
	uint32_t i;
	FileWrapper_t* node;
	
	if(path[i] == '/') {
		// Absolute path - start from root dir
		node = eFile_Open_iNode(RootDir_iNode);
		eFile_DOpen_iNode(dirBuf, node);
		i++;
		dirBuf->iNodeDisk = node;
	} else {
		// Relative path - start from currently open directory
		node = OS_get_current_dir();
		dirBuf->iNodeDisk = node;
		eFile_ReOpen_iNode(node);
	}
	
	while(1) {
		
		// find next delimiter
		char dirName[MAX_FILE_NAME_LENGTH+1];
		uint32_t j;
		for(j = i; 1; j++) {
			if(path[j] == 0){
				j = -1;
				break;
			}
			
			if(path[j] == '/') {
				
				// [i:j) is the name of the directory we need to open
				memcpy(dirName, &path[i], j-i);
				node = DirLookup(dirBuf, dirName);
				eFile_DClose(dirBuf);
				eFile_DOpen_iNode(dirBuf, node);
				
				while(path[j] == '/') {j++;} // Consume all grouped ////
				break;
			}
		}
		
		// Reached the end of the string
		if(j == -1)
			break;
		
		// Keep going
		i = j;
	}
		
	// Set filename buffer and set current dir
	strcpy(filenameBuf, &path[i]);
	
	return 0;
}





//---------- eFile_Init-----------------
// Activate the file system, without formating
// Input: none
// Output: 0 if successful and 1 on failure (already initialized)
int eFile_Init(void){ // initialize file system
	int x, y;

	disk_ioctl(0, GET_SECTOR_COUNT, &x);
	printf("found %d blocks, using %d\r\n", x, NumSectors);
	
	disk_ioctl(0, GET_SECTOR_SIZE, &x);
	disk_ioctl(0, GET_BLOCK_SIZE, &y);
	printf("BlockSize: %d\r\nSectorSize: %d\r\n\n", y, x);
	
	// TODO Update to general bitmap
	Bitmap_Init(512);
	printf("Using %d sectors for the bitmap\r\n", 1);
	
  return 0;
}

int CreateRootDir(uint32_t header_sector) {
	uint8_t DataBuf[BLOCK_SIZE];
	FileWrapper_t dir;
	
	dir.iNode.isDir = 1;
	dir.iNode.magicHW = 0x3456;
	dir.iNode.magicByte = 0x12;
	uint32_t sec = Bitmap_AllocOne();
	dir.iNode.DP[0] = sec;
	dir.iNode.size = 2;
	
	// Add first two entries
	DirEntry_t entry = { 0, ".", header_sector };
	memcpy(DataBuf, &entry, sizeof(entry));
	
	strcpy(entry.name, "..");
	memcpy(DataBuf + sizeof(entry), &entry, sizeof(entry));
	
	eDisk_WriteBlock((uint8_t*) &dir.iNode, header_sector);
	eDisk_WriteBlock(DataBuf, sec);
	
	return 0;
}

//---------- eFile_Format-----------------
// Erase all files, create blank directory, initialize free space manager
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Format(void){ // erase disk, add format
	// Erase all sectors
	uint8_t buf[BLOCK_SIZE];
	
	printf("Erasing sectors...\r\n");
	for(int i = 0; i < NumSectors; i++) {
		eDisk_WriteBlock(buf, i);
	}
	
	printf("Allocating reserved sectors\r\n");
	
	// TODO Replace with a more general bitmap initialization
	// Initialize first sectors
		// (todo, metadata sector?)
		// 0. Bitmap
		// 1. Root Dir iNode
		// 2. Root Dir First DataNode
	Bitmap_Format();
	Bitmap_AllocN((uint32_t *) buf, 2);// Allocate first 2 sectors (third in CreateRootDir)
	CreateRootDir(1);

  return 0;  
}

//---------- eFile_Mount-----------------
// Mount the file system, without formating
// Input: none
// Output: 0 if successful and 1 on failure
int eFile_Mount(void){ // initialize file system
	// Load bitmap
	Bitmap_Mount();
	
	//Note: Don't need to do anything else, the root dir will be loaded the first time an absolute path is used
  return 0;  
}

int eFile_DirCreateEntry(File_t *dir, const char name[], uint32_t header_iNode) {
	DirEntry_t fde;
	strcpy(fde.name, name);
	fde.deleted = 0;
	fde.header_inode = header_iNode;
	
	eFile_ReOpen_iNode(dir->iNodeDisk);
	for(uint32_t i = 0; i < dir->iNodeDisk->iNode.size; i++) {
		// Insert FDE entry
		
		// Find first entry with "deleted" = 1 or "iNode" = 0
		
	}
	
	return 0;
}

void eFile_DirDeleteEntry(File_t *dir, FileWrapper_t *node) {
	// Find the right node
	
	// set deleted flag
	
	
}

//---------- eFile_Create-----------------
// Create a new, empty file with one allocated block
// Input: file name is an ASCII string up to seven characters 
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Create( const char name[], uint8_t isDir){  // create new file, make it empty 
	// Parse path
		// Get filename, open directory it will be in
	File_t dir;
	char fn_buff[MAX_FILE_NAME_LENGTH];
	eFile_ParsePath(name, &dir, fn_buff);
	
	// Allocate sector for iNode and first data sector
	uint32_t iNode_s = Bitmap_AllocOne();
	uint32_t data_s = Bitmap_AllocOne();
	
	FileWrapper_t *node = eFile_Open_iNode(iNode_s);
	if(isDir) {
		// Directory creation
		
		// Set first two entries for '.' and '..'
		
		node->iNode.size = 2;
		node->iNode.isDir = 1;
		
		eDisk_WriteBlock(dir.DataBuf, data_s);
	} else {
		// File Creation
		node->iNode.size = 0;
		node->iNode.isDir = 0;
	}
	
	node->iNode.magicByte = 0x12;
	node->iNode.magicHW = 0x3456;
	node->iNode.DP[0] = data_s;
	
	// Create the directory entry
	eFile_DirCreateEntry(&dir, fn_buff, iNode_s);
	eDisk_WriteBlock(dir.DataBuf, dir.iNodeDisk->sector_num);
	
	// Close dir and iNode
	eFile_Close_iNode(node);
	eFile_DClose(&dir);
  return 0;   // replace
}


// Same as above but doesn't close the file and has a buffer for opening it
int eFile_CreateAndOpen(const char name[], uint8_t isDir,  File_t *buff) {
	// Parse path
		// Get filename, open directory it will be in
	File_t dir;
	char fn_buff[MAX_FILE_NAME_LENGTH];
	eFile_ParsePath(name, &dir, fn_buff);
	
	// Allocate sector for iNode and first data sector
	uint32_t iNode_s = Bitmap_AllocOne();
	uint32_t data_s = Bitmap_AllocOne();
	
	FileWrapper_t *node = eFile_Open_iNode(iNode_s);
	if(isDir) {
		// Directory creation
		
		// Set first two entries for '.' and '..'
		
		node->iNode.size = 2;
		node->iNode.isDir = 1;
		
		eDisk_WriteBlock(dir.DataBuf, data_s);
	} else {
		// File Creation
		node->iNode.size = 0;
		node->iNode.isDir = 0;
	}
	
	node->iNode.magicByte = 0x12;
	node->iNode.magicHW = 0x3456;
	node->iNode.DP[0] = data_s;
	
	// Create the directory entry
	eFile_DirCreateEntry(&dir, fn_buff, iNode_s);
	eDisk_WriteBlock(dir.DataBuf, dir.iNodeDisk->sector_num);
	
	buff->currently_open = 0;
	buff->iNodeDisk = node;
	buff->pos = 0;
	
	eFile_DClose(&dir);
	return 0;
}


//---------- eFile_WOpen-----------------
// Open the file, read into RAM last block
// Input: file name is an ASCII string up to seven characters
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WOpen(const char name[], File_t* buff){      // open a file for writing 
	// Parse path
	char fnbuff[MAX_FILE_NAME_LENGTH];
	eFile_ParsePath(name, buff, fnbuff);
	
	// Dir lookup for the file
	FileWrapper_t *f = DirLookup(buff, fnbuff);
	
	// wait on writer lock
	OS_Wait(&f->ReaderLock);
	f->numOpen++;
	OS_Wait(&f->WriterLock);
	
	buff->iNodeDisk = f;
	buff->pos = 0;
	buff->currently_open = 0;
	uint32_t sector = FileIdxToSector(buff, 0);
	eDisk_ReadBlock(buff->DataBuf, sector);
	
  return 0; 
}

//---------- eFile_Write-----------------
// save at end of the open file
// Input: data to be saved
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Write(File_t *f, const char data){
	
		// write to file starting at <pos>

		// When we get to the end of the current max sector (i.e. size % 512 == 0)
			// Write current sector to disk
			// Allocate a new sector
		//Crossed boundary into next sector, swap buffer
  if(f->pos % 512 == 0 && f->pos != 0) {
		uint32_t idx = FileIdxToSector(f, f->currently_open);
		eDisk_WriteBlock(f->DataBuf, idx);
		
		eFile_AddSector(f->iNodeDisk);
		f->currently_open++;
		idx = FileIdxToSector(f, f->currently_open);
		eDisk_ReadBlock(f->DataBuf, idx);
	}
	
	f->DataBuf[f->pos % 512] = data;
	f->pos++;
	f->iNodeDisk->iNode.size++;
  return 0; 
}

//---------- eFile_WClose-----------------
// close the file, left disk in a state power can be removed
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WClose(File_t *f){ // close the file for writing
	// write file's last sector out to disk
	uint32_t sector = FileIdxToSector(f, f->currently_open);
	eDisk_WriteBlock(f->DataBuf, sector);
	
	// release writer lock
	OS_Signal(&f->iNodeDisk->WriterLock);
	int I = StartCritical();
	f->iNodeDisk->numOpen--;
	if(f->iNodeDisk->numOpen == 0) {
		PrioQ_remove((PrioQ_node_t **) &DISK_INODES, (PrioQ_node_t *) f->iNodeDisk);
	}
	EndCritical(I);
	OS_Signal(&f->iNodeDisk->ReaderLock);

  return 0; 
}


//---------- eFile_ROpen-----------------
// Open the file, read first block into RAM 
// Input: file name is an ASCII string up to seven characters
// Output: 0 if successful and 1 on failure (e.g., trouble read to flash)
int eFile_ROpen(const char name[], File_t *buff){      // open a file for reading 
	// Parse path
	char fnbuff[MAX_FILE_NAME_LENGTH];
	eFile_ParsePath(name, buff, fnbuff);
	
	// Dir lookup for the file
	FileWrapper_t *f = DirLookup(buff, fnbuff);
	
	int I = StartCritical();
	f->numOpen++;
	EndCritical(I);
	
	// wait on writer lock
	OS_Wait(&f->ReaderLock);
	f->numReaders++;
	if(f->numReaders == 1) {
		OS_Wait(&f->WriterLock);
	}
	OS_Signal(&f->ReaderLock);
	
	buff->iNodeDisk = f;
	buff->pos = 0;
	buff->currently_open = 0;
	uint32_t sector = FileIdxToSector(buff, 0);
	eDisk_ReadBlock(buff->DataBuf, sector);
	
  return 0; 
}
 
//---------- eFile_ReadNext-----------------
// retreive data from open file
// Input: none
// Output: return by reference data
//         0 if successful and 1 on failure (e.g., end of file)
int eFile_ReadNext(File_t *f, char *pt){       // get next byte 
	if(f->pos > f->iNodeDisk->iNode.size) {
		return 1;
	}
	
	//Crossed boundary into next sector, swap buffer
  if(f->pos % 512 == 0 && f->pos != 0) {
		f->currently_open++;
		uint32_t sec = FileIdxToSector(f, f->currently_open);
		eDisk_ReadBlock(f->DataBuf, sec);
	}
	
	*pt = f->DataBuf[f->pos % 512];
	f->pos++;
  return 0;   // replace
}
    
//---------- eFile_RClose-----------------
// close the reading file
// Input: none
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_RClose(File_t *f){ // close the file for writing
  
	// decrement numReaders
	int I = StartCritical();
	f->iNodeDisk->numReaders--;
	EndCritical(I);
	
	OS_Wait(&f->iNodeDisk->ReaderLock);
	
	f->iNodeDisk->numOpen--;
	if(f->iNodeDisk->numReaders == 0) {
		// release writer lock
		OS_Signal(&f->iNodeDisk->WriterLock);
	}
	
	I = StartCritical();
	if(f->iNodeDisk->numOpen == 0) {
		// Remove from disk
		PrioQ_remove((PrioQ_node_t **) &DISK_INODES, (PrioQ_node_t *) f->iNodeDisk);
	}
	EndCritical(I);
	
	OS_Signal(&f->iNodeDisk->ReaderLock);
	
  return 0;  
}


//---------- eFile_Delete-----------------
// delete this file
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Delete(File_t *f){  // remove this file 
	// Parse path, open directory
	
	// set deleted flag in directory entry
	
	// close directory
  return 1;   // replace
}                             



//---------- eFile_DOpen-----------------
// Open a (sub)directory, read into RAM
// Input: directory name is an ASCII string up to seven characters
//        (empty/NULL for root directory)
// Output: 0 if successful and 1 on failure (e.g., trouble reading from flash)
int eFile_DOpen(const char name[], File_t *buff){ // open directory
   // Parse path
	
	 // get dirEntry and read from disk
	
	// TODO Split open/close into read/write (anything which will create/delete a file will write)
  return 1;   // replace
}


int eFile_DOpen_iNode(File_t* buff, FileWrapper_t *iNode) {
	buff->pos = 0;
	buff->currently_open = 0;
	
	buff->iNodeDisk = iNode;
	eFile_ReOpen_iNode(iNode);
	
	return 0;
}

void eFile_cd(File_t *newDir) {
	OS_set_current_dir(newDir->iNodeDisk);
}
  
//---------- eFile_DirNext-----------------
// Retreive directory entry from open directory
// Input: none
// Output: return file name and size by reference
//         0 if successful and 1 on failure (e.g., end of directory)
int eFile_DirNext(File_t *dir, char *name[], unsigned long *size){  // get next entry 
  // Start at <pos>
	
	// Find next directory (if we hit the end of the file return null pointer and success)
	
	// get file name from dir entry and size from header iNode
	
  return 1;   // replace
}

//---------- eFile_DClose-----------------
// Close the directory
// Input: none
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_DClose(File_t *f){ // close the directory
   // release writer lock
	
	
  return 1;   // replace
}


//---------- eFile_Unmount-----------------
// Unmount and deactivate the file system
// Input: none
// Output: 0 if successful and 1 on failure (not currently mounted)
int eFile_Unmount(void){ 
   // set to power down
	
	// write out bitmap, any other iNodes still left in memory
	Bitmap_Write_Out();
	
	// send command to unmount
  return 1;   // replace
}
