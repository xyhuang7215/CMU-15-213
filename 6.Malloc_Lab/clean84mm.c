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

#define GET(p) (*(unsigned int*)(p))
#define PUT(p, val) (*(unsigned int*)(p) = val)
#define SET(p, p2) (*(char **)p = p2)

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) - WSIZE + GET_SIZE(HDRP(bp)) - WSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))

/* Global variables */
char *bp_start, *bp_end;

static inline void push_front(void *bp)
{
    bp_start = bp;
}

/* merge neighbor blocks, and add bp to free list */
void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {
      push_front(bp);
      return bp;
    }

    /* next block is free */
    if (prev_alloc && !next_alloc) {
      char *next_bp = NEXT_BLKP(bp);
      size += GET_SIZE(HDRP(next_bp));
      PUT(HDRP(bp), PACK(size, 0));
      PUT(FTRP(bp), PACK(size, 0));
      push_front(bp);
      return bp;
    }

    /* previous block is free */
    if (!prev_alloc && next_alloc) {
      char *prev_bp = PREV_BLKP(bp);
      size += GET_SIZE(HDRP(prev_bp)); 
      bp = PREV_BLKP(bp);
      PUT(HDRP(bp), PACK(size, 0));
      PUT(FTRP(bp), PACK(size, 0));
      push_front(bp);
      return bp;
    }

    /* neighors are both free */
    if (!prev_alloc && !next_alloc) {
      char *prev_bp = PREV_BLKP(bp);
      char *next_bp = NEXT_BLKP(bp);
      size += GET_SIZE(HDRP(prev_bp)) +
              GET_SIZE(HDRP(next_bp));

      bp = PREV_BLKP(bp);
      PUT(HDRP(bp), PACK(size, 0));
      PUT(FTRP(bp), PACK(size, 0));
      push_front(bp);
      return bp;
    }

    return NULL;
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

    bp_end += size;
    return coalesce(bp);
}

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
    bp_start = heap + 2 * WSIZE;
    bp_end = heap + 4 * WSIZE;
    return 0;
}

char *find_fit(size_t asize)
{
    for (char *bp = bp_start; bp < bp_end; bp = NEXT_BLKP(bp)) {
      if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize)
        return bp;
    }
    return NULL;
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
    coalesce(free_bp);
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    if (size == 0)
      return NULL;

    size_t asize = ALIGN(size + DSIZE); // overhead

    char *bp;
    if ((bp = find_fit(asize)) != NULL) {
      place(bp, asize);
      return bp;
    }

    size_t extendsize = asize;

    if ((bp = extend_heap(extendsize)) == NULL) 
      return NULL;
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;

    copySize = GET_SIZE(HDRP(ptr)) - WSIZE;
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














