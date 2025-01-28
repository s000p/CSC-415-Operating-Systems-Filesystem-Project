/**************************************************************
* Class::  CSC-415-03 Spring 2024
* Name:: Cole Douglas, Halia Tavares
* Student IDs:: 922128256, 921851070
* GitHub-Name:: cole-d
* Group-Name:: Team Taro
* Project:: Basic File System
*
* File:: directory.c
*
* Description:: 
*       file which holds all functions relating to 
*       directories.
**************************************************************/
#include <time.h>
#include "directory.h"
#include "fsLow.h"
#include "freeSpace.h"

DE * rootDirectory;

// Function to move directories into other directories
int fs_mvdir(char * src, char *dest){

    ppretdata * srcInfo = malloc(sizeof(ppretdata));
    ppretdata * destInfo = malloc(sizeof(ppretdata));

    if(parsePath(src, srcInfo)!=0){
        printf("FS_MVDIR: PARSEPATH FAILED\n");
        
        free(srcInfo);
        free(destInfo);

        srcInfo = NULL;
        destInfo = NULL;

        return -1;
    }

    if(parsePath(dest, destInfo)!=0){
        printf("FS_MVDIR: PARSEPATH FAILED\n");

        free(srcInfo);
        free(destInfo);

        srcInfo = NULL;
        destInfo = NULL;

        return -1;
    }

    DE * src_p =loadDir(&(srcInfo->parent[srcInfo->lastElementIndex]));
    DE * dest_p = loadDir(&destInfo->parent[destInfo->lastElementIndex]);

    int emptyIndex = findEmptyDEInDir(src_p);

    if(emptyIndex<1){
        printf("FS_MVDIR: COULDN'T FIND EMPTY DIR\n");

        free(srcInfo);
        free(destInfo);
        free(src_p);
        free(dest_p);


        srcInfo = NULL;
        destInfo = NULL;
        src_p = NULL;
        dest_p = NULL;

        return -1;
    }
    
    //reassigning the src dir to the dest dir
    strcpy(dest_p[emptyIndex].name, srcInfo->parent[srcInfo->lastElementIndex].name);
    dest_p[emptyIndex].location = srcInfo->parent[srcInfo->lastElementIndex].location;
    dest_p[emptyIndex].fileSize = srcInfo->parent[srcInfo->lastElementIndex].fileSize;
    dest_p[emptyIndex].isDir =  srcInfo->parent[srcInfo->lastElementIndex].isDir;
    dest_p[emptyIndex].date =  srcInfo->parent[srcInfo->lastElementIndex].date;

    //change src's parent to dest
    src_p[1].location = dest_p[0].location;

    int destSize = ((destInfo->parent[destInfo->lastElementIndex].fileSize)+VCB_P->BlockSize-1)/VCB_P->BlockSize;
    int srcSize = ((srcInfo->parent[srcInfo->lastElementIndex].fileSize)+VCB_P->BlockSize-1)/VCB_P->BlockSize;
    int parentSize = ((srcInfo->parent[0].fileSize) + VCB_P->BlockSize-1)/VCB_P->BlockSize;

    //setting the src dir's location in the parent to unused
    srcInfo->parent[srcInfo->lastElementIndex].location = -1;
    
    //writing src
    LBAwrite(src_p, srcSize, src_p[0].location);

    //writing dest
    LBAwrite(dest_p, destSize, destInfo->parent[destInfo->lastElementIndex].location);

    //updating parent of src
    LBAwrite(srcInfo->parent, parentSize, srcInfo->parent[0].location);


    free(srcInfo);
    free(destInfo);
    free(src_p);
    free(dest_p);


    srcInfo = NULL;
    destInfo = NULL;
    src_p = NULL;
    dest_p = NULL;

    return 0;
}

// Function to find a directory in the parent directory
int findInDir(DE* parent, char* token){

    int entryCount = parent[0].fileSize / sizeof(DE);

    for(int i =0; i < entryCount; i++){
        if(strcmp(parent[i].name, token)==0){
            return i;
        }
    }
    return -1;
}

// function to find an empty directory in the parent
int findEmptyDEInDir(DE* parent){

    int entryCount = parent[0].fileSize / sizeof(DE);

    for(int i =0; i < entryCount; i++){
        if(parent[i].location==-1){
            return i;
        }
    }
    return -1;
}

// Function to load a directory
DE *loadDir(DE* parentDir){

    int parentSize = parentDir->fileSize;
    int blockBytes = VCB_P->BlockSize;

    int blocks = ((parentSize+blockBytes-1) / blockBytes);
       
    DE* returnDir = malloc(blocks*blockBytes);

    if(returnDir==NULL){
        printf("LOADDIR: MALLOC FAILED\n");

        return NULL;
    }

    LBAread(returnDir, blocks, parentDir->location);

    return returnDir;
}

// Function to check if a Directory is a directory or file
// Returns 0 if it is a directory, returns 1 otherwise
int isDEaDir(DE* entry){
    if(entry->isDir == true){
        return 0;
    } else{
        return 1;
    }
}

// Creates a directory
DE* createDE(int numOfDE, uint64_t blockSize, DE* parent){

    // memorry needed in the size of bytes
    int memNeeded = numOfDE*sizeof(DE); 
    time_t seconds;

    // convering memory needed to blocks needed
    int blocksNeeded = (memNeeded+blockSize-1)/blockSize;

    DE *newDir = malloc(blocksNeeded*blockSize);
    if(newDir == NULL){
        printf("CREATEDE: ROOTDIR MALLOC FAILED\n");

        free(newDir);

        newDir = NULL;

        return NULL;
    }

    numOfDE = (blocksNeeded*blockSize)/(sizeof(DE));

    int sizeDE = sizeof(DE);

    for(int i=0; i<numOfDE;i++){
        strcpy(newDir[i].name,"");
        newDir[i].location= -1;
        newDir[i].isDir=false;
        newDir[i].fileSize=0;
        newDir[i].date = time(NULL);
    }
    
    int rdStart = allocFreeSpace(blocksNeeded);

    strcpy(newDir[0].name,".");
    newDir[0].isDir = true;
    newDir[0].location = rdStart;
    newDir[0].fileSize= numOfDE * sizeof(DE);
    newDir[0].date = time(&seconds);

    strcpy(newDir[1].name,"..");
    newDir[1].isDir = true;

    // special case for root dir
    if(parent == NULL){
        //set second de to ".."
        newDir[1].location = rdStart;
        newDir[1].fileSize= numOfDE * sizeof(DE);
        newDir[1].date = time(&seconds);
    }
    // general case assign parent to ..
    else{
        newDir[1].location = parent[0].location;
        newDir[1].fileSize = parent[0].fileSize;
        newDir[1].date = parent[0].date;
    }

    int size = sizeof(DE)/blockSize;

    // now write root dir -- 6 blocks from starting block
    // what was returned from fs alloc
    LBAwrite(newDir, blocksNeeded, rdStart);
    
    return newDir;
}

// Test Function to see direcotory information
int printDir(DE* parent){
    printf("PRINTING DIRECTORY: %p \n", parent);
    printf("NAME: %s \n", parent->name);
    printf("SIZE: %d \n", parent->fileSize );
    printf("LOCATION: %d \n", parent->location);
    printf("isDir: %d \n", parent->isDir);
    printf("DATE: %ld \n", parent->date);
    return 0;
}

// Test Function to see directory information
int printFour(DE* parent){
    printf("PRINTING 4 DIRS\n");
    for(int i =0; i<4; i++){
        // printf("PRINTING DIRECTORY: %p \n", parent[i]);
        printf("NAME: %s \n", parent[i].name);
        printf("SIZE: %d \n", parent[i].fileSize );
        printf("LOCATION: %d \n", parent[i].location);
        printf("isDir: %d \n", parent[i].isDir);
        printf("DATE: %ld \n", parent[i].date);
        printf("-----END OF DIR %d ----- \n", i);
    }
    return 0;
}

// Function to check if a Directory Entry is a file or Directory
// Returns 0 if it is a file, otherwise returns 1
int fs_isFile(char * filename){

    ppretdata * ppinfo = malloc(sizeof(ppretdata));

    if(parsePath(filename, ppinfo)!=0){
        printf("FS_ISFILE: PARSEPATH FAILED\n");

        free(ppinfo);

        ppinfo = NULL;

        return -1;
    }

    if(ppinfo->parent[ppinfo->lastElementIndex].isDir!=1){

        free(ppinfo);

        ppinfo = NULL;
        
        return 1;
    }

    free(ppinfo);

    ppinfo = NULL;

    return 0;
}	

// Function to check if a directory entry is a directory
// returns 1 if directory, otherwise returns 0
int fs_isDir(char * pathname){
    ppretdata * ppinfo = malloc(sizeof(ppretdata));

    if(parsePath(pathname, ppinfo)!=0){
        printf("FS_ISDIR: PARSEPATH FAILED\n");

        free(ppinfo);

        ppinfo = NULL;

        return -1;
    }

    // isDir perameter is set to 1 if directory
    if(ppinfo->parent[ppinfo->lastElementIndex].isDir==1){

        free(ppinfo);

        ppinfo = NULL;

        return 1;
    }

    free(ppinfo);

    ppinfo = NULL;

    return 0;
}		

// Functions to check if directory is empty
// Returns 0 if empty, otherwise returns -1
int isDEEmpty(DE* entry){

    // loading the directory entry that needs to be checked
    DE * dir = loadDir(entry);

    int entryCount = dir[0].fileSize / sizeof(DE);

    // looping through the directory entries
    for(int i=2; i < entryCount-3; i++){
        // location being -1 means it is unused 
        // so if the location is not -1 the de isn't empty
        if(dir[i].location!=-1){

            free(dir);

            dir = NULL;

            return -1;
        }
    }

    free(dir);

    dir = NULL;

    return 0;
}

// Function to remove a directory
int fs_rmdir(const char *pathname){

    ppretdata * ppinfo = malloc(sizeof(ppretdata));

    // saving pathname for later use
    char * path = strdup(pathname);
    
    // retrieving parent info from path
    if(parsePath(path, ppinfo)!=0){
        printf("FS_RMDIR: PARSE PATH FAILED\n");
        
        free(ppinfo);
        free(path);

        ppinfo = NULL;
        path = NULL;

        return -1;
    }

    // finding the index of directory that user wants to remove
    int index = findInDir(ppinfo->parent, ppinfo->lastElementName);

    // checking if the directory is a directory
    // if not a directory, cannot remove
    if(isDEaDir(&ppinfo->parent[index])!=0){
        printf("FS_RMDIR: NOT A DIRECTORY\n");

        free(ppinfo);
        free(path);

        ppinfo = NULL;
        path = NULL;        
        
        return -1;
    }

    // checking if directory is empty
    // if not empty, cannot remove
    if(isDEEmpty(&ppinfo->parent[index])!=0){
        printf("FS_RMDIR: CANNOT REMOVE. DIRECTORY NOT EMPTY\n");

        free(ppinfo);
        free(path);

        ppinfo = NULL;
        path = NULL;

        return -1;
    }

    // if index is 0 then the file isn't there
    if(index<0){
        printf("FS_RMDIR: FILE DOES NOT EXIST\n");

        free(ppinfo);
        free(path);

        ppinfo = NULL;
        path = NULL;

        return -1;
    }
 
    // shifting blocks to be marked as unused
    if(deallocFreeSpace(&ppinfo->parent[index])!=0){
        printf("FS_RMDIR: DEALLOC FREESPACE FAILED\n");

        free(ppinfo);
        free(path);

        ppinfo = NULL;
        path = NULL;

        return -1;
    }

    // calculating location of blocks and size so we can write
    int locationOfAllocatedBlocks = ppinfo->parent[0].location;
    int size = ((ppinfo->parent[0].fileSize)+VCB_P->BlockSize-1)/VCB_P->BlockSize; //is bytes or blocks??

    // marking location as -1 to indicate directory as unused
    // setting the directory information back to defaults
    strcpy(ppinfo->parent[index].name, "");
    ppinfo->parent[index].fileSize = 0;
    ppinfo->parent[index].isDir = 0;
    ppinfo->parent[index].location = -1;

    LBAwrite(ppinfo->parent, size, locationOfAllocatedBlocks);

    free(ppinfo);
    free(path);

    ppinfo = NULL;
    path = NULL;
    
    return 0;
}
