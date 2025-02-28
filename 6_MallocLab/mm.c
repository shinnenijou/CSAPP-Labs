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
#include <errno.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Shinnen",
    /* First member's full name */
    "Shinnenijou",
    /* First member's email address */
    "littlesword111@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* Word size. using for header/footer */
#define WSIZE sizeof(size_t)

/* Pointer size. using for linked list link */
#define PSIZE sizeof(void *)

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

/* Header size. consists of block size and alloc mark bits */
/* remaining part of the block is that using for payload or free list links */
#define HEADER_SIZE (ALIGN(WSIZE))

/* Minimal block size. guarantee enough space to store block size of marks or links */
#define MIN_BLOCK_SIZE (ALIGN(WSIZE + PSIZE + PSIZE + WSIZE))

/* typical page size. always extend heap by this amount */
#define CHUNK_SIZE (1 << 12)

/*********************************************************
 * Implicit list implementation
 ********************************************************/

/* indidate current block have been allocated to user */
#define CUR_ALLOC 0x1

/* indidate previous block have been allocated to user */
#define PREV_ALLOC 0x2

#define MARKS_MASK 0x7

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and mark bits in to a word*/
#define PACK(size, marks) ((size) | (marks))

/* Read and write a word */
#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))

/* Read the size and alloc marks from address p */
#define GET_SIZE(p) (GET(p) & ~MARKS_MASK)
#define GET_ALLOC(p) (GET(p) & CUR_ALLOC)
#define GET_PRE_ALLOC(p) (GET(p) & PREV_ALLOC)

/* Given block ptr(ptr to be returning to user), compute address of its header and footer */
#define HEADER_PTR(bp) ((char *)(bp) - HEADER_SIZE)
#define FOOTER_PTR(bp) (HEADER_PTR(bp) + GET_SIZE(HEADER_PTR(bp)) - WSIZE)

/* Given block ptr, compute address of next and previous block */
#define NEXT_BLOCK(bp) ((char *)(bp) + GET_SIZE(HEADER_PTR(bp)))
#define PREV_BLOCK(bp) ((char *)(bp) - GET_SIZE(HEADER_PTR(bp) - WSIZE))

/*********************************************************
 * free list operation functions
 ********************************************************/

static void *extend_heap(size_t size);
static void *coalesce(void *bp);
static void *search_fit(size_t size);
static void *place(void *bp, size_t size);

/*
 * Head block, valid for allocating
 */
static void *HEAD = NULL;

/*********************************************************
 * DEBUG
 ********************************************************/

// static void debug_print_macro()
// {
//     fprintf(stdout, "MIN_BLOCK_SIZE = %d\n", MIN_BLOCK_SIZE);
// }

static void debug_print_list()
{
    int i = 0;
    fprintf(stdout, "======================\n");
    for (void *bp = HEAD; GET_SIZE(HEADER_PTR(bp)) != 0; bp = NEXT_BLOCK(bp))
    {
        size_t alloc = GET_ALLOC(HEADER_PTR(bp));
        size_t prev_alloc = GET_PRE_ALLOC(HEADER_PTR(bp));
        size_t size = GET_SIZE(HEADER_PTR(bp));
        fprintf(stdout, "[%d]free = %d, prev_free = %d, size = %d\n", ++i, !alloc, !prev_alloc, size);
    }
    fprintf(stdout, "======================\n");
}

// static void debug_check_consistency()
// {
//     int prev_free = is_free(HEAD);

//     for (void *block = successor(HEAD); block != TAIL; block = successor(block))
//     {
//         int free = is_free(block);

//         if (free && free == prev_free)
//         {
//             fprintf(stderr, "contiguous free block\n");
//             return;
//         }

//         prev_free = free;
//     }
// }

/*
 * extend_heap - apply for extending heap size to system
 * create a new free block at extending heap then return it
 */
static void *extend_heap(size_t size)
{
    size = ALIGN(size);
    void *bp;

    if ((bp = mem_sbrk(size)) == (void *)-1)
    {
        return NULL;
    }

    /* keep previous block's allocated status */
    PUT(HEADER_PTR(bp), PACK(size, GET_PRE_ALLOC(HEADER_PTR(bp))));
    PUT(FOOTER_PTR(bp), PACK(size, GET_PRE_ALLOC(HEADER_PTR(bp))));

    /* new epilogue block*/
    PUT(HEADER_PTR(NEXT_BLOCK(bp)), PACK(0, CUR_ALLOC));

    return coalesce(bp);
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_PRE_ALLOC(HEADER_PTR(bp));
    size_t next_alloc = GET_ALLOC(HEADER_PTR(NEXT_BLOCK(bp)));
    size_t size = GET_SIZE(HEADER_PTR(bp));

    if (prev_alloc && next_alloc)
    {
        return bp;
    }

    else if (prev_alloc && !next_alloc)
    {
        size += GET_SIZE(HEADER_PTR(NEXT_BLOCK(bp)));
        PUT(HEADER_PTR(bp), PACK(size, prev_alloc));
        PUT(FOOTER_PTR(bp), PACK(size, prev_alloc));
    }
    else if (!prev_alloc && next_alloc)
    {
        size += GET_SIZE(HEADER_PTR(PREV_BLOCK(bp)));
        PUT(FOOTER_PTR(bp), PACK(size, GET_PRE_ALLOC(HEADER_PTR(PREV_BLOCK(bp)))));
        PUT(HEADER_PTR(PREV_BLOCK(bp)), PACK(size, GET_PRE_ALLOC(HEADER_PTR(PREV_BLOCK(bp)))));
        bp = PREV_BLOCK(bp);
    }
    else
    {
        size += GET_SIZE(HEADER_PTR(NEXT_BLOCK(bp))) + GET_SIZE(HEADER_PTR(PREV_BLOCK(bp)));
        PUT(HEADER_PTR(PREV_BLOCK(bp)), PACK(size, GET_PRE_ALLOC(HEADER_PTR(PREV_BLOCK(bp)))));
        PUT(FOOTER_PTR(NEXT_BLOCK(bp)), PACK(size, GET_PRE_ALLOC(HEADER_PTR(PREV_BLOCK(bp)))));
        bp = PREV_BLOCK(bp);
    }

    return bp;
}

static void *search_fit(size_t size)
{
    for (void *bp = HEAD; GET_SIZE(HEADER_PTR(bp)) != 0; bp = NEXT_BLOCK(bp))
    {
        if (GET_ALLOC(HEADER_PTR(bp)))
        {
            continue;
        }

        if (GET_SIZE(HEADER_PTR(bp)) < size)
        {
            continue;
        }

        /* first hit */
        return bp;
    }

    return NULL;
}

static void *place(void *bp, size_t size)
{
    size_t old_size = GET_SIZE(HEADER_PTR(bp));

    if (old_size > size + MIN_BLOCK_SIZE)
    {
        void *header = HEADER_PTR(bp);
        PUT(header, PACK(size, GET_PRE_ALLOC(header) | CUR_ALLOC));

        /* split block */
        PUT(HEADER_PTR(NEXT_BLOCK(bp)), PACK(old_size - size, PREV_ALLOC));
        PUT(FOOTER_PTR(NEXT_BLOCK(bp)), PACK(old_size - size, PREV_ALLOC));
    }
    else
    {
        void *header = HEADER_PTR(bp);
        void *next_header = HEADER_PTR(NEXT_BLOCK(bp));
        PUT(header, PACK(old_size, GET_PRE_ALLOC(header) | CUR_ALLOC));
        PUT(next_header, PACK(GET_SIZE(next_header), PREV_ALLOC | GET_ALLOC(next_header)));
    }

    return bp;
}

/*
 * mm_init - initialize the malloc package.
 * initially allocate one PAGE size
 * use one block for tail to deal with corner case, which will never be freed
 * also need some bytes to align 8 bytes
 */
int mm_init(void)
{
    /* linux sbrk DO NOT guarantee return a aligned address thus need to align the init address carefully */
    void *cur_heap = mem_sbrk(0);
    void *aligned_heap = (void *)ALIGN((size_t)cur_heap);

    /*
     * One block for HEAD, one Epilogue header, plus some bytes to guarantee alignment
     * now this guaranteed no remaining bytes after TAIL block then we can always
     * use new heap address as new block while extending heap
     */
    if ((HEAD = mem_sbrk(MIN_BLOCK_SIZE + HEADER_SIZE + (aligned_heap - cur_heap))) == (void *)-1)
    {
        return -1;
    }

    HEAD = (void *)ALIGN((size_t)HEAD);

    // adjust HEAD point to payload
    HEAD += HEADER_SIZE;
    PUT(HEADER_PTR(HEAD), PACK(MIN_BLOCK_SIZE, CUR_ALLOC));

    /*
     * Epilogue block is a special block which only contains a header
     * then every time extending the heap, Epilogue block become the header of new block
     */
    PUT(HEADER_PTR(NEXT_BLOCK(HEAD)), PACK(0, CUR_ALLOC | PREV_ALLOC));

    // extend the empty heap
    if (extend_heap(CHUNK_SIZE) == NULL)
    {
        return -1;
    }

    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 *
 * search through list to find a best fit block
 */
void *mm_malloc(size_t size)
{
    size = ALIGN(size);
    void *bp;

    if (size == 0)
    {
        return NULL;
    }

    size = MAX(size + HEADER_SIZE, MIN_BLOCK_SIZE);

    /* Try to extend heap if no fit found */
    if ((bp = search_fit(size)) == NULL)
    {
        size_t extend_size = MAX(size, CHUNK_SIZE);

        if ((bp = extend_heap(extend_size)) == NULL)
        {
            return NULL;
        }
    }

    place(bp, size);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 * except HEAD and TAIL
 */
void mm_free(void *bp)
{
    void *hp = HEADER_PTR(bp);
    size_t size = GET_SIZE(hp);

    /* keep previous block's allocated status and set current to free*/
    PUT(hp, PACK(size, GET_PRE_ALLOC(hp)));
    PUT(FOOTER_PTR(bp), PACK(size, GET_PRE_ALLOC(hp)));

    /* set next block's previous block status to free*/
    void *next_hp = HEADER_PTR(NEXT_BLOCK(bp));
    PUT(next_hp, PACK(GET_SIZE(next_hp), GET_ALLOC(next_hp)));

    coalesce(bp);
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
    copySize = *(size_t *)((char *)oldptr - HEADER_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
