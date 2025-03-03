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

/* double word (8) alignment */
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
#define MIN(x, y) ((x) < (y) ? (x) : (y))

/* Pack a size and mark bits in to a word*/
#define PACK(size, marks) ((size) | (marks))

/* Read and write a word */
#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (size_t)(val))

/* Read the size and alloc marks from address p */
#define GET_SIZE(p) (GET(p) & ~MARKS_MASK)
#define NEW_SIZE(p, val) ((GET(p) & MARKS_MASK) | (val))

#define GET_ALLOC(p) (GET(p) & CUR_ALLOC)
#define MARK_ALLOC(val) ((val) | CUR_ALLOC)
#define MARK_FREE(val) ((val) & ~CUR_ALLOC)

#define GET_PRE_ALLOC(p) (GET(p) & PREV_ALLOC)
#define MARK_PRE_ALLOC(val) ((val) | PREV_ALLOC)
#define MARK_PRE_FREE(val) ((val) & ~PREV_ALLOC)

/* Given block ptr(ptr to be returning to user), compute address of its header and footer */
#define HEADER_PTR(bp) ((char *)(bp) - HEADER_SIZE)
#define FOOTER_PTR(bp) (HEADER_PTR(bp) + GET_SIZE(HEADER_PTR(bp)) - WSIZE)

/* Given block ptr, compute address of next and previous block */
#define NEXT_BLOCK(bp) ((char *)(bp) + GET_SIZE(HEADER_PTR(bp)))
#define PREV_BLOCK(bp) ((char *)(bp) - GET_SIZE(HEADER_PTR(bp) - WSIZE))

/*********************************************************
 * FREE LIST operations
 ********************************************************/

/*
 * Free list head node. use ring list implementaion here to avoid corner case
 */
static char *FREE_LIST_BEGIN = NULL;

/*
 * Defined how much the size so-called 'large' to put into the last segregated list
 * All block equal or larger than this size will be put into the last list
 */
#define LARGE_BLOCK_SIZE 136
#define SMALL_BLOCK_NUM 32

#define NEXT_FREE_OFFSET WSIZE
#define PREV_FREE_OFFSET (WSIZE + PSIZE)
#define NEXT_FREE_PTR(bp) ((void *)GET(HEADER_PTR(bp) + NEXT_FREE_OFFSET))
#define PREV_FREE_PTR(bp) ((void *)GET(HEADER_PTR(bp) + PREV_FREE_OFFSET))
#define FREE_LIST(size) ((char *)FREE_LIST_BEGIN + ((MIN(size, LARGE_BLOCK_SIZE) - MIN_BLOCK_SIZE) / ALIGNMENT) * MIN_BLOCK_SIZE)

static int init_free_list();
static void *split_to_small(void *bp, size_t size);
static void remove_free_node(void *bp);
static void insert_free_node(void *bp);
static void *search_fit(size_t size);

/*
 * init_free_list - given a sentinel block init a ring dual linked list
 */
static int init_free_list()
{
    /* create segregated list by 8 bytes unit */
    size_t free_list_num = (LARGE_BLOCK_SIZE - MIN_BLOCK_SIZE) / ALIGNMENT + 1;
    FREE_LIST_BEGIN = mem_sbrk(free_list_num * MIN_BLOCK_SIZE);

    if (FREE_LIST_BEGIN == (void *)-1)
    {
        return -1;
    }

    /* Adjust to point to payload address */
    FREE_LIST_BEGIN += HEADER_SIZE;

    for (size_t size = MIN_BLOCK_SIZE; size <= LARGE_BLOCK_SIZE; size += ALIGNMENT)
    {
        void *free_list = FREE_LIST(size);
        PUT(HEADER_PTR(free_list), 0);
        PUT(HEADER_PTR(free_list) + NEXT_FREE_OFFSET, free_list);
        PUT(HEADER_PTR(free_list) + PREV_FREE_OFFSET, free_list);
    }

    return 0;
}

/* split_to_small - split a large free block to several small blocks then insert to free list */
static void *split_to_small(void *bp, size_t size)
{
    remove_free_node(bp);

    /* backup */
    void *first_bp = bp;
    size_t prev_alloc = GET_PRE_ALLOC(HEADER_PTR(bp));
    size_t rest_size = GET_SIZE(HEADER_PTR(bp));

    while (rest_size >= MIN_BLOCK_SIZE + size)
    {
        PUT(HEADER_PTR(bp), MARK_PRE_FREE(MARK_FREE(size)));
        PUT(FOOTER_PTR(bp), GET(HEADER_PTR(bp)));
        insert_free_node(bp);
        rest_size -= size;
        bp = NEXT_BLOCK(bp);
    }

    /* last block */
    PUT(HEADER_PTR(bp), MARK_PRE_FREE(MARK_FREE(rest_size)));
    PUT(FOOTER_PTR(bp), GET(HEADER_PTR(bp)));
    insert_free_node(bp);

    /* restore first block */
    PUT(HEADER_PTR(first_bp), GET(HEADER_PTR(first_bp)) | prev_alloc);
    PUT(FOOTER_PTR(first_bp), GET(HEADER_PTR(first_bp)));

    return first_bp;
}

/*
 * insert_free_node - insert a block to the first node in free list
 */
static void insert_free_node(void *bp)
{
    size_t size = GET_SIZE(HEADER_PTR(bp));
    void *free_list = FREE_LIST(size);

    void *old_first = NEXT_FREE_PTR(free_list);

    /* link prev node and next node to bp */
    PUT(HEADER_PTR(free_list) + NEXT_FREE_OFFSET, bp);
    PUT(HEADER_PTR(old_first) + PREV_FREE_OFFSET, bp);

    /* link bp to prev and next node */
    PUT(HEADER_PTR(bp) + NEXT_FREE_OFFSET, old_first);
    PUT(HEADER_PTR(bp) + PREV_FREE_OFFSET, free_list);
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
 */
static void *search_fit(size_t request_size)
{
    void *bp = NULL;

    /* first hit for small block */
    if (request_size < LARGE_BLOCK_SIZE)
    {
        void *free_list = FREE_LIST(request_size);
        void *first_bp = NEXT_FREE_PTR(free_list);

        if (first_bp != free_list)
        {
            bp = first_bp;
        }
    }
    else
    {
        void *free_list = FREE_LIST(LARGE_BLOCK_SIZE);
        void *candidate = NULL;
        size_t candidate_size = 0;

        for (void *free_bp = NEXT_FREE_PTR(free_list); free_bp != free_list; free_bp = NEXT_FREE_PTR(free_bp))
        {
            size_t size = GET_SIZE(HEADER_PTR(free_bp));

            if (size < request_size)
            {
                continue;
            }

            if (size > candidate_size)
            {
                candidate = free_bp;
                candidate_size = size;
            }
        }

        bp = candidate;
    }

    return bp;
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
 * extend_heap - apply for extending heap size to system.
 * create a new free block at extending heap then return it.
 * new block may be larger than given size for min size reason
 */
static void *extend_heap(size_t size)
{
    size = ALIGN(size);
    size = MAX(size, MIN_BLOCK_SIZE);
    void *bp;

    if ((bp = mem_sbrk(size)) == (void *)-1)
    {
        return NULL;
    }

    /* keep previous block's allocated status */
    PUT(HEADER_PTR(bp), MARK_FREE(NEW_SIZE(HEADER_PTR(bp), size)));
    PUT(FOOTER_PTR(bp), GET(HEADER_PTR(bp)));

    /* new epilogue block*/
    PUT(HEADER_PTR(NEXT_BLOCK(bp)), MARK_ALLOC(0));

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

    if (!next_alloc)
    {
        remove_free_node(NEXT_BLOCK(bp));
        size += GET_SIZE(HEADER_PTR(NEXT_BLOCK(bp)));
        PUT(HEADER_PTR(bp), NEW_SIZE(HEADER_PTR(bp), size));
        PUT(FOOTER_PTR(bp), GET(HEADER_PTR(bp)));
    }

    if (!prev_alloc)
    {
        bp = PREV_BLOCK(bp);
        remove_free_node(bp);
        size += GET_SIZE(HEADER_PTR(bp));
        PUT(HEADER_PTR(bp), NEW_SIZE(HEADER_PTR(bp), size));
        PUT(FOOTER_PTR(bp), GET(HEADER_PTR(bp)));
    }

    insert_free_node(bp);

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

    remove_free_node(bp);

    if (old_size > size + MIN_BLOCK_SIZE)
    {
        void *header = HEADER_PTR(bp);
        PUT(header, MARK_ALLOC(NEW_SIZE(header, size)));

        /* split block */
        void *next_bp = NEXT_BLOCK(bp);
        PUT(HEADER_PTR(next_bp), MARK_PRE_ALLOC(MARK_FREE(NEW_SIZE(HEADER_PTR(next_bp), old_size - size))));
        PUT(FOOTER_PTR(next_bp), GET(HEADER_PTR(next_bp)));
        insert_free_node(next_bp);
    }
    else
    {
        void *header = HEADER_PTR(bp);
        void *next_header = HEADER_PTR(NEXT_BLOCK(bp));

        PUT(header, MARK_ALLOC(GET(header)));
        PUT(next_header, MARK_PRE_ALLOC(GET(next_header)));
    }

    return bp;
}

/*********************************************************
 * DEBUG
 ********************************************************/

#define DEBUG_MARK 0x4
#define HAS_DEBUG_MARK(p) (GET(p) & DEBUG_MARK)

static void print_free_list()
{
    fprintf(stdout, "--------------------------\n");

    for (size_t size = MIN_BLOCK_SIZE; size <= LARGE_BLOCK_SIZE; size += ALIGNMENT)
    {
        void *free_list = FREE_LIST(size);

        if (NEXT_FREE_PTR(free_list) == free_list)
        {
            continue;
        }

        fprintf(stdout, "[size = %d]{(%p)size = %d}", size, free_list, GET_SIZE(HEADER_PTR(free_list)));

        for (void *bp = NEXT_FREE_PTR(free_list); bp != free_list; bp = NEXT_FREE_PTR(bp))
        {
            fprintf(stdout, " <-> {(%p)size = %d}", bp, GET_SIZE(HEADER_PTR(bp)));
        }

        fprintf(stdout, "\n");
    }

    fflush(stdout);
}

/* Is every block in the free list marked as free? */
static int check_free_list_all_free(void *free_list)
{
    for (void *bp = NEXT_FREE_PTR(free_list); bp != free_list; bp = NEXT_FREE_PTR(bp))
    {
        if (GET_ALLOC(HEADER_PTR(bp)))
        {
            fprintf(stderr, "[Consistency Checker]allocated block in free list (%p)\n", bp);
            return -1;
        }
    }

    return 0;
}

/* Is every block in the free list has the same size? */
static int check_free_list_same_size(void *free_list, size_t size)
{
    for (void *bp = NEXT_FREE_PTR(free_list); bp != free_list; bp = NEXT_FREE_PTR(bp))
    {
        if (GET_SIZE(HEADER_PTR(bp)) != size)
        {
            fprintf(stderr, "[Consistency Checker]wrong size in free list (%p), expect: %d, get: %d\n", bp, size, GET_SIZE(HEADER_PTR(bp)));
            return -1;
        }
    }

    return 0;
}

static int mark_block_in_free_list(void *free_list, int mark)
{
    for (void *bp = NEXT_FREE_PTR(free_list); bp != free_list; bp = NEXT_FREE_PTR(bp))
    {
        if (mark)
        {
            PUT(HEADER_PTR(bp), GET(HEADER_PTR(bp)) | DEBUG_MARK);
        }
        else
        {
            PUT(HEADER_PTR(bp), GET(HEADER_PTR(bp)) & ~DEBUG_MARK);
        }
    }

    return 0;
}

int mm_check()
{
    for (size_t size = MIN_BLOCK_SIZE; size <= LARGE_BLOCK_SIZE; size += ALIGNMENT)
    {
        if (check_free_list_all_free(FREE_LIST(size)) != 0)
        {
            return -1;
        }
    }

    for (size_t size = MIN_BLOCK_SIZE; size < LARGE_BLOCK_SIZE; size += ALIGNMENT)
    {
        if (check_free_list_same_size(FREE_LIST(size), size) != 0)
        {
            return -1;
        }
    }

    /* Are there any contiguous free blocks that somehow escaped coalescing? */
    {
        size_t prev_free = 0;

        for (void *bp = NEXT_BLOCK(HEAD); GET_SIZE(HEADER_PTR(bp)) != 0; bp = NEXT_BLOCK(bp))
        {
            if (prev_free && prev_free == !GET_ALLOC(HEADER_PTR(bp)))
            {
                fprintf(stderr, "[Consistency Checker]contiguous free blocks (%p)\n", bp);
                return -1;
            }
        }
    }

    /* Is every free block actually in the free list? */
    {
        for (size_t size = MIN_BLOCK_SIZE; size <= LARGE_BLOCK_SIZE; size += ALIGNMENT)
        {
            mark_block_in_free_list(FREE_LIST(size), 1);
        }

        for (void *bp = NEXT_BLOCK(HEAD); GET_SIZE(HEADER_PTR(bp)) != 0; bp = NEXT_BLOCK(bp))
        {
            if (GET_ALLOC(HEADER_PTR(bp)))
            {
                continue;
            }

            if (!HAS_DEBUG_MARK(HEADER_PTR(bp)))
            {
                fprintf(stderr, "[Consistency Checker]free block not in free list (%p), size = %d\n", bp, GET_SIZE(HEADER_PTR(bp)));
                return -1;
            }
        }

        for (size_t size = MIN_BLOCK_SIZE; size <= LARGE_BLOCK_SIZE; size += ALIGNMENT)
        {
            mark_block_in_free_list(FREE_LIST(size), 0);
        }
    }

    //     /* Do the pointers in the free list point to valid free blocks? */
    //     for (void *bp = NEXT_FREE_PTR(FREE_LIST); bp != FREE_LIST; bp = NEXT_FREE_PTR(bp))
    //     {
    //         size_t *header = (size_t *)HEADER_PTR(bp);
    //         size_t *footer = (size_t *)FOOTER_PTR(bp);

    //         if (GET_ALLOC(header))
    //         {
    //             fprintf(stderr, "[Consistency Checker] invalid free block in the free list (%p): is allocated \n", bp);
    //             return -1;
    //         }

    //         if (*header != *footer)
    //         {
    //             fprintf(stderr, "[Consistency Checker] invalid free block in the free list (%p): header and footer are not consistent\n", bp);
    //             return -1;
    //         }

    //         if (GET_PRE_ALLOC(HEADER_PTR(NEXT_BLOCK(bp))))
    //         {
    //             fprintf(stderr, "[Consistency Checker] invalid free block in the free list (%p): next block mark prev allocated\n", bp);
    //             return -1;
    //         }
    //     }

    //     /* Do any allocated blocks overlap? */
    //     size_t prev_start = 0;
    //     size_t prev_end = 0;

    //     for (void *bp = NEXT_BLOCK(HEAD); GET_SIZE(HEADER_PTR(bp)) != 0; bp = NEXT_BLOCK(bp))
    //     {
    //         if (!GET_ALLOC(HEADER_PTR(bp)))
    //         {
    //             continue;
    //         }

    //         size_t start = (size_t)bp;
    //         size_t end = (size_t)(HEADER_PTR(bp) + GET_SIZE(HEADER_PTR(bp)));

    //         if (start < prev_end && start >= prev_start)
    //         {
    //             fprintf(stderr, "[Consistency Checker] allocated blocks overlap (%p)\n", bp);
    //             return -1;
    //         }

    //         if (end > prev_start && end <= prev_end)
    //         {
    //             fprintf(stderr, "[Consistency Checker] allocated blocks overlap (%p)\n", bp);
    //             return -1;
    //         }
    //     }

    //     /* Do the pointers in a heap block point to valid heap addresses */
    //     for (void *bp = HEAD; GET_SIZE(HEADER_PTR(bp)) != 0; bp = NEXT_BLOCK(bp))
    //     {
    //         if (bp - mem_heap_lo() < 0 || bp - mem_heap_hi() > 0)
    //         {
    //             fprintf(stderr, "[Consistency Checker] next block point to an invalid heap address (%p)\n", bp);
    //             return -1;
    //         }

    //         if ((void *)HEADER_PTR(NEXT_BLOCK(bp)) - mem_heap_lo() < 0 || (void *)HEADER_PTR(NEXT_BLOCK(bp)) - mem_heap_hi() > 0)
    //         {
    //             fprintf(stderr, "[Consistency Checker] next block point to an invalid heap address (%p)\n", bp);
    //             return -1;
    //         }
    //     }

    //     /* Do the pointers in a free list block point to valid heap addresses */
    //     for (void *bp = NEXT_FREE_PTR(FREE_LIST); bp != FREE_LIST; bp = NEXT_FREE_PTR(bp))
    //     {
    //         if (bp - mem_heap_lo() < 0 || bp - mem_heap_hi() > 0)
    //         {
    //             fprintf(stderr, "[Consistency Checker] next block point to an invalid heap address (%p)\n", bp);
    //             return -1;
    //         }

    //         if ((void *)HEADER_PTR(NEXT_FREE_PTR(bp)) - mem_heap_lo() < 0 || (void *)HEADER_PTR(NEXT_FREE_PTR(bp)) - mem_heap_hi() > 0)
    //         {
    //             fprintf(stderr, "[Consistency Checker] next block point to an invalid heap address (%p)\n", bp);
    //             return -1;
    //         }

    //         if ((void *)HEADER_PTR(PREV_FREE_PTR(bp)) - mem_heap_lo() < 0 || (void *)HEADER_PTR(PREV_FREE_PTR(bp)) - mem_heap_hi() > 0)
    //         {
    //             fprintf(stderr, "[Consistency Checker] next block point to an invalid heap address (%p)\n", bp);
    //             return -1;
    //         }
    //     }

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
    PUT(HEADER_PTR(HEAD), MARK_ALLOC(MIN_BLOCK_SIZE));

    /*
     * Epilogue block is a special block which only contains a header
     * then every time extending the heap, Epilogue block become the header of new block
     */
    PUT(HEADER_PTR(NEXT_BLOCK(HEAD)), MARK_PRE_ALLOC(MARK_ALLOC(0)));

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
        if (size < LARGE_BLOCK_SIZE)
        {
            bp = extend_heap(size * SMALL_BLOCK_NUM);
            bp = split_to_small(bp, size);
        }
        else if (size < CHUNK_SIZE)
        {
            bp = extend_heap(size);
        }
        else
        {
            size += ALIGN(size >> 3);
            bp = extend_heap(size);
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
    /* keep previous block's allocated status and set current to free*/
    PUT(HEADER_PTR(bp), MARK_FREE(GET(HEADER_PTR(bp))));
    PUT(FOOTER_PTR(bp), GET(HEADER_PTR(bp)));

    /* set next block's previous block status to free*/
    PUT(HEADER_PTR(NEXT_BLOCK(bp)), MARK_PRE_FREE(GET(HEADER_PTR(NEXT_BLOCK(bp)))));

    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (size == 0)
    {
        mm_free(ptr);
        return NULL;
    }

    void *old_ptr = ptr;
    size_t old_size = GET_SIZE(HEADER_PTR(old_ptr));

    size_t new_size = ALIGN(size) + HEADER_SIZE;
    new_size = MAX(new_size, MIN_BLOCK_SIZE);

    /* next block is allocated or is free but space still not enough. free current block then malloc a new*/
    if ((old_size < new_size) && (GET_ALLOC(HEADER_PTR(NEXT_BLOCK(old_ptr))) || (GET_SIZE(HEADER_PTR(NEXT_BLOCK(old_ptr))) + old_size < new_size)))
    {
        /*
         * this means current block is the last block. We can simply extend heap in this case
         * then the space will be enough after coalescing with new block
         */
        if (GET_SIZE(HEADER_PTR(NEXT_BLOCK(old_ptr))) == 0)
        {
            extend_heap(new_size - old_size);
        }
        else
        {
            void *new_ptr = mm_malloc(new_size);

            if (new_ptr == NULL)
            {
                return NULL;
            }

            memcpy(new_ptr, old_ptr, MIN(new_size - HEADER_SIZE, old_size - HEADER_SIZE));
            mm_free(old_ptr);
            return new_ptr;
        }
    }

    /*
     * try to coalesc with next block.
     * a safe and easy way is to store some user data in the stack then coalesce
     * then recover data after spliting
     */
    size_t old_next = GET(HEADER_PTR(old_ptr) + NEXT_FREE_OFFSET);
    size_t old_prev = GET(HEADER_PTR(old_ptr) + PREV_FREE_OFFSET);

    /* keep previous block's allocated status and set current to free*/
    PUT(HEADER_PTR(old_ptr), MARK_FREE(GET(HEADER_PTR(old_ptr))));

    /* set next block's previous block status to free*/
    PUT(HEADER_PTR(NEXT_BLOCK(old_ptr)), MARK_PRE_FREE(GET(HEADER_PTR(NEXT_BLOCK(old_ptr)))));

    /* try to coalesce with next block */
    if (!GET_ALLOC(HEADER_PTR(NEXT_BLOCK(old_ptr))))
    {
        remove_free_node(NEXT_BLOCK(old_ptr));
        PUT(HEADER_PTR(old_ptr), NEW_SIZE(HEADER_PTR(old_ptr), old_size + GET_SIZE(HEADER_PTR(NEXT_BLOCK(old_ptr)))));
        PUT(FOOTER_PTR(old_ptr), GET(HEADER_PTR(old_ptr)));
    }

    insert_free_node(old_ptr);
    old_ptr = place(old_ptr, new_size);

    PUT(HEADER_PTR(old_ptr) + NEXT_FREE_OFFSET, old_next);
    PUT(HEADER_PTR(old_ptr) + PREV_FREE_OFFSET, old_prev);

    return old_ptr;
}
