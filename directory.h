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
* Description:: Interface for all directory functions
*
**************************************************************/

#include "fsInit.h"

#ifndef ROOTDIRINIT
#define ROOTDIRINIT

int fs_mvdir(char * src, char *dest);

int findInDir(DE* parent, char* token);

DE *loadDir(DE* parentDir);

int isDEaDir(DE* entry);

DE* createDE(int numOfDE, uint64_t blockSize, DE* parent);

int findEmptyDEInDir(DE* parent);

int printDir(DE* parent);

int printFour(DE* parent);

int isDEEmpty(DE* entry);

int fs_rmdir(const char *pathname);

#endif