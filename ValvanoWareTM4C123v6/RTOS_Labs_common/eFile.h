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
#include <stdlib.h>
#include "../RTOS_Lab4_FileSystem/Bitmap.h"
#include "../RTOS_Labs_common/OS.h"
#include "../RTOS_Lab4_FileSystem/iNode.h"

#define BLOCK_SIZE 512
#define MAX_FILE_NAME_LENGTH 33
#define DIR_ENTRY_LENGTH MAX_FILE_NAME_LENGH+7
#define MAX_NODES_OPEN 10

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

// ----------------------------------- File Functions -------------------------------------- //

int eFile_F_open(iNode_t *node, File_t *buff);
int eFile_F_reopen(File_t *file, File_t *buff);
int eFile_F_close(File_t *file);
iNode_t* eFile_F_get_iNode(File_t *file);
uint32_t eFile_F_read(File_t *file, void* buffer, uint32_t size);
uint32_t eFile_F_read_at(File_t *file, void* buffer, uint32_t size, uint32_t pos);
uint32_t eFile_F_write(File_t *file, const void* buffer, uint32_t size);
uint32_t eFile_F_write_at(File_t *file, const void* buffer, uint32_t size, uint32_t pos);
uint32_t eFile_F_length(File_t *file);
void eFile_F_seek(File_t *file, uint32_t pos);
uint32_t eFile_F_tell(File_t *file);

// ------------------------------ Directory Functions -------------------------------------- //

int eFile_D_create(uint32_t parent_sector, uint32_t dir_sector, uint32_t entry_cnt);
int eFile_D_open(iNode_t *node, Dir_t *buff);
int eFile_D_open_root(Dir_t *buff);
int eFile_D_reopen(Dir_t *dir, Dir_t* buff);
int eFile_D_close(Dir_t *dir);
iNode_t* eFile_D_get_iNode(Dir_t *dir);
int eFile_D_dir_from_path(const char path[], Dir_t *buff);
int eFile_D_lookup(Dir_t *dir, const char name[], File_t *buff);
int eFile_D_lookup_by_sector(Dir_t *dir, uint32_t sector, DirEntry_t *buff);
int eFile_D_add(Dir_t *dir, const char name[], uint32_t iNode_header_sector, uint8_t isDir);
int eFile_D_remove(Dir_t *dir, const char name[]);
int eFile_D_read_next(Dir_t *dir, char buff[], uint32_t *sizeBuffer);


// -------------------------------- Unified Functions -------------------------------------- //
// i.e. functions which will work for both directories and functions

// -------------------------------- Filesys Functions -------------------------------------- //
int eFile_getCurrentDir(File_t *buff);
int eFile_Create(const char path[]);
int eFile_CreateDir(const char path[]);
int eFile_Open(const char path[], File_t *buff);
int eFile_Remove(const char path[]);
int eFile_OpenCurrentDir(File_t *buff);

int eFile_Init(void);
void eFile_Format(void);
int eFile_Mount(void);
int eFile_Unmount(void);

uint32_t eFile_get_root_sector(void);
iNode_t* eFile_getCurrentDirNode(void);
int eFile_CD(const char path[]);

#endif
