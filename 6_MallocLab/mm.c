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

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define HEADER_SIZE SIZE_T_SIZE

/*********************************************************
 * Implicit list implementation
 ********************************************************/

// use least significant bit as current block allocated mark
static const size_t CUR_ALLOC_MASK = 0x1;

// use second significant bit as previous block allocated mark
static const size_t PREV_ALLOC_MASK = 0x2;

// third bit is not used
static const size_t MARKS_MASK = ALIGNMENT - 1;
static const size_t SIZE_MASK = ~MARKS_MASK;

// pointer manipulation helper
#define HEADER(block) *(size_t *)(block)
#define PREV_FOOTER(block) *(size_t *)((char *)(block) - SIZE_T_SIZE)
#define FORWARD(ptr, bytes) (void *)((char *)(ptr) + (bytes))
#define BACKWARD(ptr, bytes) (void *)((char *)(ptr) - (bytes))

/*
 *  Allocated bit operation
 */
static int IS_ALLOCATED(void *p)
{
    return (HEADER(p) & CUR_ALLOC_MASK) != 0;
}

/*
 *  Allocated bit operation
 */
static int IS_PREV_ALLOCATED(void *p)
{
    return (HEADER(p) & PREV_ALLOC_MASK) != 0;
}

/*
 *  Allocated bit operation
 */
static void SET_FREE(void *p)
{
    HEADER(p) &= ~CUR_ALLOC_MASK;
}

/*
 *  Allocated bit operation
 */
static void SET_ALLOC(void *p)
{
    HEADER(p) |= CUR_ALLOC_MASK;
}

/*
 *  Allocated bit operation
 */
static void SET_PREV_FREE(void *p)
{
    HEADER(p) &= ~PREV_ALLOC_MASK;
}

/*
 *  Allocated bit operation
 */
static void SET_PREV_ALLOC(void *p)
{
    HEADER(p) |= PREV_ALLOC_MASK;
}

/*
 * block size operation
 */
static size_t GET_SIZE(void *p)
{
    return HEADER(p) & SIZE_MASK;
}

/*
 * block size operation. valid iff previous block is free
 */
static size_t GET_PREV_SIZE(void *p)
{
    return PREV_FOOTER(p) & SIZE_MASK;
}

static void SET_SIZE(void *p, size_t size)
{
    size_t header = HEADER(p);
    header &= MARKS_MASK;
    header |= size;
    HEADER(p) = header;
}

/*
 * list operation
 */
static void *NEXT_BLOCK(void *p)
{
    return FORWARD(p, GET_SIZE(p));
}

/*
 * list operation. valid iff previous block is free
 */
static void *PREV_BLOCK(void *p)
{
    return BACKWARD(p, GET_PREV_SIZE(p));
}

/*
 * copy header to copy when free a block
 */
static void COPY_TO_FOOTER(void *p)
{
    PREV_FOOTER(NEXT_BLOCK(p)) = HEADER(p);
}

static void *GET_PAYLOAD(void *p)
{
    return FORWARD(p, HEADER_SIZE);
}

static void *PAYLOAD_TO_BLOCK(void *p)
{
    return BACKWARD(p, HEADER_SIZE);
}

/*********************************************************
 * Implicit list helper function
 ********************************************************/

static void *mm_increment(size_t size);
static void *mm_coalesce(void *first_block, void *second_block);
static void mm_set_block_free(void *block, size_t size);
static void mm_set_block_alloc(void *block, size_t size);
static void *mm_search_block(void *begin, size_t need_size);

/*
 * List head
 */
static void *HEAD = NULL;
static void *TAIL = NULL;
/*********************************************************
 * DEBUG
 ********************************************************/

static void print_list()
{
    int i = 0;
    fprintf(stdout, "======================\n");
    for (void *block = HEAD; block != TAIL; block = NEXT_BLOCK(block))
    {
        if (GET_SIZE(block) == 0)
            break;
        fprintf(stdout, "[%d]free = %d, prev_free = %d, size = %d\n", ++i, !IS_ALLOCATED(block), !IS_PREV_ALLOCATED(block), GET_SIZE(block));
    }
    fprintf(stdout, "======================\n");
}

static void check_consistency()
{
    int prev_free = !IS_ALLOCATED(HEAD);

    for (void *block = NEXT_BLOCK(HEAD); block != TAIL; block = NEXT_BLOCK(block))
    {
        int free = !IS_ALLOCATED(block);

        if (free && free == prev_free)
        {
            fprintf(stderr, "contiguous free block\n");
            return;
        }

        prev_free = free;
    }
}

/*
 * increment heap size. always increment one page(typically 4KB)
 * firstly increase TAIL block and set new TAIL
 * then try to coalesce new block and previous block
 * do nothing with HEAD block (unless HEAD is exactly the previous block)
 * return new free block
 */
static void *mm_increment(size_t size)
{
    if (mem_sbrk(size) == (void *)-1)
    {
        return NULL;
    }

    void *new_block = TAIL;

    // set old tail as a new free block
    mm_set_block_free(new_block, size);

    // set new tail block
    TAIL = NEXT_BLOCK(new_block);
    mm_set_block_alloc(TAIL, HEADER_SIZE);

    // try to coalesce with previous block
    if (!IS_PREV_ALLOCATED(new_block))
    {
        new_block = mm_coalesce(PREV_BLOCK(new_block), new_block);
    }

    return new_block;
}

/*
 * mm_coalesce - coalesce two contiguous free block
 * return new free block
 */
static void *mm_coalesce(void *first_block, void *second_block)
{
    SET_SIZE(first_block, GET_SIZE(first_block) + GET_SIZE(second_block));
    COPY_TO_FOOTER(first_block);
    return first_block;
}

static void mm_set_block_free(void *block, size_t size)
{
    SET_SIZE(block, size);
    SET_FREE(block);
    COPY_TO_FOOTER(block);
    SET_PREV_FREE(NEXT_BLOCK(block));
}

static void mm_set_block_alloc(void *block, size_t size)
{
    SET_SIZE(block, size);
    SET_ALLOC(block);
    SET_PREV_ALLOC(NEXT_BLOCK(block));
}

/*
 * search through list to find a first fit block
 */
static void *mm_search_block(void *begin, size_t need_size)
{
    for (void *block = begin; block != TAIL; block = NEXT_BLOCK(block))
    {
        if (IS_ALLOCATED(block))
        {
            continue;
        }

        size_t block_size = GET_SIZE(block);

        if (block_size < need_size)
        {
            continue;
        }

        return block;
    }

    return NULL;
}

/*
 * mm_init - initialize the malloc package.
 * initially allocate one PAGE size
 * use one block for tail to deal with corner case, which will never be freed
 * also need some bytes to align 8 bytes
 */
int mm_init(void)
{
    void *p = mem_sbrk(HEADER_SIZE + HEADER_SIZE);

    if (p == (void *)-1)
    {
        fprintf(stderr, "mm_init error: %s", strerror(errno));
        return -1;
    }

    TAIL = (void *)ALIGN((size_t)p);

    // init tail block
    SET_PREV_ALLOC(TAIL);
    mm_set_block_alloc(TAIL, HEADER_SIZE);

    // increment heap then HEAD become a free block and a new tail will be set
    HEAD = mm_increment(mem_pagesize());

    if (HEAD == NULL)
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
    size_t need_size = ALIGN(size) + HEADER_SIZE;
    void *candidate_block = mm_search_block(HEAD, need_size);

    // if no fit block then increment heap space and try again
    if (!candidate_block)
    {
        size_t incr = 0;
        size_t page_size = mem_pagesize();

        while (incr < need_size)
        {
            incr += page_size;
        }

        candidate_block = mm_increment(incr);

        if (!candidate_block)
        {
            return NULL;
        }
    }

    size_t candidate_size = GET_SIZE(candidate_block);

    // try to split remaining block. note every block is at least 2 * 8 bytes
    if (candidate_size - need_size >= HEADER_SIZE + ALIGNMENT)
    {
        mm_set_block_alloc(candidate_block, need_size);
        mm_set_block_free(NEXT_BLOCK(candidate_block), candidate_size - need_size);
    }
    else
    {
        mm_set_block_alloc(candidate_block, candidate_size);
    }

    check_consistency();
    return GET_PAYLOAD(candidate_block);
}

/*
 * mm_free - Freeing a block does nothing.
 * when free a block, the next block and previous block should always be free
 * except HEAD and TAIL
 */
void mm_free(void *ptr)
{
    void *block = PAYLOAD_TO_BLOCK(ptr);
    mm_set_block_free(block, GET_SIZE(block));

    // coalesce with next block
    void *next_block = NEXT_BLOCK(block);

    if (next_block != TAIL && !IS_ALLOCATED(next_block))
    {
        mm_coalesce(block, next_block);
    }

    // try to coalesce with previous block
    if (!IS_PREV_ALLOCATED(block))
    {
        void *prev_block = PREV_BLOCK(block);
        mm_coalesce(prev_block, block);
    }

    check_consistency();
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
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
