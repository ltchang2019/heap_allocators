/* Luke Tchang
 * CS 107
 * implicit: This program is an implementation of an implicit free list heap allocator. The only major design decision 
 *           was using first_fit search for malloc requests. Other than that, free is standard (doesn't support coalesce)
 *           and realloc doesn't support in place operations.
 */ 
#include "allocator.h"
#include "debug_break.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
​
//CONSTANTS:
//___________
#define WIDTH 8
#define ALLOC 1
#define FREE 0
​
​
/* GLOBAL VARIABLES:
 * _________________
 * segment_start: beginning of heap segment
 * segment_size: total size of heap segment
 * start_block: first block in segment (initialized to start + 8 bytes)
 * nblocks: total number of blocks (free and allocated)
 * nused: total number of allocated blocks
 * last_free_block: most recent free block appended to explicit free list
 */
static void *segment_start;
static void *segment_end;
static size_t segment_size;
static size_t nblocks;
static int nused;
static size_t bytes_used;
static void *start_block;
​
​
//SHORT HELPER FUNCTIONS:
//_______________________
​
/* Functions: get_hdr
 * __________________
 * Parameters:
 *    - block_ptr: pointer to start of block
 *
 * Return: pointer to block header
 *
 * Description: This function takes a pointer to the start of a block and returns a pointer to the start of that 
 *              block's header.
 */
void *get_hdr(void *block_ptr) {
    return ((char *)block_ptr - WIDTH);
}
​
/* Functions: get_ul
 * __________________
 * Parameters:
 *    - hdr_adr: pointer to block header
 *
 * Return: size_t contained in header (but not the actual size since size if right shifted)
 *
 * Description: This function is used as a utility for other functions that work with the blocks header such as 
 *              getting the block's size or allocation status.
 */
unsigned long get_ul(void *hdr_adr) {
    return *(unsigned long *)hdr_adr;
}
​
/* Functions: get_block_size
 * __________________
 * Parameters:
 *    - block_ptr: pointer to start of block
 *
 * Return: size of block 
 *
 * Description: This function takes a pointer to the start of a block and returns the size of that block.
 */
size_t get_block_size(void *block_ptr) {
    return get_ul(get_hdr(block_ptr)) >> 2;
}
​
/* Functions: get_next_block
 * __________________
 * Parameters:
 *    - block_ptr: pointer to start of block
 *
 * Return: pointer to the neighboring right block
 *
 * Description: This function takes a pointer to the start of a block and returns a pointer to the start of the 
 *              next block.
 */
void *get_next_block(void *block_ptr) {
    void *next_block = (char *)block_ptr + get_block_size(block_ptr) + WIDTH;
    return (next_block < segment_end) ? next_block : NULL;
}
​
/* Functions: check_alloc
 * __________________
 * Parameters:
 *    - block_ptr: pointer to start of block
 *
 * Return: 0 if block is free, 1 if block is allocated
 *
 * Description: This function takes a pointer to the start of a block and returns 0 or 1 depending on whether or not 
 *              the block is free or allocated.
 */
int check_alloc(void *block_ptr) {
    return get_ul(get_hdr(block_ptr)) & 0x1;
}
​
/* Functions: round_up
 * __________________
 * Parameters:
 *    - req_size: size specified in malloc or realloc request
 *
 * Return: rounded up version of original request (to nearest multiple of 8)
 *
 * Description: This function rounds up the client's request size to the nearest multiple of 8 and returns that number.
 */
size_t round_up(size_t req_size){
    return (req_size + WIDTH - 1) & ~(WIDTH - 1);
}
​
/* Functions: set_hdr_size
 * __________________
 * Parameters:
 *    - block_ptr: pointer to start of block
 *    - upd_size: new size to write into block header
 *
 * Return: N/A
 *
 * Description: This function updates the size written into the header of the given block.
 */
void set_hdr_size(void *block_ptr, size_t upd_size){
    void *hdr_ptr = get_hdr(block_ptr);
    *(unsigned long *)hdr_ptr = (get_ul(hdr_ptr) & 0x3) | (upd_size << 2);
}
​
/* Functions: set_hdr_status
 * __________________
 * Parameters:
 *    - block_ptr: pointer to start of block
 *    - status: FREE (0) or ALLOC (1)
 *
 * Return: N/A
 *
 * Description: This function updates the allocation status of written into the last bit of the block's header.
 */
void set_hdr_status(void *block_ptr, unsigned long status) {
    void *hdr_ptr = get_hdr(block_ptr);
    *(unsigned long *)hdr_ptr = ((get_ul(hdr_ptr) & ~0x3) | status);
}
​
//LONGER HELPER FUNCTIONS:
//________________________
​
/* Function: first_fit
 * ___________________
 * Parameters:
 *    - req_size: requested block size
 *
 * Return: pointer to first free block able to satisfy malloc request OR NULL if no free blocks available to
 *         satisfy request
 *
 * Description: This function traverse the heap looking for the first free block big enough to satisfy the malloc 
 *              request. If a block is found, a pointer to that block is returned. Otherwise, NULL is returned.
 */
void *first_fit(size_t req_size) {
    void *curr_block = start_block;
    for(int i = 0; i < nblocks; i++) {
        if(get_block_size(curr_block) >= req_size && !check_alloc(curr_block)) {
            return curr_block;
        }
        curr_block = get_next_block(curr_block);
    }
    return NULL;
}
​
/* Function: best_fit
 * ___________________
 * Parameters:
 *    - req_size: requested block size
 *
 * Return: pointer to best fitting free block able to satisfy malloc request OR NULL if no free blocks available to
 *         satisfy request
 *
 * Description: This function traverses the heap looking for the first free block big enough to satisfy the malloc 
 *              request. If a block is found, a pointer to that block is returned. Otherwise, NULL is returned.
 */
void *best_fit(size_t req_size) {
    void *curr_block = first_fit(req_size);
    size_t curr_size = get_block_size(curr_block);
    void *next_block = curr_block;
    
    while((next_block = get_next_block(next_block)) != NULL) {
        if(!check_alloc(next_block)) {
            size_t next_size = get_block_size(next_block);
            if(next_size >= req_size && next_size < curr_size) {
                curr_block = next_block;
                curr_size = next_size;
            }
        }
    }
    return curr_block;
}
​
//ALLOCATOR FUNCTIONS:
//____________________
​
/* Function: myinit
 * ___________________
 * Parameters:
 *    - heap_start: start of heap segment
 *    - heap_size: size of heap segment
 *
 * Return: true if segment available for use, false otherwise
 *
 * Description: This function initializes global variables for the rest of the program. This includes initializing the start of 
 *              the heap, the heap's size, the first block, the number of total blocks, the number of allocated blocks, and the
 *              total number of bytes used (used by validate_heap).
 */
bool myinit(void *heap_start, size_t heap_size) {
    segment_start = heap_start;
    segment_end = (char *)heap_start + heap_size;
    segment_size = heap_size;
    start_block = (char *)segment_start + WIDTH;
​
    *(unsigned long *)segment_start = ((heap_size - WIDTH) << 2);
    nblocks = 1;
    nused = 0;
    bytes_used = 0;
    return true;
}
​
/* Function: mymalloc
 * ___________________
 * Parameters:
 *    - req_size: requested block size
 *
 * Return: pointer to newly allocated block
 *
 * Description: This function allocates a block on the heap (using first_fit search). If the search is successful, the header of the
 *              block returned by the search is set to the requested size. If there is remaining space, another free block is created
 *              using that extra space. (note that I ended up choosing to allocate "0 byte" free blocks in case of 8 byte free block)
 */
void *mymalloc(size_t req_size) {
    if(req_size <= 0) {
        return NULL;
    }
    
    void *alloc_block = NULL;
    req_size = round_up(req_size);
    
    if((alloc_block = first_fit(req_size)) != NULL){
        size_t size_diff = get_block_size(alloc_block) - req_size;
        set_hdr_size(alloc_block, req_size);
        set_hdr_status(alloc_block, ALLOC);
​
        if(size_diff > 0) {
            void *next_block = get_next_block(alloc_block);
            set_hdr_size(next_block, size_diff - WIDTH);
            set_hdr_status(next_block, FREE);
            nblocks += 1;
        }
        nused += 1;
        bytes_used += (req_size + WIDTH);
        return alloc_block;
    }
    
    return NULL;
}
​
/* Function: myfree
 * ___________________
 * Parameters:
 *    - req_size: ptr
 *
 * Return: N/A
 *
 * Description: If the provided pointer isn't NULL, this function frees the block at that pointer (updates last bit of header).
 */
void myfree(void *ptr) {
    if(ptr != NULL) {
        nused -= 1;
        set_hdr_status(ptr, FREE);
        bytes_used -= (get_block_size(ptr) + WIDTH);
    }
}
​
void *myrealloc(void *old_ptr, size_t new_size) {
    if(old_ptr == NULL) {
        return mymalloc(new_size);
    }
    if(new_size == 0) {
        myfree(old_ptr);
        return NULL;
    }
    void *new_ptr = mymalloc(new_size);
    memcpy(new_ptr, old_ptr, new_size);
    myfree(old_ptr);
    return new_ptr;
}
​
//check that sum of all used and free blocks adds up to heap size
//(implicitly checks that block counts are correct, heap is accounted for)
bool validate_heap() {
    void *curr_block = start_block;
    size_t used_bytes = 0;
    size_t free_bytes = 0;
​
    for(int i = 0; i < nblocks; i++) {
        size_t block_width_size = get_block_size(curr_block) + WIDTH;
        
        if(check_alloc(curr_block)) {
            used_bytes += block_width_size;
        } else {
            free_bytes += block_width_size;
        }
​
        curr_block = get_next_block(curr_block);
    }
​
    if(used_bytes + free_bytes != segment_size) {
        return false;
    }
    return true;
}