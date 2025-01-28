/**************************************************************
* Class::  CSC-415-03 Spring 2024
* Name:: Halia Tavares, Cole Douglas, Bhargava Kadiyala, Pablo Partida
* Student IDs:: 921851070, 922128256, 920736411, 921514214
* GitHub-Name:: cole-d
* Group-Name:: Team Taro
* Project:: Basic File System
*
* File:: freeSpace.h
*
* Description:: This is the freespace interface
*
**************************************************************/
#include "fsInit.h"

#ifndef FREESPACEINIT
#define FREESPACEINIT   


int initFreeSpace(uint64_t numberOfBlocks, uint64_t blockSize);

int growFreeSpace(int blocksRequested,int locationToStartAllocating, int locationOfFile);

int allocFreeSpace(int blocksRequested);

int deallocFreeSpace(DE* entry);

int deallocBlocks(int location, int size);

u_int8_t *loadFreeSpace();

void setBit(int blockNum);
void clearBit(int blockNum);
int getBitZeroOrOne(int blockNum);

#endif