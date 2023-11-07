/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "team",
    /* First member's full name */
    "XinYao",
    /* First member's email address */
    "ms0007215@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


/* MACROs */
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)

#define MAX(x, y) ((x) > (y)) ? (x) : (y)

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned *)(p))
#define PUT(p, val) (*(unsigned *)(p) = (unsigned) val)

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) - WSIZE + GET_SIZE(HDRP(bp)) - WSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))

/* Get the address to store next or previous free block from a given block pointer */
#define NEXT_FPTR(bp) ((char *)(bp))
#define PREV_FPTR(bp) ((char *)(bp) + WSIZE)

/* Get the block pointer to next or previous free block */
#define NEXT_FRBLKP(bp) ((char *)(*((unsigned *) NEXT_FPTR(bp))))
#define PREV_FRBLKP(bp) ((char *)(*((unsigned *) PREV_FPTR(bp))))


/* Global variables */
char *free_bp_start;

/* Interface for allocator*/
int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *ptr);
void *mm_realloc(void *ptr, size_t size);

/* Helper function for maintaining context of blocks */
static void *extend_heap(size_t size);
static void *coalesce(void *bp);
static void place(char *bp, size_t asize);

/* Helper function for maintaining free list of blocks */
static char *find_first_fit(size_t asize);
static void push_front_list(void *bp);
static void pop_from_list(void *bp);


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    char *heap;
    if ((heap = mem_sbrk(4 * WSIZE)) == (void *)-1)
      return -1;
    PUT(heap, 0);                            // not use
    PUT(heap + (1 * WSIZE), PACK(DSIZE, 1)); // prologue header
    PUT(heap + (2 * WSIZE), PACK(DSIZE, 1)); // prologue footer
    PUT(heap + (3 * WSIZE), PACK(0, 1));     // Epilogue
    free_bp_start = NULL;
    return 0;
}


/* 
 * mm_malloc - Allocate a block from free list or by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    if (size == 0) return NULL;
    size_t asize = ALIGN(size + DSIZE); // demanded size + overhead

    char *bp;
    if ((bp = find_first_fit(asize)) != NULL) {
      pop_from_list(bp);
      place(bp, asize);
      return bp;
    }

    if ((bp = extend_heap(asize)) != NULL) {
      place(bp, asize);
      return bp;
    }

    return NULL;
}


/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    ptr = coalesce(ptr);
    push_front_list(ptr);
}


/*
 * mm_realloc - Simply allocate a new space and copy context to it. 
 *    Previous space is added to free_list.
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (!ptr) return NULL;

    void *old_ptr = ptr;
    void *new_ptr = mm_malloc(size);
    size_t orig_size = GET_SIZE(HDRP(ptr)) - WSIZE;
    
    if (!new_ptr)
      return NULL;
    if (size < orig_size)
      orig_size = size;

    memcpy(new_ptr, old_ptr, orig_size);
    mm_free(old_ptr);
    return new_ptr;
}


void *extend_heap(size_t size)
{
    size = ALIGN(size);

    char *bp;
    if ((void *)(bp = mem_sbrk(size)) == (void *)-1)
      return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    PUT(NEXT_FPTR(bp), 0);
    PUT(PREV_FPTR(bp), 0);

    return coalesce(bp);
}


/* merge neighbor blocks, remove them from free list if necessary */
void *coalesce(void *bp)
{
    char *prev_bp = PREV_BLKP(bp);
    char *next_bp = NEXT_BLKP(bp);
    size_t prev_alloc = GET_ALLOC(HDRP(prev_bp));
    size_t next_alloc = GET_ALLOC(HDRP(next_bp));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {
      ; // do nothing
    }

    else if (prev_alloc && !next_alloc) {
      size += GET_SIZE(HDRP(next_bp));
      PUT(HDRP(bp), PACK(size, 0));
      PUT(FTRP(bp), PACK(size, 0));
      pop_from_list(next_bp);
    } 

    else if (!prev_alloc && next_alloc) {
      size += GET_SIZE(HDRP(prev_bp)); 
      PUT(HDRP(prev_bp), PACK(size, 0));
      PUT(FTRP(prev_bp), PACK(size, 0));
      pop_from_list(prev_bp);
      bp = PREV_BLKP(bp);
    }

    else if (!prev_alloc && !next_alloc) {
      size += GET_SIZE(HDRP(prev_bp))+
              GET_SIZE(HDRP(next_bp));

      PUT(HDRP(prev_bp), PACK(size, 0));
      PUT(FTRP(prev_bp), PACK(size, 0));
      pop_from_list(prev_bp);
      pop_from_list(next_bp);
      bp = PREV_BLKP(bp);
    }

    return bp;
}


void place(char *bp, size_t asize)
{
    size_t orig_size = GET_SIZE(HDRP(bp));

    if (orig_size - asize <= 2 * DSIZE) {   // 2 * DSIZE : minimum block size
      PUT(HDRP(bp), PACK(orig_size, 1));
      PUT(FTRP(bp), PACK(orig_size, 1));
      return;
    }

    // only use partial
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));

    // handle the remain space
    char *free_bp = NEXT_BLKP(bp);
    size_t free_size = orig_size - asize;
    PUT(HDRP(free_bp), PACK(free_size, 0));
    PUT(FTRP(free_bp), PACK(free_size, 0));
    free_bp = coalesce(free_bp);
    push_front_list(free_bp);
}


char *find_first_fit(size_t asize)
{
    char *bp = free_bp_start;
    while (bp != NULL) {
      if (GET_SIZE(HDRP(bp)) >= asize) {
        return bp;
      }
      bp = (char *) GET(bp);
    }
    return NULL;
}


void push_front_list(void *bp)
{
    PUT(PREV_FPTR(bp), 0);
    PUT(NEXT_FPTR(bp), free_bp_start);
    
    if (free_bp_start != NULL) {
      PUT(PREV_FPTR(free_bp_start), bp);
    }

    free_bp_start = bp;
}


void pop_from_list(void *bp)
{
    char *prev = PREV_FRBLKP(bp);
    char *next = NEXT_FRBLKP(bp);

    if (prev && next) {
      PUT(NEXT_FPTR(prev), next);
      PUT(PREV_FPTR(next), prev);
    }

    else if (prev && !next) {
      PUT(NEXT_FPTR(prev), 0);
    }

    else if (!prev && next) {
      PUT(PREV_FPTR(next), 0);
      free_bp_start = next;
    }

    else if (!prev && !next) {
      free_bp_start = NULL;
    }
}