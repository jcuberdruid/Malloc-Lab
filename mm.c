/*
* 
*	Author: Jason Cuthbert 
*	Email: jcuthber@uccs.edu
*
* 	description: 
*
*	Each block of memory has a header and a footer; the footer points to the header, the header contains pointers to  next and previous blocks, aswell as the 
*	size of the blocks payload, this multiple of 8 bytes (for alignment) payload may be bigger than the payload requested by the user. Using a first fit strategy *       freed blocks are reused; if they are large enough to be split then they are, otherwise the free block is transfered to the allocated list. 
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"


team_t team = {
    /* Team name */
    "teambla",
    /* First member's full name */
    "Jason Cuthbert",
    /* First member's email address */
    "jcuthber@uccs.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

struct header_f { //header structure for free and allocated blocks
   struct header_f* h_prev; 
   struct header_f* h_next; 
   size_t h_size;  // bottom bit of size = 1 -> free
}; //size 24
#define HEADER_SIZE (sizeof(struct header_f))

struct footer_f { //footer structure for free and allocated blocks 
   struct header_f* f_header; // points to this block's header 
}; //size 8 
#define FOOTER_SIZE (sizeof(struct footer_f))


/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))
#define ALIGN_DOWN(size) ((size) & ~(ALIGNMENT-1))
#define SIZE_T_SIZE ALIGN(sizeof(size_t))

#define END_OF_PROLOGUE (mem_heap_lo() + 2*HEADER_SIZE) //this is a void*

struct header_f* getPrevPhyBlk(struct header_f* thisBlk) {
	struct footer_f* prevFooter = (struct footer_f*)((void*)thisBlk - FOOTER_SIZE); 
	if(((void*)prevFooter) < END_OF_PROLOGUE) return 0;
	return prevFooter->f_header;
}

struct header_f* getNextPhyBlk(struct header_f* thisBlk) {
	size_t payloadSize = thisBlk->h_size & ~1;
	size_t thisBlkSize = HEADER_SIZE + payloadSize + FOOTER_SIZE;
	void* nextPhyBlk = (void*)thisBlk + thisBlkSize; 
	if(nextPhyBlk + HEADER_SIZE > mem_heap_hi()) return 0; 
	return (struct header_f*)nextPhyBlk; 
}

/* // Debugging Function: Dumps memory
void memdump () {
	FILE *f;
//	f = fopen("memdump.txt", "w");
	f = stdout;  

	struct header_f* allocBasep = (struct header_f*)mem_heap_lo();
	fprintf(f, "A:             %20p %20p \n",  allocBasep->h_prev, allocBasep->h_next);
	struct header_f* freeBasep = allocBasep + 1;
	fprintf(f, "F:           %20p %20p \n",  freeBasep->h_prev, freeBasep->h_next); 

	struct header_f* thisBlk = ((struct header_f*)mem_heap_lo())+2;
	while(thisBlk != 0 && (void*)thisBlk < mem_heap_hi()) {
		struct footer_f* ftr = ((void*)thisBlk) + HEADER_SIZE + (thisBlk->h_size &~1);
		if (thisBlk->h_size & 1) {//free? 
			fprintf(f, "			%20p: %20p %20p, %5zu  (%p)\n", thisBlk, thisBlk->h_prev, thisBlk->h_next, thisBlk->h_size, ftr->f_header);
		} else {
			fprintf(f, "%20p: %20p %20p, %5zu  (%p)\n", thisBlk, thisBlk->h_prev, thisBlk->h_next, thisBlk->h_size, ftr->f_header);
		}
		thisBlk = getNextPhyBlk(thisBlk);
	}
//	fclose(f);
}
*/

void removeFromList(struct header_f* thisBlk){
	if(thisBlk->h_next != 0){
		thisBlk->h_next->h_prev = thisBlk->h_prev;
		thisBlk->h_prev->h_next = thisBlk->h_next;
	} else { 
		struct header_f* allocBasep = (struct header_f*)mem_heap_lo();
		allocBasep->h_prev = thisBlk->h_prev;
		thisBlk->h_prev->h_next = (struct header_f*)mem_heap_lo();
	}
}

/* debug function for ensuring that all free addresses are marked as free 
void isAddressOnAllocList(struct header_f* thisBlk) {
	struct header_f* allocBasep = (struct header_f*)mem_heap_lo();
        for(struct header_f* this = allocBasep->h_next; this != allocBasep; this = this->h_next) {
                if (this == thisBlk) { 
           	      //printf("isAddressOnAllocList: Error -> alloced not marked as free at %p\n", this);
                      exit(0);
                }
        }
	return;
}
*/

void addToAllocList(struct header_f* thisBlk) {
	thisBlk->h_next = (struct header_f*)mem_heap_lo();
	thisBlk->h_prev = thisBlk->h_next->h_prev;
	thisBlk->h_prev->h_next = thisBlk;
	thisBlk->h_next->h_prev = thisBlk;
}
void addToFreeList(struct header_f* thisBlk) {
	//printf("adding free %d\n", thisBlk);
	thisBlk->h_next = ((struct header_f*)mem_heap_lo())+1;
	thisBlk->h_prev = thisBlk->h_next->h_prev;
	thisBlk->h_prev->h_next = thisBlk;
	thisBlk->h_next->h_prev = thisBlk;
}

/* 
 * 	mm_init - initialize the malloc package.
 *
 *	Usage: set up heap, the first 8 bytes is the base header of all allocated blocks, the next 8 is likewise for the free block base header.
 *	Assign these headers to the free and allocated list, create a third header to be the first free block in the list, as well as it's footer. 
 *
 */

int mm_init(void) {
    void *p = mem_sbrk (3*HEADER_SIZE+FOOTER_SIZE); 
    //printf("memlo %p,     int init p: %p\n",mem_heap_lo(), p );
		    
    struct header_f* allocBasep = (struct header_f*)p;		// base of all allocated blocks
    allocBasep->h_next = allocBasep->h_prev = allocBasep; 

    p += HEADER_SIZE;
    struct header_f* freeBasep = (struct header_f*)p;		// base of all free blocks
    freeBasep->h_next = freeBasep->h_prev = freeBasep; 
   
    p += HEADER_SIZE;
    struct header_f* freeHeader = (struct header_f*)p;		// first free block
    freeHeader->h_size = 0 | 1; //this block is free

    p += HEADER_SIZE;
    struct footer_f* freeFooter = (struct footer_f*)p;
    freeFooter->f_header = freeHeader;

    addToFreeList(freeHeader);

    return 0;
}

/*
 *	sbrkBlk: Supporting Function
 *	
 *	usage: when mm_alloc cannot find any free blocks of sufficient size. 
 *	
 *	logic: 
 *
 *	Is the last physical block free? 
 *
 *	yes: allocate the required size minus the size of last free block, reuse block, return
 *	no: allocate full required size, create a new allocated block at the end of the list, return
 * 
 */

struct header_f* sbrkBlk(size_t req_size) {

	struct footer_f* lastFooter = (struct footer_f*)ALIGN_DOWN((unsigned long)mem_heap_hi());
	struct header_f* lastHeader = lastFooter->f_header;

	size_t current_size = (lastHeader->h_size & ~1);
	if (lastHeader->h_size & 1) { //free? 
		removeFromList(lastHeader);
		size_t needed_size = req_size - current_size; 
		(void)mem_sbrk(needed_size); 
		lastHeader->h_size = req_size; 
		struct footer_f* lastFooter = (struct footer_f*)ALIGN_DOWN((unsigned long)mem_heap_hi());
		lastFooter->f_header = lastHeader; 
		addToAllocList(lastHeader);
		return lastHeader; 
	}
	//no  <- not free 
	struct header_f* new_header = mem_sbrk(ALIGN(req_size+FOOTER_SIZE+HEADER_SIZE)); 
	struct footer_f* new_footer = (struct footer_f*)((void*)new_header + HEADER_SIZE + req_size);
	new_header->h_size = req_size; 
	addToAllocList(new_header);
	new_footer->f_header = new_header;
	return new_header; 
}

/* 
 * mm_check(void)
 *  
 * 
 * 
 *
int mm_check(void) {
    	struct header_f* allocBasep = (struct header_f*)mem_heap_lo(); 
    	struct header_f* freeBasep = allocBasep + 1;
	//for each free block; are you marked as free? 
	for(struct header_f* this = freeBasep->h_next; this != freeBasep; this = this->h_next) { 
		if (((this->h_size)%2) == 1) { // because of free bit 
			continue; 	
		} 
		//printf("mm_check: Error -> free block not marked as free at %p\n", this);
		exit(0);
	}
	for(struct header_f* this = allocBasep->h_next; this != allocBasep; this = this->h_next) { 
		if (((this->h_size)%2) == 1) { // because of free bit 
			//printf("mm_check: Error -> allocated block marked as free at %p\n", this);
			exit(0);
		} 
	}
	return 0; 
}
 */




/*
 *      mm_malloc:
 *      
 *      usage: takes in request from user for pointer to data; uses all or part of existing free block with first fit strategy, 
 *      otherwise increases the size of the heap
 *
 *      logic: 
 *	
 *	for loop through free linked list: 
 * 	
 * 	Is the size greater or equal to requested size? 
 *
 *      yes: 
 *		Is the free block big enough split it? 
 *		yes: split the free block, create a new allocated block within it and return
 *		no: reuse the free block, as allocated block and return 
 *      no:
 *      	call sbrkBlk to allocate more memory 
 * 
 */
 
void* mm_malloc(size_t size)
{ 
    size = ALIGN(size);

    // traverse free list for fit
    struct header_f* allocBasep = (struct header_f*)mem_heap_lo(); 
    struct header_f* freeBasep = allocBasep + 1;

    for(struct header_f* this = freeBasep->h_next; this != freeBasep; this = this->h_next) { 
	if ((this->h_size & ~1) < size) { // & ~1 because of free bit 
		continue; 	
	}
	//first fit
	size_t chunkSize = size + HEADER_SIZE + FOOTER_SIZE;
	size_t minFreeSize = 8 + HEADER_SIZE + FOOTER_SIZE;
	size_t currentFreeSize = (this->h_size & ~1) + HEADER_SIZE + FOOTER_SIZE;

	//Can we split the free space? 
	if ((chunkSize + minFreeSize) <= currentFreeSize) {
		//make new freeblock; link it in; make payloadblock; return it; 
		this->h_size = (this->h_size & ~1) - chunkSize; // resize this size need to refree it
		this->h_size |= 1;
		struct header_f* newChunkHeader = (struct header_f*)((void*)this+currentFreeSize-chunkSize); //create new alloc header
		struct footer_f* newChunkFooter = (struct footer_f*)((void*)newChunkHeader + HEADER_SIZE + size);
		struct footer_f* freeFooter = ((struct footer_f*)newChunkHeader)-1;
		//insert newChunkHeader into alloc linked-list
		addToAllocList(newChunkHeader);
		newChunkHeader->h_size = size;
		//point free footer to free header 
		freeFooter->f_header = this;
		//point chunk footer to chunk header
		newChunkFooter->f_header = newChunkHeader;
		return (void*)(newChunkHeader+1);
	}
	// no excess free space in freeblk; give all to user
	removeFromList(this);
	addToAllocList(this);
	this->h_size = (this->h_size &~1);
	return (void*)(this+1); // return payload addr 
    }

    void* returnVal = (void*)(sbrkBlk(size)+1); 
    return returnVal; 
}
/*
 * coallescing function: coalesceses a header ptr and its next
 */

struct header_f* mm_coalesce(struct header_f* keptBlk) {
	struct header_f* nextBlk = getNextPhyBlk(keptBlk);
	removeFromList(keptBlk);
	removeFromList(nextBlk);

	//printf("coallescing nextBlk\n");
	keptBlk->h_size += HEADER_SIZE + (nextBlk->h_size & ~1) + FOOTER_SIZE; 
	void* footerAddr = ((void*)keptBlk) + HEADER_SIZE + (keptBlk->h_size &~1); 
	(((struct footer_f*)footerAddr))->f_header = keptBlk; 

	return keptBlk;
}


/*
 * mm_free - Free the block, if adjacents blocks are also free coalesce them 
 */

void mm_free(void *ptr)
{
	struct header_f* thisBlk = (struct header_f*)(ptr - HEADER_SIZE);
	removeFromList(thisBlk);

	struct header_f* nextBlk = getNextPhyBlk(thisBlk);
	if (nextBlk && (nextBlk->h_size & 1)) { // next block coallescible? 
		thisBlk = mm_coalesce(thisBlk);
	}
	struct header_f* prevBlk = getPrevPhyBlk(thisBlk);
	if (prevBlk && (prevBlk->h_size & 1)) { // previous block coallescible? 
		thisBlk = mm_coalesce(prevBlk);
	}

	addToFreeList(thisBlk);
	if (thisBlk->h_size & ~1) {
		thisBlk->h_size |= 1;
	}
}

/*
 *
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 *
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
