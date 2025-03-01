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

#define MARKS_MASK (ALIGNMENT - 1)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and mark bits in to a word*/
#define PACK(size, marks) ((size) | (marks))

/* Read and write a word */
#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (size_t)(val))

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
 * FREE LIST operations
 ********************************************************/

#define NEXT_FREE_OFFSET WSIZE
#define PREV_FREE_OFFSET (WSIZE + PSIZE)
#define NEXT_FREE_PTR(bp) ((void *)GET(HEADER_PTR(bp) + NEXT_FREE_OFFSET))
#define PREV_FREE_PTR(bp) ((void *)GET(HEADER_PTR(bp) + PREV_FREE_OFFSET))

/*
 * Free list head node. use ring list implementaion here to avoid corner case
 */
static void *FREE_LIST = NULL;

static int init_free_list();
static void remove_free_node(void *bp);
static void insert_free_node(void *bp);
static void *search_fit(size_t size);

/*
 * init_free_list - given a sentinel block init a ring dual linked list
 */
static int init_free_list()
{
    FREE_LIST = mem_sbrk(MIN_BLOCK_SIZE);

    if (FREE_LIST == (void *)-1)
    {
        return -1;
    }

    PUT(HEADER_PTR(FREE_LIST) + NEXT_FREE_OFFSET, FREE_LIST);
    PUT(HEADER_PTR(FREE_LIST) + PREV_FREE_OFFSET, FREE_LIST);

    return 0;
}

/*
 * insert_free_node - insert a block to the first node in free list
 */
static void insert_free_node(void *bp)
{
    void *old_first = NEXT_FREE_PTR(FREE_LIST);

    /* link prev node and next node to bp */
    PUT(HEADER_PTR(FREE_LIST) + NEXT_FREE_OFFSET, bp);
    PUT(HEADER_PTR(old_first) + PREV_FREE_OFFSET, bp);

    /* link bp to prev and next node */
    PUT(HEADER_PTR(bp) + NEXT_FREE_OFFSET, old_first);
    PUT(HEADER_PTR(bp) + PREV_FREE_OFFSET, FREE_LIST);
}

/*
 * remove_free_node - remove a node from free list
 */
static void remove_free_node(void *bp)
{
    void *prev = PREV_FREE_PTR(bp);
    void *next = NEXT_FREE_PTR(bp);

    PUT(HEADER_PTR(prev) + NEXT_FREE_OFFSET, next);
    PUT(HEADER_PTR(next) + PREV_FREE_OFFSET, prev);
}

/*
 * search_fit - Given a request size then search a fit block which size is equal or greater than request size
 *      use best fit strategy here
 */
static void *search_fit(size_t request_size)
{
    void *candidate = NULL;
    size_t candidate_size = ~(size_t)0;

    for (void *bp = NEXT_FREE_PTR(FREE_LIST); bp != FREE_LIST; bp = NEXT_FREE_PTR(bp))
    {
        void *header = HEADER_PTR(bp);

        if (GET_ALLOC(header))
        {
            continue;
        }

        size_t size = GET_SIZE(header);

        if (size < request_size)
        {
            continue;
        }

        if (size < candidate_size)
        {
            candidate = bp;
            candidate_size = size;
        }
    }

    return candidate;
}

/*********************************************************
 * block operation helpers
 ********************************************************/

/*
 * Head block, internal used to deal with corner case.
 * valid for allocating from next block to HEAD
 */
static void *HEAD = NULL;

static void *extend_heap(size_t size);
static void *coalesce(void *bp);
static void *place(void *bp, size_t size);

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

    /* new block will be inserted to free list when coalescing */
    return coalesce(bp);
}

/*
 * coalesce - coalesce given free block with potentially free previous block and next block
 *      Free list is maintained here
 */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_PRE_ALLOC(HEADER_PTR(bp));
    size_t next_alloc = GET_ALLOC(HEADER_PTR(NEXT_BLOCK(bp)));
    size_t size = GET_SIZE(HEADER_PTR(bp));

    if (prev_alloc && next_alloc)
    {
        insert_free_node(bp);
    }
    else if (prev_alloc && !next_alloc)
    {
        remove_free_node(NEXT_BLOCK(bp));
        size += GET_SIZE(HEADER_PTR(NEXT_BLOCK(bp)));
        PUT(HEADER_PTR(bp), PACK(size, prev_alloc));
        PUT(FOOTER_PTR(bp), PACK(size, prev_alloc));
        insert_free_node(bp);
    }
    else if (!prev_alloc && next_alloc)
    {
        /* prev block is already in free list */
        size += GET_SIZE(HEADER_PTR(PREV_BLOCK(bp)));
        PUT(FOOTER_PTR(bp), PACK(size, GET_PRE_ALLOC(HEADER_PTR(PREV_BLOCK(bp)))));
        PUT(HEADER_PTR(PREV_BLOCK(bp)), PACK(size, GET_PRE_ALLOC(HEADER_PTR(PREV_BLOCK(bp)))));
        bp = PREV_BLOCK(bp);
    }
    else
    {
        /* prev block is already in free but still need to remove next block */
        remove_free_node(NEXT_BLOCK(bp));
        size += GET_SIZE(HEADER_PTR(NEXT_BLOCK(bp))) + GET_SIZE(HEADER_PTR(PREV_BLOCK(bp)));
        PUT(HEADER_PTR(PREV_BLOCK(bp)), PACK(size, GET_PRE_ALLOC(HEADER_PTR(PREV_BLOCK(bp)))));
        PUT(FOOTER_PTR(NEXT_BLOCK(bp)), PACK(size, GET_PRE_ALLOC(HEADER_PTR(PREV_BLOCK(bp)))));
        bp = PREV_BLOCK(bp);
    }

    return bp;
}

/*
 * place - given a block to be allocated, decide which part return to user
 * if remaining space is greater than minimal block size then block will be split
 * free list also is maintained in this function
 */
static void *place(void *bp, size_t size)
{
    size_t old_size = GET_SIZE(HEADER_PTR(bp));

    if (old_size > size + MIN_BLOCK_SIZE)
    {
        void *header = HEADER_PTR(bp);
        PUT(header, PACK(old_size - size, GET_PRE_ALLOC(header)));
        PUT(FOOTER_PTR(bp), PACK(old_size - size, GET_PRE_ALLOC(header)));

        /* split block */
        bp = NEXT_BLOCK(bp);
        PUT(HEADER_PTR(bp), PACK(size, CUR_ALLOC));
        void *next_header = HEADER_PTR(NEXT_BLOCK(bp));
        PUT(next_header, PACK(GET_SIZE(next_header), PREV_ALLOC | GET_ALLOC(next_header)));
    }
    else
    {
        remove_free_node(bp);
        void *header = HEADER_PTR(bp);
        void *next_header = HEADER_PTR(NEXT_BLOCK(bp));
        PUT(header, PACK(old_size, GET_PRE_ALLOC(header) | CUR_ALLOC));
        PUT(next_header, PACK(GET_SIZE(next_header), PREV_ALLOC | GET_ALLOC(next_header)));
    }

    return bp;
}

/*********************************************************
 * DEBUG
 ********************************************************/

int mm_check()
{
    /* Is every block in the free list marked as free? */
    for (void *bp = NEXT_FREE_PTR(FREE_LIST); bp != FREE_LIST; bp = NEXT_FREE_PTR(bp))
    {
        if (GET_ALLOC(HEADER_PTR(bp)))
        {
            fprintf(stderr, "[Consistency Checker] allocated block in free list (%p)\n", bp);
            return -1;
        }
    }

    /* Are there any contiguous free blocks that somehow escaped coalescing? */
    size_t prev_free = 0;

    for (void *bp = NEXT_BLOCK(HEAD); GET_SIZE(HEADER_PTR(bp)) != 0; bp = NEXT_BLOCK(bp))
    {
        if (prev_free && prev_free == !GET_ALLOC(HEADER_PTR(bp)))
        {
            fprintf(stderr, "[Consistency Checker] contiguous free blocks (%p)\n", bp);
            return -1;
        }
    }

    /* Is every free block actually in the free list? */
    for (void *bp = NEXT_BLOCK(HEAD); GET_SIZE(HEADER_PTR(bp)) != 0; bp = NEXT_BLOCK(bp))
    {
        if (GET_ALLOC(HEADER_PTR(bp)))
        {
            continue;
        }

        size_t hit = 0;

        for (void *free_bp = NEXT_FREE_PTR(FREE_LIST); free_bp != FREE_LIST; free_bp = NEXT_FREE_PTR(free_bp))
        {
            if (free_bp == bp)
            {
                hit = 1;
                break;
            }
        }

        if (!hit)
        {
            fprintf(stderr, "[Consistency Checker] free block is not in the free list (%p)\n", bp);
            return -1;
        }
    }

    /* Do the pointers in the free list point to valid free blocks? */
    for (void *bp = NEXT_FREE_PTR(FREE_LIST); bp != FREE_LIST; bp = NEXT_FREE_PTR(bp))
    {
        size_t *header = (size_t *)HEADER_PTR(bp);
        size_t *footer = (size_t *)FOOTER_PTR(bp);

        if (GET_ALLOC(header))
        {
            fprintf(stderr, "[Consistency Checker] invalid free block in the free list (%p): is allocated \n", bp);
            return -1;
        }

        if (*header != *footer)
        {
            fprintf(stderr, "[Consistency Checker] invalid free block in the free list (%p): header and footer are not consistent\n", bp);
            return -1;
        }

        if (GET_PRE_ALLOC(HEADER_PTR(NEXT_BLOCK(bp))))
        {
            fprintf(stderr, "[Consistency Checker] invalid free block in the free list (%p): next block mark prev allocated\n", bp);
            return -1;
        }
    }

    /* Do any allocated blocks overlap? */
    size_t prev_start = 0;
    size_t prev_end = 0;

    for (void *bp = NEXT_BLOCK(HEAD); GET_SIZE(HEADER_PTR(bp)) != 0; bp = NEXT_BLOCK(bp))
    {
        if (!GET_ALLOC(HEADER_PTR(bp)))
        {
            continue;
        }

        size_t start = (size_t)bp;
        size_t end = (size_t)(HEADER_PTR(bp) + GET_SIZE(HEADER_PTR(bp)));

        if (start < prev_end && start >= prev_start)
        {
            fprintf(stderr, "[Consistency Checker] allocated blocks overlap (%p)\n", bp);
            return -1;
        }

        if (end > prev_start && end <= prev_end)
        {
            fprintf(stderr, "[Consistency Checker] allocated blocks overlap (%p)\n", bp);
            return -1;
        }
    }

    /* Do the pointers in a heap block point to valid heap addresses */
    for (void *bp = HEAD; GET_SIZE(HEADER_PTR(bp)) != 0; bp = NEXT_BLOCK(bp))
    {
        if (bp - mem_heap_lo() < 0 || bp - mem_heap_hi() > 0)
        {
            fprintf(stderr, "[Consistency Checker] next block point to an invalid heap address (%p)\n", bp);
            return -1;
        }

        if ((void *)HEADER_PTR(NEXT_BLOCK(bp)) - mem_heap_lo() < 0 || (void *)HEADER_PTR(NEXT_BLOCK(bp)) - mem_heap_hi() > 0)
        {
            fprintf(stderr, "[Consistency Checker] next block point to an invalid heap address (%p)\n", bp);
            return -1;
        }
    }

    /* Do the pointers in a free list block point to valid heap addresses */
    for (void *bp = NEXT_FREE_PTR(FREE_LIST); bp != FREE_LIST; bp = NEXT_FREE_PTR(bp))
    {
        if (bp - mem_heap_lo() < 0 || bp - mem_heap_hi() > 0)
        {
            fprintf(stderr, "[Consistency Checker] next block point to an invalid heap address (%p)\n", bp);
            return -1;
        }

        if ((void *)HEADER_PTR(NEXT_FREE_PTR(bp)) - mem_heap_lo() < 0 || (void *)HEADER_PTR(NEXT_FREE_PTR(bp)) - mem_heap_hi() > 0)
        {
            fprintf(stderr, "[Consistency Checker] next block point to an invalid heap address (%p)\n", bp);
            return -1;
        }

        if ((void *)HEADER_PTR(PREV_FREE_PTR(bp)) - mem_heap_lo() < 0 || (void *)HEADER_PTR(PREV_FREE_PTR(bp)) - mem_heap_hi() > 0)
        {
            fprintf(stderr, "[Consistency Checker] next block point to an invalid heap address (%p)\n", bp);
            return -1;
        }
    }

    return 0;
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
    mem_sbrk(aligned_heap - cur_heap);

    /* init free list. free list. sentinel blocks always place before HEAD */
    if (init_free_list() == -1)
    {
        return -1;
    }

    /*
     * One block for HEAD, one Epilogue header
     * now this guaranteed no remaining bytes after TAIL block then we can always
     * use new heap address as new block while extending heap
     */
    if ((HEAD = mem_sbrk(MIN_BLOCK_SIZE + HEADER_SIZE)) == (void *)-1)
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

    /* allocated block will be removed from free list when placing */
    return place(bp, size);
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
