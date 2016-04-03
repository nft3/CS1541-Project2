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
#include <sys/time.h>
#include "trace_item"

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
	int index = cp->blocks[set][0]->timestamp, i;

	for(i = 1; i < cp->assoc; i++){
		if(cp->blocks[set][i]->timestamp < cp->blocks[set][i-1]->timestamp){
			index = i;
		}
	}

	return index;
}

/*
	For LRU replacement, we have to find the element in the set of the cache that is the least recently used.
	That means we need to go through the entire set of the cache and compare timestamps to find the one that is 
	the furthest in the past.
*/
int LRU_Replacement(struct cache_t *cp, unsigned long address, int set, char access_type, unsigned long long now) {
	
	int returnValue = -1, i;

	// The addresses are 32 bits long. Use this to find the tag.
	unsigned log tag = 32 - log2(cp->bsize) - log2(cp->nsets); 

	// Now let's check to see if the set is full. We can do that by seeing if every member of the struct is 0.
	for(i = 0; i < cp->assoc; i++){

		// We encounter tags that are the same, we have to update it in the cache.
		// If it is a store instruction mark as dirty, loads just read so not necessary
		// Also, change the valid bit to 1 if it is already not.
		// Update the time stamp for LRU!

		if(cp->blocks[set][i]->tag == tag){
			// If it is a hit, we only have to set the dirty bit on a store. On a read it doesn't matter.
			if(access_type == ti_store){
				cp->blocks[set][i]->dirty = 1;
			}
			cp->blocks[set][i]->valid = 1;
			cp->blocks[set][i]->timestamp = now; // Updated time stamp for a hit!
			returnValue = 0;
			break;
		}

		// We are encountering a block that has not yet been occupied. We can place it here, It is a miss.
		else if(!cp->blocks[set][i]->valid && !cp[set][i]->tag && !cp[set][i]->dirty && !cp[set][i]->timestamp){ 

			// Construct a new block
			struct cache_blk_t *newBlock = (struct cache_blk_t *)calloc(1, sizeof(struct cache_blk_t));
			constructNewBlock(newBlock, tag, now);

			// Now we can add this block where there isn't a block yet
			cp->block[set][i] = *newBlock;

			// We have to write back to memory if it is a store instruction.
			if(access_type == ti_store){
				returnValue = 2;
			}
			else if(access_type == ti_load){
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
			if(access_type == ti_store){
				returnValue = 2;
			}
			else if(access_type == ti_load){
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
int FIFO_Replacement(struct cache_t *cp, unsigned long address, int set, char access_type, unsigned long long now) {
	
	int returnValue = -1, i;

	// The addresses are 32 bits long. Use this to find the tag.
	unsigned log tag = 32 - log2(cp->bsize) - log2(cp->nsets); 

	// Loop through the set
	for(i = 0; i < cp->assoc; i++){

		// We encounter tags that are the same, we have to update it in the cache.
		// If it is a store instruction mark as dirty, loads just read so not necessary
		// Also, change the valid bit to 1 if it is already not.
		// DO NOT update the time stamp for FIFO!

		if(cp->blocks[set][i]->tag == tag){
			// If it is a hit, we only have to set the dirty bit on a store. On a read it doesn't matter.
			if(access_type == ti_store){
				cp->blocks[set][i]->dirty = 1;
			}
			cp->blocks[set][i]->valid = 1;
			returnValue = 0;
			break;
		}

		// If there is a circumstance where the set is not yet full and there is an empty space. This is a miss.
		// (If everything is 0, that's how we know that nothing is in that space in the set yet)
		else if(!cp->blocks[set][i]->valid && !cp[set][i]->tag && !cp[set][i]->dirty && !cp[set][i]->timestamp){ 

			// Construct a new block
			struct cache_blk_t *newBlock = (struct cache_blk_t *)calloc(1, sizeof(struct cache_blk_t));
			constructNewBlock(newBlock, tag, now);

			// Now we can add this block where there isn't a block yet
			cp->block[set][i] = *newBlock;

			// We have to write back to memory if it is a store instruction.
			if(access_type == ti_store){
				returnValue = 2;
			}
			else if(access_type == ti_load){
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
			if(access_type == ti_store){
				returnValue = 2;
			}
			else if(access_type == ti_load){
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
int cache_access(struct cache_t *cp, unsigned long address, char access_type, unsigned long long now)
{
    int status = 0; //return 0 if a hit, 1 if a miss or 2 if a miss_with_write_back
	
	//Use address to get set number and associativity number to access blocks within cashes (tag)
	
	//if load (read)
	
		//Check if block contains the right data (check that address is within the range)
			
		//if yes, return 0
			
		//if no, run replacement algorithm (which one to kick out)
			
			//if not dirty, 
				//return 1 and update cache
			
			//if dirty
				//write back and update cache
				//return 2
	
	//if store (write)
		
		//Check if block contains the right data (check that address is within the range)
		
		//if yes, 
		
			// update dirty bit return 0 
			
		//if no,, run replacement algorithm (which one to kick out)
		
			//if not dirty, 
				//return 1 and update cache
			
			//if dirty
				//write back and update cache
				//return 2
	
	return status; //return 0 if a hit, 1 if a miss or 2 if a miss_with_write_back
}

#endif