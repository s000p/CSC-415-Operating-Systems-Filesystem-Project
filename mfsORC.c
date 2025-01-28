/**************************************************************
* Class::  CSC-415-03 Spring 2024
* Name:: Pablo Partida
* Student IDs:: 921514214
* GitHub-Name:: cole-d
* Group-Name:: Team Taro
* Project:: Basic File System
*
* File:: mfsORC.c
*
* Description:: 
*   This file contains opendir, readdir, closedir functions
*
**************************************************************/


#include "mfs.h"
#include "directory.h"
#include "directory.h"
//#include "fsInit.h"

// Function to open a directory
fdDir *fs_opendir(const char *pathname){
    //duplicate the path to manupilate without any issues
     char *path = strdup(pathname);
     ppretdata *pathInfo = malloc(sizeof(ppretdata));
     //we want to parse the path and check if it a valid path
     if(parsePath(path, pathInfo) != 0) { 
         printf("Error: Invalid path"); 
         return NULL;
     } 
     // current directory based on the parse path 
     DE * currDir;
     if(pathInfo->lastElementIndex == -2){
        currDir = rootDirectory;
     }else {
        // want to check if last element in path is a valid directory 
        if( isDEaDir(&(pathInfo->parent[pathInfo->lastElementIndex]))!=0) {
        printf("Error: Invalid directory entry\n");
         return NULL;
           
     }
        // load in the last element of the directory 
        currDir = loadDir(&pathInfo->parent[pathInfo->lastElementIndex]);     
     }
 
     fdDir *fdd = malloc(sizeof(fdDir));
     if (fdd == NULL) {
         return NULL;
     }
     //intialize directory struct
     fdd->d_reclen = sizeof(fdDir);
     fdd-> dirEntryPosition = 0;  
     fdd-> fdd= currDir;
     fdd ->di=NULL;
     
    
     //make sure to clean up and return our directory struct 
     return fdd; 
 }

// Function to read a directory
 struct fs_diriteminfo *fs_readdir(fdDir *dirp){
     if(dirp== NULL){
         return NULL;
     }
     // allocate memory for directory item info 
     DE * currDir = dirp -> fdd;
     int entryCount = currDir[0].fileSize / sizeof(DE); // number of directory entries 
     
     struct fs_diriteminfo *itemInfo = malloc (sizeof(struct fs_diriteminfo));
      if(itemInfo == NULL){
        return NULL;
     }
     //loop through directory entries, start at dirEntryPositon = 0
     for(int i= dirp-> dirEntryPosition; i < entryCount; i++){

        //check if direcotry entry is not empty
        if(currDir[i].location!=-1){
        //populate directory item info 
         strcpy(itemInfo->d_name, currDir[i].name);
         itemInfo -> d_reclen = dirp->d_reclen;
         itemInfo -> fileType = -1;
         // write an in statement to check what type of fileType
        if (currDir[i].isDir) {
                itemInfo->fileType = FT_DIRECTORY;
                
            } else {
                itemInfo->fileType = FT_REGFILE; 
            }
         dirp-> dirEntryPosition = i+1; 
         return itemInfo;

     }
    
     //then check for end, have we reached the end 
     if(dirp -> dirEntryPosition == entryCount-1){ 
         free(itemInfo); 
         return NULL;
     }
     dirp-> dirEntryPosition = i+1; 

 }
        
 }

 // Function to close a directory 
 int fs_closedir(fdDir *dirp){
        if(dirp == NULL){
            return -1;
        
    }
    if(dirp -> di !=NULL){
        free(dirp->di);
    }
    //check if the loaded directory is not the root or current working directory
    if(dirp->fdd != NULL && dirp -> fdd != loadedRD && dirp -> fdd != loadedCWD) {
        free(dirp->fdd);
         
    }
    free(dirp);
    return 0;
    }