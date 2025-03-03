// filename ************** eFile.c *****************************
// High-level routines to implement a solid-state disk 
// Students implement these functions in Lab 4
// Jonathan W. Valvano 1/12/20
#include <stdint.h>
#include <string.h>
#include "../RTOS_Labs_common/OS.h"
#include "../RTOS_Labs_common/eDisk.h"
#include "../RTOS_Labs_common/eFile.h"
#include <stdio.h>

//---------- eFile_ParsePath-----------------
// Parse a pathstring, open directory and return file name
// Input: Path string (null terminated)
//         e.g. "/absolute/file/path.txt" or "relative/file/path.txt"
//				dirBuf: buffer for the opened directory
//				filenameBuf: buffer for the name of the file
// Output: 0 if successful and 1 on failure (already initialized)
int eFile_ParsePath(char *path, File_t **dirBuf, char *filenameBuf) {
	// relative or absolute?
	
	// open/close directories until we get to the base
		// return 1 on any failed opens
	
	//set dirBuf and strcpy final file name
	
	return 1;
}


//---------- eFile_Init-----------------
// Activate the file system, without formating
// Input: none
// Output: 0 if successful and 1 on failure (already initialized)
int eFile_Init(void){ // initialize file system
 
	
	
  return 1;   // replace
}

//---------- eFile_Format-----------------
// Erase all files, create blank directory, initialize free space manager
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Format(void){ // erase disk, add format
	
	// Erase all sectors
	
	// Format metadata sector(s)
		// 0. Sector bitmap
		// 1. Root dir

  return 1;   // replace
}

//---------- eFile_Mount-----------------
// Mount the file system, without formating
// Input: none
// Output: 0 if successful and 1 on failure
int eFile_Mount(void){ // initialize file system

	
	
  return 1;   // replace
}


//---------- eFile_Create-----------------
// Create a new, empty file with one allocated block
// Input: file name is an ASCII string up to seven characters 
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Create( const char name[]){  // create new file, make it empty 
	// Parse path
		// Get filename, open directory it will be in
	
	// Allocate a header sector and a single data sector
		// return 1 on fail
	
	// set file size = 0
	// set isDir = 0
	// set first data sector pointer
	
	// in dirEntry
		// Set name
		// set deleted = 0
		// set header sector pointer
	
	// Close dir
  return 1;   // replace
}


//---------- eFile_WOpen-----------------
// Open the file, read into RAM last block
// Input: file name is an ASCII string up to seven characters
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WOpen( const char name[]){      // open a file for writing 
	// Parse path
	
	// wait on writer lock
	
	
  return 1;   // replace  
}

//---------- eFile_Write-----------------
// save at end of the open file
// Input: data to be saved
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Write( const char data){
	
		// write to file starting at <pos>
		
		// When we get to the end of the current sector (i.e. size % 512 == 0)
			// Write current sector to disk
			// Allocate a new sector

	
    return 1;   // replace
}

//---------- eFile_WClose-----------------
// close the file, left disk in a state power can be removed
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WClose(void){ // close the file for writing
  
	// write file's last sector out to disk
	
	// release writer lock
	
  return 1;   // replace
}


//---------- eFile_ROpen-----------------
// Open the file, read first block into RAM 
// Input: file name is an ASCII string up to seven characters
// Output: 0 if successful and 1 on failure (e.g., trouble read to flash)
int eFile_ROpen( const char name[]){      // open a file for reading 
	// Parse path
	
	
	// acquire writer lock if numReaders = 0
	// increment numReaders
	
  return 1;   // replace   
}
 
//---------- eFile_ReadNext-----------------
// retreive data from open file
// Input: none
// Output: return by reference data
//         0 if successful and 1 on failure (e.g., end of file)
int eFile_ReadNext( char *pt){       // get next byte 
  
  return 1;   // replace
}
    
//---------- eFile_RClose-----------------
// close the reading file
// Input: none
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_RClose(void){ // close the file for writing
  
	// decrement numReaders
	
	// release writer lock if numReaders == 0
	
  return 1;   // replace
}


//---------- eFile_Delete-----------------
// delete this file
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Delete( const char name[]){  // remove this file 
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
int eFile_DOpen( const char name[]){ // open directory
   // Parse path
	
	 // get dirEntry and read from disk
	
	// TODO Split open/close into read/write (anything which will create/delete a file will write)
  return 1;   // replace
}
  
//---------- eFile_DirNext-----------------
// Retreive directory entry from open directory
// Input: none
// Output: return file name and size by reference
//         0 if successful and 1 on failure (e.g., end of directory)
int eFile_DirNext( char *name[], unsigned long *size){  // get next entry 
  // Start at <pos>
	
	// Find next directory (if we hit the end of the file return null pointer and success)
	
	// get file name from dir entry and size from header iNode
	
  return 1;   // replace
}

//---------- eFile_DClose-----------------
// Close the directory
// Input: none
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_DClose(void){ // close the directory
   // release writer lock
	
	
  return 1;   // replace
}


//---------- eFile_Unmount-----------------
// Unmount and deactivate the file system
// Input: none
// Output: 0 if successful and 1 on failure (not currently mounted)
int eFile_Unmount(void){ 
   // set to power down
  return 1;   // replace
}
