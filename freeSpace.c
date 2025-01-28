/**************************************************************
* Class::  CSC-415-03 Spring 2024
* Name:: Halia Tavares, Cole Douglas, Bhargava Kadiyala, Pablo Partida
* Student IDs:: 921851070, 922128256, 920736411, 921514214
* GitHub-Name:: cole-d
* Group-Name:: Team Taro
* Project:: Basic File System
*
* File:: freeSpace.c
*
* Description:: 
*	This file contains functions to manage freespace
*
**************************************************************/
#include "fsInit.h"
#include "freeSpace.h"
#include "fsLow.h"
#include "b_io.h"

u_int8_t * fsmap; // Global variable declaration for fsmap

// Function to initialize the freespace map
// Free space uses a bit map where bit 0 means unused and 1 means used
int initFreeSpace(uint64_t numberOfBlocks, uint64_t blockSize){

	int bytesNeeded = (numberOfBlocks + 8-1) /8;
	int blocksNeeded =(bytesNeeded+blockSize-1)/blockSize;
	    
	fsmap = malloc((blocksNeeded*blockSize));  
	
	// 1 is used 0 is unused
    // setting first 6 as used bits
    for(int i =0; i<6;i++){
		setBit(i); 
	}

    // ting rest of bits as free
    for(int h=6;h<numberOfBlocks;h++){
        clearBit(h);
    }
    LBAwrite(fsmap,blocksNeeded,1); 

    return 1;
}

// Function to load freespace
u_int8_t *loadFreeSpace(){

    int bytesNeeded = (VCB_P->numBlocks + 8 - 1)/8;
    int blocksNeeded = (VCB_P->BlockSize + bytesNeeded -1 )/VCB_P->BlockSize;

    u_int8_t* returnMap = malloc(blocksNeeded * VCB_P->BlockSize);

    LBAread(returnMap, blocksNeeded, VCB_P->locationOfFreespace);

    return returnMap;
}

// Function to grow freespace
int growFreeSpace(int blocksRequested,int locationToStartAllocating, int locationOfFile){
    int locationOfAllocatedBlocks= locationToStartAllocating;
    int blocksInARow = 0;
    int totalBlocks = VCB_P->numBlocks;
    int blockWasHit = 0;
    int fileSize = locationToStartAllocating - locationOfFile;

    for(int i = locationOfAllocatedBlocks; i<totalBlocks; i++){
        // the bit is free

        if(getBitZeroOrOne(i)==0){
            // if there are not any blocks grouped yet
            if(blocksInARow==0){ 
                locationOfAllocatedBlocks=i;
            }

            if(blockWasHit == 1){
                blocksInARow= blocksInARow + 1;

                // clearing original bits
                for(int h = locationOfFile; h<(locationToStartAllocating); h++){
                    clearBit(h);
                }

                if(blocksInARow == blocksRequested+fileSize){
                    for(int j = locationOfAllocatedBlocks;j<(locationOfAllocatedBlocks+blocksRequested+fileSize);j++){
                        setBit(j);
                    }

                    LBAwrite(fsmap,5,1);  
                    return locationOfAllocatedBlocks;
                }                 
            } else{
                // adding block to blocks we're allocing
                blocksInARow= blocksInARow + 1;

                     // no used blocks in the way 
                if(blocksInARow == blocksRequested){
                    for(int j = locationOfAllocatedBlocks;j<locationOfAllocatedBlocks+blocksRequested;j++){
                        setBit(j);
                    }
                    LBAwrite(fsmap,5,1); 
                    return locationOfFile;
                } 

            }

       
        }  
        // the bit is not free
        // a block was hit
        if(getBitZeroOrOne(i)!=0){
            // indicator that we need to move stuff
            blockWasHit = 1;
            blocksInARow = 0;
        }     

    }
    return 0;
}

// Function to allocate bits in freespace bitmap
int allocFreeSpace(int blocksRequested){
    int locationOfAllocatedBlocks=-500;
    int blocksInARow=0;
    int totalBlocks = VCB_P->numBlocks;
    
    for(int i=0; i<totalBlocks;i++){
        //see if there is a free block
        if(getBitZeroOrOne(i)==0){
            if(blocksInARow==0){
                locationOfAllocatedBlocks=i;
            }
            
            blocksInARow= blocksInARow + 1;
            if(blocksInARow == blocksRequested){
                for(int j = locationOfAllocatedBlocks;j<locationOfAllocatedBlocks+blocksRequested;j++){
                    setBit(j);
                }
                LBAwrite(fsmap,5,1); // ToDO: Need to fix hardcoded value
                return locationOfAllocatedBlocks;
            } 
        }
    }
    return -1;
}

// Function to deallocate bits in freespace bitmap
int deallocFreeSpace(DE* entry){

    int locationOfAllocatedBlocks = entry->location;
    int size = ((entry->fileSize)+VCB_P->BlockSize-1)/VCB_P->BlockSize; //is bytes or blocks??

    for(int i=locationOfAllocatedBlocks; i<locationOfAllocatedBlocks+size;i++){
        clearBit(i);
        if(getBitZeroOrOne(i)!=0){
            printf("DEALLOCFREESPACE: ISSUE CLEARING BIT\n");
            return -1;
        }
    }
    LBAwrite(fsmap,5,1); 
    return 0;
}

// Function to allocate bits in freespace bitmap
int deallocBlocks(int location, int size){

    for(int i=location; i<location+size;i++){
        clearBit(i);
        if(getBitZeroOrOne(i)!=0){
            printf("DEALLOCFREESPACE: ISSUE CLEARING BIT\n");
            return -1;
        }
    }
    LBAwrite(fsmap,5,1); 
    return 0;
}

// Function to set bit to 1 (used)
void setBit(int blockNum) {
    int bytenum = blockNum / 8;
    int bitnum = blockNum % 8;
    
    fsmap[bytenum] = fsmap[bytenum] | (1 << bitnum);
}

// Function to set bit to 0 (unused)
void clearBit(int blockNum) {
    int bytenum = blockNum / 8;
    int bitnum = blockNum % 8;
   
    fsmap[bytenum] = fsmap[bytenum] & (~(1 << bitnum));
}

// Function which returns if bit is used or unused (0=unused 1=used)
int getBitZeroOrOne(int blockNum) {
    int bytenum = blockNum / 8;
    int bitnum = blockNum % 8;  
    return ((*(fsmap+bytenum)>>bitnum)&1);
}