/**************************************************************
* Class::  CSC-415-03 Spring 2024
* Name:: Halia Tavares, Cole Douglas, Bhargava Kadiyala, Pablo Partida
* Student IDs:: 921851070, 922128256, 920736411, 921514214
* GitHub-Name:: cole-d
* Group-Name:: Team Taro
* Project:: Basic File System
*
* File:: fsInit.c
*
* Description:: Main driver for file system assignment.
*
* This file is where you will start and initialize your system
*
**************************************************************/
#include "fsInit.h"
#include "freeSpace.h"
#include "directory.h"

#include "fsLow.h"
#include "mfs.h"
#define SIGNATURE 0x43543fe594ca5435

uint64_t numOfBlock;
VCB* VCB_P;

// Function to initialize Filesystem
int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize){
    printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);

    VCB_P = malloc(blockSize);
    if(VCB_P == NULL){
        return -1;
    }
    numOfBlock = numberOfBlocks;
    if(LBAread(VCB_P,1,0) == -1){
        printf("INITFILESYSTEM: VCB FAILED\n");
        free(VCB_P);
        return -1;
    }

    if(VCB_P->signature != SIGNATURE){
        //volume is already initialized
        VCB_P->numBlocks = numberOfBlocks;
        VCB_P->signature = SIGNATURE;

        VCB_P->locationOfFreespace=initFreeSpace(numberOfBlocks, blockSize);

        rootDirectory  = createDE(DEFAULT_NUMOFDIRS, blockSize, NULL);

        VCB_P->locationOfRootDir = rootDirectory[0].location;
        VCB_P->rootdirectorySize = rootDirectory[0].fileSize;
        VCB_P->BlockSize = blockSize;
        LBAwrite(VCB_P, 1, 0);
    } 
    fsmap = loadFreeSpace();
    DE fakeDir;

    fakeDir.fileSize = VCB_P->rootdirectorySize;
    fakeDir.location = VCB_P->locationOfRootDir;

    loadedRD = loadDir(&fakeDir);
    rootDirectory = loadedRD;
    loadedCWD = loadedRD;

    struct fs_stat * info;

    strcpy(loadedCWDString,"/");
    fs_setcwd(strdup(loadedCWDString));
    
	return 0;
}
    
// Function to exit filesystem
void exitFileSystem (){
    printf ("System exiting\n");

    for(int i = 0; i<((loadedRD->fileSize)/(sizeof(DE)));i++){
        //free(rootDirectory[i]);
    }

    free(loadedRD);
    free(VCB_P);
	free(fsmap);
}
    
