/**************************************************************
* Class::  CSC-415-03 Spring 2024
* Name:: Cole Douglas, Halia Tavares
* Student IDs:: 922128256, 921851070
* GitHub-Name:: cole-d
* Group-Name:: Team Taro
* Project:: Basic File System
*
* File:: b_io.c
*
* Description:: Basic File System - Key File I/O Operations
*
**************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>			// for malloc
#include <string.h>			// for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "b_io.h"

#include "fsInit.h"
#include "directory.h"
#include "fsLow.h"
#include "freeSpace.h"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512

typedef struct b_fcb
	{
	DE * parent;
	char * buf;			// holds the open file buffer
	int indexOfEntry;	// index of child in the parent
	int RWflag;			// mask for read/write flags 
	int index;			// holds the current position in the buffer
	int buflen;			// holds how many valid bytes are in the buffer
	int currentBlk;		// current block in the file you're on
	int numBlocks;		// number of blocks in the file
	int filePosition;
	int fileBlockNum;
	int blocksPreallocated;
	int isDirty;        // the buffer has not yet been written to disk
	} b_fcb;
	
b_fcb fcbArray[MAXFCBS];

int startup = 0;		// Indicates that this has not been initialized

int remainingBytesInBuf;

// Method to initialize our file system
void b_init (){
	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
		fcbArray[i].buf = NULL; //indicates a free fcbArray
		}
		
	startup = 1;
}

// Method to get a free FCB element
b_io_fd b_getFCB (){
	for (int i = 0; i < MAXFCBS; i++)
		{
		if (fcbArray[i].buf == NULL)
			{
			return i;		//Not thread safe (But do not worry about it for this assignment)
			}
		}
	return (-1);  //all in use
}
	
// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_io_fd b_open (char * filename, int flags){

	b_io_fd returnFd;

	ppretdata * ppinfo = malloc(sizeof(ppretdata));

	char * file = strdup(filename);

	if(parsePath(file, ppinfo)!=0){
        printf("B_OPEN: PARSEPATH FAILED\n");
		free(ppinfo);
		ppinfo = NULL;
        return -1;
    }

	int RWmask = O_RDONLY | O_WRONLY | O_RDWR;
	int RWflag  = flags & RWmask;
	char * buf = malloc(VCB_P->BlockSize);
	int index = 0;

	// if the location of the last element is negative -1 
	// therefore it does not exit
	if(ppinfo->lastElementIndex == -1){
		if(flags & O_CREAT == O_CREAT){
			//create the file
			DE * file = fs_create(ppinfo->lastElementName);

			if (file == NULL){
        		printf("B_OPEN: CREATE FILE FAILED\n");
        		
				free(ppinfo);
				free (file);

				ppinfo = NULL;
				file = NULL;

        		return -1;
    		}

			int newIndex = findEmptyDEInDir(ppinfo->parent);
			if(newIndex < 0){
       			printf("B_OPEN: COULDN'T FIND EMPTY DIRECTORY\n");
        		return -1;
    		};

			// Initializing new file in parent
    		strcpy(ppinfo->parent[newIndex].name,ppinfo->lastElementName);
    		ppinfo->parent[newIndex].location = file[0].location;
    		ppinfo->parent[newIndex].fileSize = file[0].fileSize;
    		ppinfo->parent[newIndex].isDir = file[0].isDir;
    		ppinfo->parent[newIndex].date = file[0].date;

			int size = (sizeof(ppinfo->parent) + VCB_P->BlockSize -1 )/(VCB_P->BlockSize);

			LBAwrite(ppinfo->parent,size , ppinfo->parent->location);
    
			ppinfo->lastElementIndex=newIndex; // update to the index
		}else{
			printf("B_OPEN: FILE DOES NOT EXIST. O_CREAT FLAG IS NOT SPECIFIED\n");

			  		
			free(ppinfo);
			free (file);

			ppinfo = NULL;
			file = NULL;

			return -1;
		}
	}

	if(flags & O_TRUNC == O_TRUNC){
		// check if write only or wr is specificed
		if((RWflag == O_WRONLY )||(RWflag == O_RDWR)){
			//get rid of blocks associated with file 
			deallocFreeSpace((&ppinfo->parent[ppinfo->lastElementIndex]));
			ppinfo->parent[ppinfo->lastElementIndex].fileSize = 0;
			ppinfo->parent[ppinfo->lastElementIndex].location =0;
		}
	}

    int filePosition = 0;
	int fileBlockNum = 0;

	if(flags & O_APPEND == O_APPEND){
		if((RWflag == O_RDONLY) || (RWflag == O_RDWR)){
			filePosition = ppinfo->parent[ppinfo->lastElementIndex].fileSize;
			index = (filePosition % VCB_P->BlockSize);
			fileBlockNum = filePosition / VCB_P->BlockSize;
			LBAread(buf, 1,ppinfo->parent[ppinfo->lastElementIndex].location+fileBlockNum);
			
		}
	}


	if(fs_isFile(file)!=1){
		printf("ERR B_OPEN: NOT A FILE.\n");
		  		
		free(ppinfo);
		free (file);

		ppinfo = NULL;
		file = NULL;

		return -1;
	}
		
	if (startup == 0) b_init(); //Initialize our system
	
	returnFd = b_getFCB(); // get our own file descriptor

	fcbArray[returnFd].parent = ppinfo->parent;
	fcbArray[returnFd].indexOfEntry = ppinfo->lastElementIndex;
	fcbArray[returnFd].buf = buf;
	fcbArray[returnFd].index = index;
	fcbArray[returnFd].buflen = 0;
	fcbArray[returnFd].currentBlk = 0;
	fcbArray[returnFd].numBlocks = ((ppinfo->parent[ppinfo->lastElementIndex].fileSize + 
		(VCB_P->BlockSize-1))/VCB_P->BlockSize);
	fcbArray[returnFd].RWflag = RWflag;
	fcbArray[returnFd].filePosition = filePosition;
	fcbArray[returnFd].fileBlockNum = fileBlockNum;
	fcbArray[returnFd].blocksPreallocated = 0;


	  		
	free(ppinfo);
	free (file);

	ppinfo = NULL;
	file = NULL;

	return (returnFd);
}


// Interface to seek function	
int b_seek (b_io_fd fd, off_t offset, int whence)
	{

	//takes a fd, position, and from
	//can be from start end or current 
	//int is wither + 0 or -
	//purpose of seek is to modify
	//can return where i currently am
	//reposition file pointer
	//not easy...
	//0 from end that means set the file posisiton equals end of file
	//0 from start set the fiel pos equals 0
	//never know when these are called
	//what if 4000 from start
	//fileposition = 4000
	//4000 from current 
	//fileposition = fp +4000
	//-100 from end 
	//set fp = filesize -100
	//200 from end 
	//fp = filesize + 200
	//point where i acn write a new header
	//can positoin past the end of the file
	//why insidious? -> not accessing data no reads or write
	// because we can seek past what's allocated
	//what happens when we change the fp the buffer might no longer make ur buffer valid
	//right????? huh...
	// your buffer might change from underneith you ?
	//change position ... dirty buffers... wrote 
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		
		
	return (0); //Change this
	}



// Interface to write function	
int b_write (b_io_fd fd, char * buffer, int count){
	int part1 = 0;
	int part2 = 0;
	int part3 = 0;

	int bytesWritten = 0;

	if (startup == 0) b_init(); // Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS)){
		return (-1); // invalid file descriptor
	}
	remainingBytesInBuf = fcbArray[fd].buflen - fcbArray[fd].index;
  
	int amountAlreadyDelivered = ((fcbArray[fd].currentBlk*VCB_P->BlockSize)-remainingBytesInBuf);
	
	fcbArray[fd].currentBlk = fcbArray[fd].filePosition/VCB_P->BlockSize;
	fcbArray[fd].index = fcbArray[fd].filePosition % VCB_P->BlockSize;

	// need to check if the file needs more space than there is then 
	// we need to allocate more blocks for the file.
	if(((fcbArray[fd].filePosition) + count) > 
		fcbArray[fd].parent[fcbArray[fd].indexOfEntry].fileSize){
		// need to grow
		int neededBlocks = (( (fcbArray[fd].parent[fcbArray[fd].indexOfEntry].fileSize) + count + 
			VCB_P->BlockSize - 1)/(VCB_P->BlockSize));

		int preAlloc = ((neededBlocks*2)+20);

		int fileSizeInBlocks = (((fcbArray[fd].parent[fcbArray[fd].indexOfEntry].fileSize) +
			VCB_P->BlockSize -1)/(VCB_P->BlockSize));

		int endLoc = fcbArray[fd].parent[fcbArray[fd].indexOfEntry].location+fileSizeInBlocks;

		if(neededBlocks>fcbArray[fd].blocksPreallocated){
			// growing
			fcbArray[fd].parent[fcbArray[fd].indexOfEntry].location = growFreeSpace(preAlloc, 
				endLoc, fcbArray[fd].parent[fcbArray[fd].indexOfEntry].location);

			fcbArray[fd].blocksPreallocated = preAlloc;
		}
		
		if(count<0){
			printf("B_WRITE: COUNT LESS THAN 0\n");
			bytesWritten = fcbArray[fd].parent[fcbArray[fd].indexOfEntry].fileSize;
			return bytesWritten;
		}	
		fcbArray[fd].parent[fcbArray[fd].indexOfEntry].fileSize = fcbArray[fd].filePosition + count;		
	}

	int numberOfBlocksToCopy = 0;

	if(remainingBytesInBuf>=count){
		part1 = count;
		part2 = 0;
		part3 = 0;
	}else{
		part1 = remainingBytesInBuf;

		part3 = count - remainingBytesInBuf;

		numberOfBlocksToCopy = part3/VCB_P->BlockSize;
		part2 = ((numberOfBlocksToCopy)*(VCB_P->BlockSize));

		part3 = part3 -part2;
	}

	if(part1>0){
		memcpy(fcbArray[fd].buf+fcbArray[fd].index, buffer, part1);
		fcbArray[fd].index = fcbArray[fd].index+part1;
		fcbArray[fd].isDirty = 1;
	}

	if(part2>0){
		for(int i = 0; i<numberOfBlocksToCopy;i++){
			bytesWritten = LBAwrite(buffer+part1, numberOfBlocksToCopy,fcbArray[fd].currentBlk 
				+ fcbArray[fd].parent[fcbArray[fd].indexOfEntry].location);
			fcbArray[fd].currentBlk += numberOfBlocksToCopy;
			part2 = bytesWritten*VCB_P->BlockSize;
		}
		
	}
	
	if(part3>0){
		memcpy(fcbArray[fd].buf+fcbArray[fd].index, buffer+part1+part2, part3);

		fcbArray[fd].isDirty = 1;
		bytesWritten=LBAwrite(fcbArray[fd].buf, 1, 
			fcbArray[fd].currentBlk + fcbArray[fd].parent[fcbArray[fd].indexOfEntry].location);

		fcbArray[fd].isDirty = 0;

		bytesWritten = bytesWritten * VCB_P->BlockSize;

		fcbArray[fd].currentBlk += 1;
		fcbArray[fd].index = 0;
		fcbArray[fd].buflen = bytesWritten;

		if(bytesWritten < part3){
			part3 = bytesWritten;
		}

		if(part3>0){
			
			memcpy(fcbArray[fd].buf+fcbArray[fd].index, buffer+part1+part2, part3);

			fcbArray[fd].isDirty = 1;

			fcbArray[fd].index = fcbArray[fd].index+part3;
		}
	}

	int bytesReturned = part1+part2+part3;

	fcbArray[fd].filePosition = fcbArray[fd].filePosition + bytesReturned;

	return (bytesReturned);	
}



// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill 
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+
int b_read (b_io_fd fd, char * buffer, int count){

		
	int part1 = 0;
	int part2 = 0;
	int part3 = 0;

	int bytesRead =0;

	if (startup == 0) b_init(); // Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS)){
		return (-1); //invalid file descriptor
	}
	int fileSize = fcbArray[fd].parent[fcbArray[fd].indexOfEntry].fileSize;

	remainingBytesInBuf = fcbArray[fd].buflen - fcbArray[fd].index;

	int amountAlreadyDelivered = ((fcbArray[fd].currentBlk*VCB_P->BlockSize)-remainingBytesInBuf);

	// if requesting more than there is in the file, 
	// just give them what is left in the file. 
	if((count + amountAlreadyDelivered) > fileSize){
		count = fileSize - amountAlreadyDelivered;

		if(count<0){
			bytesRead = fileSize;
			return bytesRead;
		}
	}

	int numberOfBlocksToCopy = 0;

	if(remainingBytesInBuf>=count){
		part1 = count;
		part2 = 0;
		part3 = 0;
	}else{
		part1 = remainingBytesInBuf;

		part3 = count - remainingBytesInBuf;

		numberOfBlocksToCopy = part3/VCB_P->BlockSize;
		part2 = ((numberOfBlocksToCopy)*(VCB_P->BlockSize));

		part3 = part3 -part2;
	}
		
	if(part1>0){
		memcpy(buffer, fcbArray[fd].buf+fcbArray[fd].index, part1);
		fcbArray[fd].index = fcbArray[fd].index+part1;
	}

	if(part2>0){
		bytesRead = LBAread(buffer+part1, numberOfBlocksToCopy,fcbArray[fd].currentBlk
		+ fcbArray[fd].parent[fcbArray[fd].indexOfEntry].location);
		fcbArray[fd].currentBlk += numberOfBlocksToCopy;
		part2 = bytesRead*VCB_P->BlockSize;
	}

	if(part3>0){
		bytesRead=LBAread(fcbArray[fd].buf, 1, 
		fcbArray[fd].currentBlk +fcbArray[fd].parent[fcbArray[fd].indexOfEntry].location);

		
		bytesRead = bytesRead * VCB_P->BlockSize;

		fcbArray[fd].currentBlk += 1;
		fcbArray[fd].index = 0;
		fcbArray[fd].buflen = bytesRead;

		if(bytesRead < part3){
			part3 = bytesRead;
		}

		if(part3>0){
			memcpy(buffer+part1+part2, fcbArray[fd].buf+fcbArray[fd].index, part3);
			fcbArray[fd].index = fcbArray[fd].index+part3;
		}
	}

	int bytesReturned = part1+part2+part3;

	return (bytesReturned);
}
	
// Interface to Close the file	
int b_close (b_io_fd fd){

	//check to see if buffer is dirty then write
	if(fcbArray[fd].isDirty==1){
		LBAwrite(fcbArray[fd].buf, 1, 
			fcbArray[fd].currentBlk + fcbArray[fd].parent[fcbArray[fd].indexOfEntry].location);
	}

	// trim unsed blocks from write
	int fileSizeInBlocks = (fcbArray[fd].parent[fcbArray[fd].indexOfEntry].fileSize+VCB_P->BlockSize-1)/(VCB_P->BlockSize);
	int extraPreallocedBlocks = fcbArray[fd].blocksPreallocated - fileSizeInBlocks;

	int locationOfEnd = fcbArray[fd].parent[fcbArray[fd].indexOfEntry].location + fileSizeInBlocks;

	deallocBlocks(locationOfEnd,extraPreallocedBlocks);

	// clean up things
	int parentFileSizeInBlocks = (fcbArray[fd].parent->fileSize+VCB_P->BlockSize-1)/(VCB_P->BlockSize); 

	LBAwrite(fcbArray[fd].parent, parentFileSizeInBlocks,fcbArray[fd].parent[0].location);

	fcbArray[fd].index = 0;

				
	free(fcbArray[fd].buf);
	fcbArray[fd].buf = NULL;

	fd = -1;

	return 0;	
}
