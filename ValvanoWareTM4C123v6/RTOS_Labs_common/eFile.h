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

#ifndef EFILE_H
#define EFILE_H

#include <stdint.h>
#include "../RTOS_Lab4_FileSystem/Bitmap.h"
#include "../RTOS_Labs_common/OS.h"


// 64 bytes per entry --> 58 length file names + extension
// This also means there are 8 entries per sector --> max files/dirs in a dir is 124*8 + 128*8 + 128*128*8 = 133,000 files
#define DIR_ENTRY_LEN 64
#define MAX_FILE_NAME_LENGTH 58
#define MAX_FILES_OPEN 25
#define BLOCK_SIZE 512


// Note - iNode's must be exactly BLOCKSIZE in length

typedef struct iNode {
	uint8_t isDir;
	uint32_t size;
	uint32_t DP[124];
	uint32_t SIP;
	uint32_t DIP;
	uint16_t magicHW;
	uint8_t magicByte;
} iNode_t;

typedef struct FileWrapper {
	struct PrioQ_Node *next_ptr, *prev_ptr;
	uint32_t priority;
	iNode_t iNode;
	uint32_t sector_num;
	uint8_t numOpen;
	uint8_t numReaders;
	Sema4Type ReaderLock;
	Sema4Type WriterLock;
	
} FileWrapper_t;

typedef struct File {
	FileWrapper_t *iNodeDisk;
	uint32_t pos; // Cursor position
	uint8_t DataBuf[512];	// Data buffer
	uint32_t currently_open; // Currently open sector
} File_t;

//typedef struct Dir {
//	FileWrapper_t *f;
//	uint32_t pos;
//} Dir_t;

typedef struct DirEntry {
	uint8_t deleted;
	char name[MAX_FILE_NAME_LENGTH+1]; // To ensure null termination
	uint32_t header_inode;
} DirEntry_t;


FileWrapper_t* eFile_Open_iNode(uint32_t sector_num);
FileWrapper_t* eFile_Open_RootiNode(void);
void eFile_cd(File_t *newDir);
int eFile_DOpen_iNode(File_t *buf, FileWrapper_t *iNode);

/**
 * @details This function must be called first, before calling any of the other eFile functions
 * @param  none
 * @return 0 if successful and 1 on failure (already initialized)
 * @brief  Activate the file system, without formating
 */
int eFile_Init(void); // initialize file system

/**
 * @details Erase all files, create blank directory, initialize free space manager
 * @param  none
 * @return 0 if successful and 1 on failure (e.g., trouble writing to flash)
 * @brief  Format the disk
 */
int eFile_Format(void); // erase disk, add format

/**
 * @details Mount disk and load file system metadata information
 * @param  none
 * @return 0 if successful and 1 on failure (e.g., already mounted)
 * @brief  Mount the disk
 */
int eFile_Mount(void); // mount disk and file system

/**
 * @details Create a new, empty file with one allocated block
 * @param  name file name is an ASCII string up to seven characters
 * @return 0 if successful and 1 on failure (e.g., already exists)
 * @brief  Create a new file
 */
int eFile_Create(const char name[], uint8_t isDir);  // create new file, make it empty 

/**
 * @details Open the file for writing, read into RAM last block
 * @param  name file name is an ASCII string up to seven characters
 * @return 0 if successful and 1 on failure (e.g., trouble reading from flash)
 * @brief  Open an existing file for writing
 */
int eFile_WOpen(const char name[], File_t *buf);      // open a file for writing 

/**
 * @details Save one byte at end of the open file
 * @param  data byte to be saved on the disk
 * @return 0 if successful and 1 on failure (e.g., trouble writing to flash)
 * @brief  Format the disk
 */
int eFile_Write(File_t *f, const char data);  

/**
 * @details Close the file, leave disk in a state power can be removed.
 * This function will flush all RAM buffers to the disk.
 * @param  none
 * @return 0 if successful and 1 on failure (e.g., trouble writing to flash)
 * @brief  Close the file that was being written
 */
int eFile_WClose(File_t *f); // close the file for writing

/**
 * @details Open the file for reading, read first block into RAM
 * @param  name file name is an ASCII string up to seven characters
 * @return 0 if successful and 1 on failure (e.g., trouble reading from flash)
 * @brief  Open an existing file for reading
 */
int eFile_ROpen(const char name[], File_t *buf);      // open a file for reading 
   
/**
 * @details Read one byte from disk into RAM
 * @param  pt call by reference pointer to place to save data
 * @return 0 if successful and 1 on failure (e.g., trouble reading from flash)
 * @brief  Retreive data from open file
 */
int eFile_ReadNext(File_t *f, char *pt);       // get next byte 
                              
/**
 * @details Close the file, leave disk in a state power can be removed.
 * @param  none
 * @return 0 if successful and 1 on failure (e.g., wasn't open)
 * @brief  Close the file that was being read
 */
int eFile_RClose(File_t *f); // close the file for writing

/**
 * @details Delete the file with this name, recover blocks so they can be used by another file
 * @param  name file name is an ASCII string up to seven characters
 * @return 0 if successful and 1 on failure (e.g., file doesn't exist)
 * @brief  delete this file
 */
int eFile_Delete(File_t *f);  // remove this file 

/**
 * @details Open a (sub)directory, read into RAM
 * @param directory name is an ASCII string up to seven characters
 * if subdirectories are supported (optional, empty sring for root directory)
 * @return 0 if successful and 1 on failure (e.g., trouble reading from flash)
 */
int eFile_DOpen(const char name[], File_t *buf);
	
/**
 * @details Retreive directory entry from open directory
 * @param pointers to return file name and size by reference
 * @return 0 if successful and 1 on failure (e.g., end of directory)
 */
int eFile_DirNext(File_t *dir, char *name[], unsigned long *size);

/**
 * @details Close the directory
 * @param none
 * @return 0 if successful and 1 on failure (e.g., wasn't open)
 */
int eFile_DClose(File_t *dir);

/**
 * @details Unmount and deactivate the file system.
 * @param  none
 * @return 0 if successful and 1 on failure (e.g., trouble writing to flash)
 * @brief  Unmount the disk
 */
int eFile_Unmount(void); 

#endif
