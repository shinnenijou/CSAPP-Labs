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

// use second significant bit as predecessor block allocated mark
static const size_t PREV_ALLOC_MASK = 0x2;

static const size_t MARKS_MASK = ALIGNMENT - 1;
static const size_t SIZE_MASK = ~MARKS_MASK;

// pointer manipulation helper

#define HEADER(block) *(size_t *)(block)
#define PRE_FOOTER(block) *(size_t *)((char *)(block) - SIZE_T_SIZE)
#define FORWARD(ptr, distance) (void *)((char *)(ptr) + (distance))
#define BACKWARD(ptr, distance) (void *)((char *)(ptr) - (distance))

// list implementation independent helper functions

static int is_free(void *block);
static int is_pre_free(void *block);
static void set_free(void *block);
static void set_alloc(void *block);
static void set_pre_free(void *block);
static void set_pre_alloc(void *block);
static size_t get_size(void *block);
static size_t get_pre_size(void *block);
static void set_size(void *block, size_t size);
static void *block_to_payload(void *block);
static void *payload_to_block(void *p);
static void copy_to_footer(void *block);
static void *successor(void *block);
static void *predecessor(void *block);

/*
 *  is_free - if this block is free then could be allocated to user
 */
static int is_free(void *block)
{
    return (HEADER(block) & CUR_ALLOC_MASK) == 0;
}

/*
 * is_pre_free - if the predecessor block of this block is free
 *      every block record predecessor block's allocated status thus call to this function
 *      will not access the predecessor
 */
static int is_pre_free(void *block)
{
    return (HEADER(block) & PREV_ALLOC_MASK) == 0;
}

/*
 *  set_free - mark a block as free block
 */
static void set_free(void *block)
{
    HEADER(block) &= ~CUR_ALLOC_MASK;
}

/*
 *  set_alloc - mark a block as allocated block
 */
static void set_alloc(void *block)
{
    HEADER(block) |= CUR_ALLOC_MASK;
}

/*
 *  set_pre_free - mark a block as its predecessor block is free block
 */
static void set_pre_free(void *block)
{
    HEADER(block) &= ~PREV_ALLOC_MASK;
}

/*
 *  set_pre_alloc - mark a block as its predecessor block is allocated block
 */
static void set_pre_alloc(void *block)
{
    HEADER(block) |= PREV_ALLOC_MASK;
}

/*
 * get_size - get current block size. block size consists of user's payload
 *      and overhead(e.g., HEADER)
 */
static size_t get_size(void *block)
{
    return HEADER(block) & SIZE_MASK;
}

/*
 * get_pre_size - get size of predecessor block. the call is valid iff the predecessor
 *      block is free
 */
static size_t get_pre_size(void *block)
{
    return PRE_FOOTER(block) & SIZE_MASK;
}

/*
 * set_size - set current block size. block size consists of user's payload
 *      and overhead(e.g., HEADER)
 */
static void set_size(void *block, size_t size)
{
    size_t header = HEADER(block);
    header &= MARKS_MASK;
    header |= size;
    HEADER(block) = header;
}

/*
 * copy_to_footer - copy header to footer. useful when coalesce
 */
static void copy_to_footer(void *block)
{
    PRE_FOOTER(successor(block)) = HEADER(block);
}

/*
 * block_to_payload - get payload part of the block. typically used when return
 *      pointer to user
 */
static void *block_to_payload(void *block)
{
    return FORWARD(block, HEADER_SIZE);
}

/*
 * payload_to_block - get corresponding block pointer from a payload pointer.
 *      typically used when free a block by user
 */
static void *payload_to_block(void *p)
{
    return BACKWARD(p, HEADER_SIZE);
}

/*
 * successor - successor block to current block(i.e., next block in physical
 * memory space).This should be distinct with logic 'next' block in list.
 */
static void *successor(void *block)
{
    return FORWARD(block, get_size(block));
}

/*
 * predecessor - successor block to current block(i.e., next block in physical
 * memory space).This should be distinct with logic 'previous' block in list.
 * ths call to this function is valid iff predecessor block is free
 */
static void *predecessor(void *block)
{
    return BACKWARD(block, get_pre_size(block));
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
    for (void *block = HEAD; block != TAIL; block = successor(block))
    {
        if (get_size(block) == 0)
            break;
        fprintf(stdout, "[%d]free = %d, prev_free = %d, size = %d\n", ++i, is_free(block), is_pre_free(block), get_size(block));
    }
    fprintf(stdout, "======================\n");
}

static void check_consistency()
{
    int prev_free = is_free(HEAD);

    for (void *block = successor(HEAD); block != TAIL; block = successor(block))
    {
        int free = is_free(block);

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
 * then try to coalesce new block and predecessor block
 * do nothing with HEAD block (unless HEAD is exactly the predecessor block)
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
    TAIL = successor(new_block);
    mm_set_block_alloc(TAIL, HEADER_SIZE);

    // try to coalesce with predecessor block
    if (is_pre_free(new_block))
    {
        new_block = mm_coalesce(predecessor(new_block), new_block);
    }

    return new_block;
}

/*
 * mm_coalesce - coalesce two contiguous free block
 * return new free block
 */
static void *mm_coalesce(void *first_block, void *second_block)
{
    set_size(first_block, get_size(first_block) + get_size(second_block));
    copy_to_footer(first_block);
    return first_block;
}

static void mm_set_block_free(void *block, size_t size)
{
    set_size(block, size);
    set_free(block);
    copy_to_footer(block);
    set_pre_free(successor(block));
}

static void mm_set_block_alloc(void *block, size_t size)
{
    set_size(block, size);
    set_alloc(block);
    set_pre_alloc(successor(block));
}

/*
 * mm_search_block - search list from begin to end to find a fit block
 */
static void *mm_search_block(void *begin, size_t need_size)
{
    for (void *block = begin; block != TAIL; block = successor(block))
    {
        if (!is_free(block))
        {
            continue;
        }

        size_t block_size = get_size(block);

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
    set_pre_alloc(TAIL);
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

    size_t candidate_size = get_size(candidate_block);

    // try to split remaining block. note every block is at least 2 * 8 bytes
    if (candidate_size - need_size >= HEADER_SIZE + ALIGNMENT)
    {
        mm_set_block_alloc(candidate_block, need_size);
        mm_set_block_free(successor(candidate_block), candidate_size - need_size);
    }
    else
    {
        mm_set_block_alloc(candidate_block, candidate_size);
    }

    check_consistency();
    return block_to_payload(candidate_block);
}

/*
 * mm_free - Freeing a block does nothing.
 * except HEAD and TAIL
 */
void mm_free(void *ptr)
{
    void *block = payload_to_block(ptr);
    mm_set_block_free(block, get_size(block));

    // coalesce with next block
    void *next_block = successor(block);

    if (next_block != TAIL && is_free(next_block))
    {
        mm_coalesce(block, next_block);
    }

    // try to coalesce with predecessor block
    if (is_pre_free(block))
    {
        void *prev_block = predecessor(block);
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
