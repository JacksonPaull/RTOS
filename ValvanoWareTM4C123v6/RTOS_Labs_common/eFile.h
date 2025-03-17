/**
 * @file      eFile.h
 * @brief     high-level file system
 * @details   This file system sits on top of eDisk.
 * @version   V1.0
 * @author    Valvano
 * @copyright Copyright 2020 by Jonathan W. Valvano, valvano@mail.utexas.edu,
 * @warning   AS-IS
 * @note      For more information see  http://users.ece.utexas.edu/~valvano/
 * @date      Jan 12, 2020
 ******************************************************************************/

//Define as 0 for FAT, 1 for iNode

#ifndef EFILE_H
#define EFILE_H 0


#if EFILE_H
#include <stdint.h>
#include <stdlib.h>
#include "../RTOS_Lab4_FileSystem/Bitmap.h"
#include "../RTOS_Labs_common/OS.h"
#include "../RTOS_Lab4_FileSystem/iNode.h"

#define BLOCK_SIZE 512
#define MAX_FILE_NAME_LENGTH 33
#define DIR_ENTRY_LENGTH MAX_FILE_NAME_LENGH+7
#define MAX_NODES_OPEN 3

typedef struct File {
	iNode_t *iNode;
	uint32_t pos; // Cursor position
} File_t;

// Dirs are also files, but this makes the code more readable
#define Dir_t File_t

typedef struct DirEntry {
	uint32_t Header_Sector;
	char name[MAX_FILE_NAME_LENGTH+1]; // To ensure null termination
	uint8_t in_use;
	uint8_t isDir;
} DirEntry_t;

// ----------------------------------------------------------------- File Functions -------------------------------------------------------------------- //

// ******** eFile_F_open ************
// Open a new file from disk after its iNode is loaded into memory
// input:  iNode_t *node - Pointer to the header iNode of the file
//         File_t *buff  - Result buffer, where the opened file is created
// output: 1 on success, 0 on fail
int eFile_F_open(iNode_t *node, File_t *buff);

// ******** eFile_F_reopen ************
// Reopen an already open file
// input:  File_t *file  - Pointer to the file that will be reopened
//         File_t *buff  - Result buffer, where the opened file is created
// output: 1 on success, 0 on fail
int eFile_F_reopen(File_t *file, File_t *buff);

// ******** eFile_F_close ************
// Close a file. If this was the last opener of the file, it is freed from memory.
// If the file was also removed, it is freed from disk.
// input:  File_t *file - Pointer to the file to close
int eFile_F_close(File_t *file);

// ******** eFile_F_get_iNode ************
// Get the header iNode owned by an open file
// input:  File_t *file - file to inspect
// output: iNode_t* - Pointer to the iNode owned by file
iNode_t* eFile_F_get_iNode(File_t *file);

// ******** eFile_F_read ************
// Read from a file starting at its cursor
// input:  File_t *file - File being read 
//				 void* buffer - Output buffer to read into
//				uint32_t size - number of bytes to read
// output: 1 on success, 0 on fail
uint32_t eFile_F_read(File_t *file, void* buffer, uint32_t size);

// ******** eFile_F_read_at ************
// Read from a specific file position
// input:  File_t *file - File being read 
//				 void* buffer - Output buffer to read into
//				uint32_t size - number of bytes to read
//				 uint32_t pos - file position to start reading from
// output: 1 on success, 0 on fail
uint32_t eFile_F_read_at(File_t *file, void* buffer, uint32_t size, uint32_t pos);

// ******** eFile_F_write ************
// Write to a file starting at its cursor
// input:  File_t *file - File being written to 
//				 void* buffer - Output buffer to write from
//				uint32_t size - number of bytes to write
// output: 1 on success, 0 on fail
uint32_t eFile_F_write(File_t *file, const void* buffer, uint32_t size);

// ******** eFile_F_write_at ************
// Write to a specific file position
// input:  File_t *file - File being written to 
//				 void* buffer - Output buffer to write from
//				uint32_t size - number of bytes to write
//				 uint32_t pos - file position to start reading from
// output: 1 on success, 0 on fail
uint32_t eFile_F_write_at(File_t *file, const void* buffer, uint32_t size, uint32_t pos);

// ******** eFile_F_length ************
// Get the current size of a file
// input:  File_t *file - File being inspected
// output: size of the file in bytes
uint32_t eFile_F_length(File_t *file);

// ******** eFile_F_seek ************
// Set the cursor position of a file
// input:  File_t *file - File being inspected
// output: none
void eFile_F_seek(File_t *file, uint32_t pos);

// ******** eFile_F_tell ************
// Get the cursor position of a file
// input:  File_t *file - File being inspected
// output: Current position of the cursor in the file
uint32_t eFile_F_tell(File_t *file);

// ------------------------------------------------------------ Directory Functions -------------------------------------------------------------------- //

// ******** eFile_D_create ************
// Create a new directory in a pre-allocated iNode
// input:  uint32_t parent_sector - Sector of the parent directory
//					  uint32_t dir_sector - Header sector for the new directory
//						 uint32_t entry_cnt - Number of entries to initially allocate space for
// output: 1 on success, 0 on fail
int eFile_D_create(uint32_t parent_sector, uint32_t dir_sector, uint32_t entry_cnt);

// ******** eFile_D_open ************
// Open a directory from its header iNode
// input:  iNode_t *node - iNode to open from
//					 Dir_t *buff - Output buffer to open into
// output: 1 on success, 0 on fail
int eFile_D_open(iNode_t *node, Dir_t *buff);

// ******** eFile_D_open_root ************
// Open the root directory
// input:  Dir_t *buff - Output buffer to open intoa
// output: 1 on success, 0 on fail
int eFile_D_open_root(Dir_t *buff);

// ******** eFile_D_reopen ************
// Reopen a currently open directory
// Note: This function is called automatically if necessary
// input: Dir_t *dir - Directory to reopen
//				Dir_t *buff - Output buffer to open into
// output: 1 on success, 0 on fail
int eFile_D_reopen(Dir_t *dir, Dir_t* buff);

// ******** eFile_D_close ************
// Close a currently open directory, freeing it from memory and disk if possible
// input: Dir_t *dir - Directory to close
// output: 1 on success, 0 on fail
int eFile_D_close(Dir_t *dir);

// ******** eFile_D_get_iNode ************
// Get the header iNode for an open directory
// input: Dir_t *dir - Directory to inspect
// output: Pointer to the directory's header iNode
iNode_t* eFile_D_get_iNode(Dir_t *dir);

// ******** eFile_D_dir_from_path ************
// Open a directory based on a (relative or absolute) path to that directory
// input: const char path[] - relative or absolute path to directory
//							Dir_t *buff - output buffer
// output: 1 on success, 0 on fail
int eFile_D_dir_from_path(const char path[], Dir_t *buff);

// ******** eFile_D_lookup ************
// Find a directory entry within dir that matches the filename
// input: Dir_t *dir - Directory to search within
// const char name[] - Filename to match
//			File_t *buff - Output buffer
// output: 1 on success, 0 on fail
int eFile_D_lookup(Dir_t *dir, const char name[], File_t *buff);

// ******** eFile_D_lookup_by_sector ************
// Find a directory entry within dir that matches the sector
// input: Dir_t *dir - Directory to search within
// 	 uint32_t sector - Sector header to match
//			File_t *buff - Output buffer
// output: 1 on success, 0 on fail
int eFile_D_lookup_by_sector(Dir_t *dir, uint32_t sector, DirEntry_t *buff);

// ******** eFile_D_add ************
// Add a new entry into a directory
// input: 						Dir_t *dir - Directory to add to
//		 				 const char name[] - File name to add
// 	uint32_t iNode_header_sector - header sector of the new file
//								 uint8_t isDir - Whether the file is a directory or not
// output: 1 on success, 0 on fail
int eFile_D_add(Dir_t *dir, const char name[], uint32_t iNode_header_sector, uint8_t isDir);

// ******** eFile_D_remove ************
// Remove an entry from a directory
// input: 						Dir_t *dir - Directory to remove from
//		 				 const char name[] - File name to remove
// output: 1 on success, 0 on fail
int eFile_D_remove(Dir_t *dir, const char name[]);

// ******** eFile_D_read_next ************
// Read out the next entry from the directory, based on cursor position
// input: 			Dir_t *dir - Directory to read from
//				 		 char buff[] - Buffer for the filename
//		uint32_t *sizeBuffer - Buffer for the filesize
// output: 1 on success, 0 on fail
int eFile_D_read_next(Dir_t *dir, char buff[], uint32_t *sizeBuffer);


// ------------------------------------------------------------ Filesys Functions ------------------------------------------------------------ //


// ******** eFile_Create ************
// Create a new file at the specified path
// Note: All intermediate dirs will NOT be automatically created
// input: const char path[] - relative or absolute path to the file to create
// output: 1 on success, 0 on fail
int eFile_Create(const char path[]);

// ******** eFile_CreateDir ************
// Create a new directory at the specified path
// Note: All intermediate dirs will NOT be automatically created
// input: const char path[] - relative or absolute path to the directory to create
// output: 1 on success, 0 on fail
int eFile_CreateDir(const char path[]);

// ******** eFile_Open ************
// Open an existing file at a relative or absolute path
// input: const char path[] - relative or absolute path to the file to open
//						 File_t* buff - Output buffer for the opened file
// output: 1 on success, 0 on fail
int eFile_Open(const char path[], File_t *buff);

// ******** eFile_Remove ************
// Remove an existing file from disk
// input: const char path[] - Relative or absolute path to the file to remove
// output: 1 on success, 0 on fail
int eFile_Remove(const char path[]);

// ******** eFile_OpenCurrentDir ************
// Reopen and return the current directory
// input: Dir_t *buff - output buffer
// output: 1 on success, 0 on fail
int eFile_OpenCurrentDir(Dir_t *buff);

// ******** eFile_Init ************
// Initialize the filesystem
// Note: Does NOT initialize the disk. eDisk_Init should be called first
// input: none
// output: 1 on success, 0 on fail
int eFile_Init(void);

// ******** eFile_Format ************
// Erase the disk, reset the bitmap, and create the root directory
// input: none
// output: none
void eFile_Format(void);

// ******** eFile_Mount ************
// Load the filesystem from disk
// input: none
// output: 1 on success, 0 on fail
int eFile_Mount(void);

// ******** eFile_Unmount ************
// Write the filesystem to disk, prepare for ejection
// input: none
// output: 1 on success, 0 on fail
int eFile_Unmount(void);

// ******** eFile_get_root_sector ************
// Get the sector for the header of the root directory
// input: none
// output: sector number for the root directory header
uint32_t eFile_get_root_sector(void);

// ******** eFile_getCurrentDirNode ************
// Reopen and return the current directory iNode
// input: none
// output: Pointer to the root directory iNode
iNode_t* eFile_getCurrentDirNode(void);

// ******** eFile_CD ************
// Change this thread's current directory to a new one
// Note: A thread opens the root directory by default, if no other directory is open yet
// input: const char path[] - relative or absolute path to the new directory
// output: 1 on success, 0 on fail
int eFile_CD(const char path[]);

#else
// Basic FAT implementation

//---------- eFile_Init-----------------
// Activate the file system, without formating
// Input: none
// Output: 0 if successful and 1 on failure (already initialized)
int eFile_Init(void);

//---------- eFile_Format-----------------
// Erase all files, create blank directory, initialize free space manager
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Format(void);

//---------- eFile_Mount-----------------
// Mount the file system and load metadata
// Input: none
// Output: 0 if successful and 1 on failure (already initialized)
int eFile_Mount(void);

//---------- eFile_Create-----------------
// Create a new, empty file with one allocated block
// Input: file name is an ASCII string up to seven characters 
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Create( const char name[]);

//---------- eFile_WOpen-----------------
// Open the file, read into RAM last block
// Input: file name is an ASCII string up to seven characters
// Output: 0 if successful and 1 on failure (e.g., trouble reading from flash)
int eFile_WOpen( const char name[]);

//---------- eFile_Write-----------------
// Save at end of the open file
// Input: data to be saved
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Write( char data);

//---------- eFile_WClose-----------------
// Close the file, left disk in a state power can be removed
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WClose(void);

//---------- eFile_ROpen-----------------
// Open the file, read first block into RAM 
// Input: file name is an ASCII string up to seven characters
// Output: 0 if successful and 1 on failure (e.g., trouble reading from flash)
int eFile_ROpen( const char name[]);
 
//---------- eFile_ReadNext-----------------
// Retreive data from open file
// Input: none
// Output: return by reference data
//         0 if successful and 1 on failure (e.g., end of file)
int eFile_ReadNext( char *pt);

//---------- eFile_RClose-----------------
// Close the reading file
// Input: none
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_RClose(void);

//---------- eFile_Delete-----------------
// delete this file
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Delete( const char name[]);                        

//---------- eFile_DOpen-----------------
// Open a (sub)directory, read into RAM
// Input: directory name is an ASCII string up to seven characters
//        (empty/NULL for root directory)
// Output: 0 if successful and 1 on failure (e.g., trouble reading from flash)
int eFile_DOpen( const char name[]);
  
//---------- eFile_DirNext-----------------
// Retreive directory entry from open directory
// Input: none
// Output: return file name and size by reference
//         0 if successful and 1 on failure (e.g., end of file)
int eFile_DirNext( char *name[], unsigned long *size);

//---------- eFile_DClose-----------------
// Close the directory
// Input: none
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_DClose(void);

//---------- eFile_Unmount-----------------
// Unmount and deactivate the file system
// Input: none
// Output: 0 if successful and 1 on failure (not currently mounted)
int eFile_Unmount(void);


#endif

#endif
