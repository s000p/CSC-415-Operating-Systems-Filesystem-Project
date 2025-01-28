/**************************************************************
* Class::  CSC-415-03 Spring 2024
* Name:: Cole Douglas, Halia Tavares
* Student IDs:: 922128256, 921851070
* GitHub-Name:: cole-d
* Group-Name:: Team Taro
* Project:: Basic File System
*
* File:: mfs.c
*
* Description:: 
*	This is a misc file which mainly consists of file functions
*   Also contains helper functions used by the file system 
*   functions.
*
**************************************************************/
#include "mfs.h"
#include "directory.h"
#include "freeSpace.h"
#include "fsLow.h"

// Function to give stats 
int fs_stat(const char *path, struct fs_stat *buf){

    ppretdata * ppinfo = malloc(sizeof(ppretdata));
    char * pathname = strdup(path);


    if(parsePath(pathname,ppinfo)!=0){
        printf("FS_STAT: PARSEPATH FAILED");

        free(ppinfo);
        free(pathname);

        ppinfo = NULL;
        pathname = NULL;

        return -1;
    }

    buf->st_size = ppinfo->parent[ppinfo->lastElementIndex].fileSize;
    buf->st_blksize = VCB_P->BlockSize;
    buf->st_blocks = (buf->st_size + VCB_P->BlockSize -1) / VCB_P->BlockSize;
    buf->st_accesstime = ppinfo->parent[ppinfo->lastElementIndex].date;
    buf->st_createtime = ppinfo->parent[ppinfo->lastElementIndex].date;
    buf->st_modtime = ppinfo->parent[ppinfo->lastElementIndex].date;

    free(ppinfo);
    free(pathname);

    ppinfo = NULL;
    pathname = NULL;


    return 0;
}

// Function to delete a file
int fs_delete(char* filename){

    ppretdata * ppinfo = malloc(sizeof(ppretdata));

    // if it is a not a file get out
    if(fs_isFile(filename)!=1){
        printf("FS_DELETE: NOT A FILE");

        free(ppinfo);

        ppinfo = NULL;

        return -1;
    }

    if(parsePath(filename, ppinfo)!=0){
        printf("FS_DELETE: PARSEPATH FAILED");
        
        free(ppinfo);

        ppinfo = NULL;

        return -1;
    }

    // getting the index of the file in the parent
    int index = findInDir(ppinfo->parent, filename);
    
    // shifting the blocks/bits to be marked as unused
    if(deallocFreeSpace(&ppinfo->parent[index])!=0){
        printf("FS_DELETE: IN FS DELETE DEALLOC FREESPACE FAILED");

        free(ppinfo);

        ppinfo = NULL;

        return -1;
    }

    // setting file info to null
    strcpy(ppinfo->parent[index].name, "");
    ppinfo->parent[index].fileSize = 0;
    ppinfo->parent[index].isDir = 0;
    ppinfo->parent[index].location = -1;
    ppinfo->parent[index].date = time(NULL);

    free(ppinfo);

    ppinfo = NULL;
    
    return 0;
}

// Function to create files
DE * fs_create(const char* filename){
    // int memNeeded;
    // memNeeded = numOfDE*sizeof(DE); //bytes
    time_t seconds;

    int blocksNeeded = ((sizeof(DE) + VCB_P->BlockSize -1 )/ VCB_P->BlockSize);
    DE *newDir = malloc(sizeof(DE));
    if(newDir == NULL){
        printf("FS_CREATE: ROOT DIRECTORY MALLOC FAILED\n");

        free(newDir);

        newDir = NULL;
        
        return NULL;
    }

    int sizeDE = sizeof(DE);

    strcpy(newDir->name,"");
    newDir->location= 0;
    newDir->isDir=false;
    newDir->fileSize=0;
    newDir->date = time(NULL);

    int size = ((sizeof(DE))+VCB_P->BlockSize-1)/VCB_P->BlockSize;
    
    return newDir;
}