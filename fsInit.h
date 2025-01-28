/**************************************************************
* Class::  CSC-415-03 Spring 2024
* Name:: Halia Tavares, Cole Douglas, Bhargava Kadiyala, Pablo Partida
* Student IDs:: 921851070, 922128256, 920736411, 921514214
* GitHub-Name:: cole-d
* Group-Name:: Team Taro
* Project:: Basic File System
*
* File:: fsInit.h
*
* Description:: 
*       Interface for global variables and structures
*       for the file
*
**************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <mfs.h>


#ifndef FSINIT
#define FSINIT

#define DEFAULT_NUMOFDIRS 50

extern struct fs_stat* fsStatP;

typedef struct VCB{
    long signature;
    int locationOfFreespace;
    int numBlocks; 
    int locationOfRootDir;
    int rootdirectorySize;
    int BlockSize; 

} VCB;

extern VCB* VCB_P;

typedef struct DE{
    char name[64];
    int location; 
    int fileSize;
    bool isDir;
    time_t date;
} DE;

extern DE* rootDirectory;

typedef struct ppretdata{
    DE * parent;
    int lastElementIndex;
    char * lastElementName;
} ppretdata;

int parsePath(char * pathname,ppretdata * ppinfo);

extern DE * loadedCWD;
extern DE * loadedRD;
extern char loadedCWDString[5000];


extern u_int8_t * fsmap; // Global variable declaration for fsmap

#endif