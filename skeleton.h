#ifndef __SKELETON_H__
#define __SKELETON_H__

///////////////////////////////////////////////////////////////////////////////
//
// CS 1541 Introduction to Computer Architecture
// You may use this skeleton code to create a cache instance and implement cache operations.
// Feel free to add new variables in the structure if needed.
//
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h> //only for linux
#include "trace_item.h"

#define CHECK_BIT(var,pos) ((var) & (1<<(pos))) //macro for checking if bit at position pos is 1

/////////////////////////////////////////////////////////////////////
//FOR WINDOWS ONLY
/////////////////////////////////////////////////////////////////////
/* #define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdint.h> // portable: uint64_t   MSVC: __int64

typedef struct timeval {
    unsigned long long tv_sec;
    unsigned long long tv_usec;
} timeval;

int gettimeofday(struct timeval * tp, struct timezone * tzp) {
    static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime( &system_time );
    SystemTimeToFileTime( &system_time, &file_time );
    time =  ((uint64_t)file_time.dwLowDateTime )      ;
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec  = (unsigned long long) ((time - EPOCH) / 10000000L);
    tp->tv_usec = (unsigned long long) (system_time.wMilliseconds * 1000);
    return 0;
} */
/////////////////////////////////////////////////////////////////////
//FOR WINDOWS ONLY
/////////////////////////////////////////////////////////////////////

struct cache_blk_t {
    unsigned long tag;
    char valid;
    char dirty;
    unsigned long long timestamp;
};

enum cache_policy {
    LRU,
    FIFO
};

struct cache_t {
    int nsets;        // # sets
    int bsize;        // block size
    int assoc;        // associativity
    
    enum cache_policy policy;       // cache replacement policy
    
    // ** Is a pointer to a pointer, we want to make a linked list of cache_blk_t
    struct cache_blk_t **blocks;    // cache blocks in the cache
};

struct cache_t * cache_create(int size, int blocksize, int assoc, enum cache_policy policy)
{
    // The cache is represented by a 2-D array of blocks.
    // The first dimension of the 2D array is "nsets" which is the number of sets (entries)
    // The second dimension is "assoc", which is the number of blocks in each set.
    
    int i;
    int nblocks = 1; // number of blocks in the cache
    int nsets = 1;   // number of sets (entries) in the cache
    
	//calculate number of blocks and sets in cache
	nblocks = (size * 1024) / blocksize;
	nsets = (size * 1024) / (blocksize * assoc);
    
    struct cache_t * C = (struct cache_t *)calloc(1, sizeof(struct cache_t));
    
    C->nsets = nsets;
    C->bsize = blocksize;
    C->assoc = assoc;
    C->policy = policy;
    
    C->blocks= (struct cache_blk_t **)calloc(nsets, sizeof(struct cache_blk_t));
    
    for(i = 0; i < nsets; i++) {
        C->blocks[i] = (struct cache_blk_t *)calloc(assoc, sizeof(struct cache_blk_t));
    }

    return C;
}

// Constructs a new block to be put in the set
void constructNewBlock(struct cache_blk_t *newBlock, unsigned long tag, unsigned long long now){
	newBlock->valid = 1;
	newBlock->tag = tag;
	newBlock->dirty = 0;
	newBlock->timestamp = now;
}

// Returns the index of the oldest block in the set
int findOldestBlock(struct cache_t *cp, int set){
	int index = cp->blocks[set][0].timestamp, i;

	for(i = 1; i < cp->assoc; i++){
	if(cp->blocks[set][i].timestamp < cp->blocks[set][i-1].timestamp){
			index = i;
		}
	}

	return index;
}

double logBaseTwo(int number){
	return log((double)number) / log(2);
}

/*
	For LRU replacement, we have to find the element in the set of the cache that is the least recently used.
	That means we need to go through the entire set of the cache and compare timestamps to find the one that is 
	the furthest in the past.
*/
int LRU_Replacement(struct cache_t *cp, unsigned long address, int set, int tag, char access_type, unsigned long long now) {

	int returnValue = -1, i;

	// Loop through all the blocks that are in the set in the cache, using assoc to determine how many blocks are in the set
	for(i = 0; i < cp->assoc; i++){

		// We encounter tags that are the same, we have to update it in the cache.
		// If it is a store instruction mark as dirty, loads just read so not necessary
		// Also, change the valid bit to 1 if it is already not.
		// Update the time stamp	 for LRU!

		if(cp->blocks[set][i].tag == tag){
			// If it is a hit, we only have to set the dirty bit on a store. On a read it doesn't matter.
			if(access_type == ti_STORE){
				cp->blocks[set][i].dirty = 1;
			}
			cp->blocks[set][i].valid = 1; // We are using it in memory, so it is valid
			cp->blocks[set][i].timestamp = now; // Updated time stamp for a hit!
			returnValue = 0;
			break;
		}

		// We are encountering a way in the set that has not yet been occupied. We can place it here, It is a miss.
		else if(!cp->blocks[set][i].valid && !cp->blocks[set][i].tag && !cp->blocks[set][i].dirty && !cp->blocks[set][i].timestamp){ 

			// Construct a new block
			struct cache_blk_t *newBlock = (struct cache_blk_t *)calloc(1, sizeof(struct cache_blk_t));
			constructNewBlock(newBlock, tag, now);

			// Add this block to the free space in the cache
			cp->blocks[set][i] = *newBlock;

			// We have to write back to memory if it is a store instruction
			if(access_type == ti_STORE){
				returnValue = 2;
			}
			else if(access_type == ti_LOAD){
				returnValue = 1;
			}

			break; 
		}

		// We have reached the end of the set and we have to evict the oldest
		// NOTE: Maybe this can be refactored to be outside of for loop?
		else if(i == (cp->assoc - 1)){
			// First let's construct a new block
			struct cache_blk_t *newBlock = (struct cache_blk_t *)calloc(1, sizeof(struct cache_blk_t));
			constructNewBlock(newBlock, tag, now);

			// Find the oldest block
			int indexOfOldestBlock = findOldestBlock(cp, set);

			// Now that we have the index of the oldest block, let's overwrite what is there 
			cp->blocks[set][indexOfOldestBlock] = *newBlock;
			
			// We have to write back to memory if it is a store instruction
			if(access_type == ti_STORE){
				returnValue = 2;
			}
			else if(access_type == ti_LOAD){
				returnValue = 1;
			}

			break;
		}
	}

	return returnValue;
}

/*
	For FIFO (First In First Out) replacement, we need to find the element that is at the the start of the 
	list of cache blocks in a set.
*/ 

/**
	FIFO IS DIFFERENT THAN LRU BECAUSE YOU DON'T UPDATE THE TIME STAMP IF THERE IS A HIT. IF WE UPDATE THE TIMESTAMP
	EVERYTIME, THEN IT IS LRU.
*/
int FIFO_Replacement(struct cache_t *cp, unsigned long address, int set, int tag, char access_type, unsigned long long now) {
	
	int returnValue = -1, i;

	// Loop through the set
	for(i = 0; i < cp->assoc; i++){

		// We encounter tags that are the same, we have to update it in the cache.
		// If it is a store instruction mark as dirty, loads just read so not necessary
		// Also, change the valid bit to 1 if it is already not.
		// DO NOT update the time stamp for FIFO!

		if(cp->blocks[set][i].tag == tag){
			// If it is a hit, we only have to set the dirty bit on a store. On a read it doesn't matter.
			if(access_type == ti_STORE){
				cp->blocks[set][i].dirty = 1;
			}
			cp->blocks[set][i].valid = 1;
			returnValue = 0;
			break;
		}

		// If there is a circumstance where the set is not yet full and there is an empty space. This is a miss.
		// (If everything is 0, that's how we know that nothing is in that space in the set yet)
		else if(!cp->blocks[set][i].valid && !cp->blocks[set][i].tag && !cp->blocks[set][i].dirty && !cp->blocks[set][i].timestamp){ 

			// Construct a new block
			struct cache_blk_t *newBlock = (struct cache_blk_t *)calloc(1, sizeof(struct cache_blk_t));
			constructNewBlock(newBlock, tag, now);

			// Now we can add this block where there isn't a block yet
			cp->blocks[set][i] = *newBlock;

			// We have to write back to memory if it is a store instruction.
			if(access_type == ti_STORE){
				returnValue = 2;
			}
			else if(access_type == ti_LOAD){
				returnValue = 1;
			}

			break; 
		}

		// We have reached the end of the set and we have to evict the oldest
		// NOTE: Maybe this can be refactored to be outside of for loop?
		else if(i == (cp->assoc - 1)){
			// First let's construct a new block
			struct cache_blk_t *newBlock = (struct cache_blk_t *)calloc(1, sizeof(struct cache_blk_t));
			constructNewBlock(newBlock, tag, now);

			// Find the oldest block
			int indexOfOldestBlock = findOldestBlock(cp, set);

			// Now that we have the index of the oldest block, let's overwrite what is there 
			cp->blocks[set][indexOfOldestBlock] = *newBlock;
			
			// We have to write back to memory if it is a store instruction
			if(access_type == ti_STORE){
				returnValue = 2;
			}
			else if(access_type == ti_LOAD){
				returnValue = 1;
			}

			break;
		}

	}

	return returnValue;
}

//////////////////////////////////////////////////////////////////////
//
// based on address determine the set to access in cp
// examine blocks in the set to check hit/miss
// if miss, determine the victim in the set to replace
// if update the block list based on the replacement policy
// return 0 if a hit, 1 if a miss or 2 if a miss_with_write_back
//
//////////////////////////////////////////////////////////////////////
int cache_access(struct cache_t *cp, unsigned long address, char access_type, FILE* file_results, int trace_view_on, unsigned long long now)
{
	int block_size; //in bytes
	int number_of_sets;
	int n_bits_for_block_offset;
	int n_bits_for_set_number;
	int n_bits_for_tag;
	unsigned long set; //index
	unsigned long tag;
	int i; //multipurpose variable
	
	//Use address to get set number and tag to access blocks within cashes
	block_size = cp->bsize;
	number_of_sets = cp->nsets;
	
	n_bits_for_block_offset = -1;
	for(i = 0; i < (sizeof(block_size)*8); i++) {
		n_bits_for_block_offset =  n_bits_for_block_offset + 1;
		if(CHECK_BIT(block_size, i)) {
			break;
		}
	}
	
	n_bits_for_set_number = -1;
	for(i = 0; i < (sizeof(number_of_sets)*8); i++) {
		n_bits_for_set_number =  n_bits_for_set_number + 1;
		if(CHECK_BIT(number_of_sets, i)) {
			break;
		}
	}
	
	if(cp->assoc == 1) {
		n_bits_for_tag = sizeof(address)*8 - n_bits_for_set_number;	
		n_bits_for_tag = sizeof(address)*8 - n_bits_for_set_number - n_bits_for_block_offset;		
		tag = address >> (n_bits_for_set_number);
		set = address & (~(tag<<(n_bits_for_set_number)));
	}
	else {
		n_bits_for_tag = sizeof(address)*8 - n_bits_for_set_number - n_bits_for_block_offset;		
		tag = address >> (n_bits_for_set_number + n_bits_for_block_offset);
		set = address & (~(tag<<(n_bits_for_set_number + n_bits_for_block_offset)));
		set = set >> n_bits_for_block_offset;
	}

	if (trace_view_on) {
		//printf("\nNumber of Bits for Block Offset: %d bits", n_bits_for_block_offset);
		//fprintf(file_results, "\nNumber of Bits for Block Offset: %d bits", n_bits_for_block_offset);
		//printf("\nNumber of Bits for Set Number: %d bits", n_bits_for_set_number);
		//fprintf(file_results, "\nNumber of Bits for Set Number: %d bits", n_bits_for_set_number);
		//printf("\nNumber of Bits for Tag: %d bits", n_bits_for_tag);
		//fprintf(file_results, "\nNumber of Bits for Tag: %d bits", n_bits_for_tag);
		printf("\nTag: %d", tag);
		fprintf(file_results, "\nTag: %x", tag);
		printf("\nSet: %d", set);
		fprintf(file_results, "\nSet Number: %d", set);
	}	
	
	//Check if block contains the right data (check that address is within the range)
	for(i = 0; i < cp->assoc; i++){
		if ((cp->blocks[(int)set][i].valid) && (cp->blocks[(int)set][i].tag == tag)) { //if yes, return 0
			return 0; //hit
		} 
	}
	//if no, run replacement algorithm (which one to kick out)
	if(cp->policy == LRU) {
		if(LRU_Replacement(cp, address, (int)set, (int) tag, access_type, now)== 1){//if not dirty, 
			return 1; //return 1 and update cache
		}
		else { //if dirty
			//write back and update cache
			return 2;	//return 2
		}
	}
	else {
		if(FIFO_Replacement(cp, address, (int)set, (int)tag, access_type, now)== 1){//if not dirty, 
			return 1; //return 1 and update cache
		}
		else { //if dirty
			//write back and update cache
			return 2;	//return 2
		}			
	}

	
}
#endif
