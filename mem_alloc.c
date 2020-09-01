#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#define  HEADER_SIZE     (sizeof (struct _block_header_st))
#define  FOOTER_SIZE     (sizeof (uint))       /* has the address to header */
#define  OVERHEAD        HEADER + FOOTER 
#define  ALIGN8(N)       (8 * ((N+7)/8)

#define  HEADER(p)      ((block_header_st *) (p - HEADER_SIZE))
#define  TRAILER(p)     HEADER(p) + BLOCK_SIZE(p) - FOOTER_SIZE

#define  PREV_BLOCK(p)  HEADER(p)->prev
#define  NEXT_BLOCK(p)  HEADER(p)->next
#define  BLOCK_SIZE(p)  HEADER(p)->size

#define  GET(p)         (*(uint *)(p))
#define  PUT(p, val)    (*(uint *)(p) = (val))

#define  SET_FREE(p)    PUT(TRAILER(p), ((uint)HEADER(p) | 0x1))
#define  SET_USED(p)    PUT(TRAILER(p), ((uint)HEADER(p) & ~0x1))

typedef unsigned char uchar;
typedef short boolean;

/*
 * prologue block.
 * What is there at the front of the allocated 
 * memory.
 */
typedef struct  _block_header_st {
    struct _block_header_st *next;
    struct _block_header_st *prev;
    uint                     size; /* size in bytes, which can also */
                                   /* tell us if the block is free or used */
} block_header_st;

typedef struct  _mem_manager_st {
    struct _block_header_st    *free_list;
    struct _block_header_st    *used_list;
} mem_manager_st;

mem_manager_st    mem_manager_g;

void
init_mem_block (block_header_st *block_p, block_header_st *next, block_header_st *prev,
        uint size, boolean is_free)
{
    block_p->next = next;
    block_p->prev = prev;
    block_p->size = size;

    /* 
     * put the address of the header in the trailer block. 
     * The LSB is used to mark it free or used 
     */ 
     
    if (is_free)
       SET_FREE(block_p + HEADER_SIZE);
    else 
       SET_USED(block_p + HEADER_SIZE);

    printf ("header addr = 0x%x, trailer addr = 0x%x, trailer = %d\n", 
            block_p, TRAILER(block_p + HEADER_SIZE),
            GET(TRAILER(block_p + HEADER_SIZE))); 
}

int 
alloc_chunk (int chunk_size)
{
    uint      overhead, size_required;
    
    overhead = HEADER_SIZE + FOOTER_SIZE;
    size_required = chunk_size + overhead;

    /* allocate the free list */
    mem_manager_g.free_list = (block_header_st *) 
        malloc(sizeof(uchar) * size_required);
    if (!mem_manager_g.free_list) return (-1);

    init_mem_block ((block_header_st *) mem_manager_g.free_list, NULL, NULL, 
            size_required, 1);
}

int 
main (int argc, char **argv)
{
    int        chunk_size;

    chunk_size = 1024;
    alloc_chunk (chunk_size);

    exit (0);
}


