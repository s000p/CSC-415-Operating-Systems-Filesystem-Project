/**************************************************************
* Class::  CSC-415-03 Spring 2024
* Name:: Halia Tavares, Cole Douglas
* Student IDs:: 921851070, 922128256
* GitHub-Name:: cole-d
* Group-Name:: Team Taro
* Project:: Basic File System
*
* File:: mfsCWD.c
*
* Description:: 
*	This holds the set cwd and get cwd functions as well
*   as their helper functions.
*	This is what the user needs to use commands such as 
*   cd and pwd.
*
**************************************************************/
#include "mfs.h"
#include "directory.h"
#include "freeSpace.h"
#include "fsLow.h"

DE * loadedCWD;
char loadedCWDString[5000];
DE * loadedRD;

// Function to set the current working directory 
// Used by cd shell command
int fs_setcwd(char *pathname){

    ppretdata * ppinfo = malloc(sizeof(ppretdata));

    if(parsePath(pathname, ppinfo)!=0){ 
        printf("FS_SETCWD: INVALID PATH\n");

        free(ppinfo);

        ppinfo = NULL;

        return -1; 
    }

    if (ppinfo->lastElementIndex == -1){   
        printf("FS_SETCWD: INVALID ENTRY\n");

        free(ppinfo);

        ppinfo = NULL;

        return -1;
    }

    if(isDEaDir(&(ppinfo->parent[ppinfo->lastElementIndex]))!=0){ 
                
        free(ppinfo);

        ppinfo = NULL;

        return -1;
    }

    DE * temp = loadDir(&(ppinfo->parent[ppinfo->lastElementIndex])); 

    if(loadedCWD!=loadedRD){
        free(loadedCWD);
    }

    loadedCWD = temp; 

    //if path starts with "/" then it is absolute
    if(pathname[0]=='/'){
        strcpy(loadedCWDString,pathname);
    }else{
        //it is relative path 
        //create a new string / cat of the current string and path spec
        strcat(loadedCWDString, pathname);
        
        //look at last character and if not a / append a /
        if(pathname[sizeof(pathname)]!='/')
            {
            strcat(loadedCWDString, "/");
            }
    }

    //... more to fix weird paths... do later... helper function

    free(ppinfo);

    ppinfo = NULL;

    return 0;
} //linux chdir

// Function that helps to fix weird paths
// Todo
int adjustPath(ppretdata* ppinfo){
    return 0;
}

// Function which parses the path and retrieves parent and last element info
int parsePath(char * pathname,ppretdata * ppinfo){
    DE* startParent = NULL;
    DE* parent = NULL;
    DE* tempParent = NULL;
    char* token = NULL;
    char* token2 = NULL;
    int index = -1;

    if(pathname==NULL){
        printf("PARSEPATH: PATHNAME IS NULL\n");
    
        return -1;
    }

    if(ppinfo==NULL){
        printf("PARSEPATH: PPINFO IS NULL\n");

        return -1;
    }

    if(pathname[0]=='/'){
        startParent=loadedRD; 
    }else{
        startParent = loadedCWD;
    }

    parent = startParent;

    token = strtok(pathname,"/");

    if(token == NULL)
        {
        if(pathname[0]=='/'){
            ppinfo->parent =parent;
            ppinfo->lastElementIndex = -2;
            ppinfo->lastElementName = NULL;
            return 0;
        }
        else{
            printf("PARSEPATH: TOKEN DOES NOT EXIST\n");
            return -1;
        }
    }

    while(1){
        index = findInDir(parent,token);

        token2 = strtok(NULL,"/");
        //at the last element 
 	    if (token2 == NULL) {
		        ppinfo->parent = parent ;
		        ppinfo->lastElementIndex = index;
		        ppinfo-> lastElementName = token;
		        return 0;
            }

       if (index == -1){
            printf("PARSEPATH: INVALID PATH\n");
		    return -1; 
            }
	    
        if (isDEaDir(&parent[index])==0){
            tempParent = loadDir(&parent[index]);
            if (parent != startParent )
                free(parent);
            parent = tempParent;
            token = token2;
        } 
        else{
            printf("PARSEPATH: INVALID PATH\n");
            return -1; 
        }
    } // END OF WHILE	
}

// Function to get the current working directory
// Used by shell command pwd
char * fs_getcwd(char *pathname, size_t size){

    strncpy(pathname,loadedCWDString, size);
    return pathname;
}

// Function to create a new directory
int fs_mkdir(const char *pathname, mode_t mode){
    
    ppretdata * ppinfo = malloc(sizeof(ppretdata));
    char * path = strdup(pathname);

    if(ppinfo==NULL){
        printf("FS_MKDIR: PPINFO INVALID\n");

        free(ppinfo);
        free(path);

        ppinfo = NULL;
        path = NULL;

        return -1;
    }

    if(parsePath(path, ppinfo)!=0){
        printf("FS_MKDIR: PARSE PATH FAILED\n");

        free(ppinfo);
        free(path);

        ppinfo = NULL;
        path = NULL;
    
        return -1;
    }

    if(ppinfo->lastElementIndex != -1){
        printf("FS_MKDIR: NAME ALREADY EXISTS\n");

        free(ppinfo);
        free(path);

        ppinfo = NULL;
        path = NULL;

        return -1;
    }

    DE* newdir = createDE(DEFAULT_NUMOFDIRS,VCB_P->BlockSize,  ppinfo->parent);

    if (newdir == NULL){
        printf("FS_MKDIR: CREATEDE CALL FAILED\n");

        free(ppinfo);
        free(path);
        free(newdir);

        ppinfo = NULL;
        path = NULL;
        newdir = NULL;
        
        return -1;
    }
  
    int index = findEmptyDEInDir(ppinfo->parent);

    if(index < 0){
        printf("FS_MKDIR: COULDN'T FIND EMPTY DIRECTORY\n");
        
        free(ppinfo);
        free(path);
        free(newdir);

        ppinfo = NULL;
        path = NULL;
        newdir = NULL;

        return -1;
    };

    strcpy(ppinfo->parent[index].name,ppinfo->lastElementName);
    ppinfo->parent[index].location = newdir[0].location;
    ppinfo->parent[index].fileSize = newdir[0].fileSize;
    ppinfo->parent[index].isDir = newdir[0].isDir;
    ppinfo->parent[index].date = newdir[0].date;

    int size = ((sizeof(ppinfo->parent))+VCB_P->BlockSize-1)/VCB_P->BlockSize;

    LBAwrite(ppinfo->parent, size, ppinfo->parent->location);

    free(ppinfo);
    free(path);
    free(newdir);

    ppinfo = NULL;
    path = NULL;
    newdir = NULL;
    
    return 0;
}