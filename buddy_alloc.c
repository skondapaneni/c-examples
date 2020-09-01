#include "buddy_alloc.h"

/* 
 * global memory manager data
 */
dlist_st    freelists_g[MAX_BLOCK_SIZE];
boolean     mem_init_g;

/*
 * get_real_size
 *
 * Adjusts the size of the chunk by adding
 * the overhead and alignment and return the real size.
 */
uint 
get_real_size (uint chunk)
{
    uint  new_size;

    if (chunk < MIN_SIZE_REQUIRED)
        chunk = MIN_SIZE_REQUIRED;
    new_size = ROUND8(chunk);
    return (new_size);
}

/*
 * log_error
 *
 * Just print an error to stderr for now.
 */
void
log_error (char *err_msg)
{
    fprintf (stderr, "%s\n", err_msg);
}

/*
 * get_free_block
 *
 * returns a free block of size 2^i from the freelists
 */
uchar *
get_free_block (uint i)
{
    dlist_st  *dlist;

    dlist = &freelists_g[i];
    return ((uchar *) dlist_dequeue_head(dlist));
}

/*
 * put_free_block
 *
 * puts a free block into the doubly linked freelist.
 */
int
put_free_block(uchar *buddy, uint i)
{
    dlist_st  *dlist;
    node_st   *node;

    dlist = &freelists_g[i];
    node = (node_st *) buddy;
    return (dlist_enqueue (dlist, node));
}

/*
 * delete_free_block
 *
 * deletes a free block from the doubly linked freelist.
 * Most likely we are coalescing 2 buddies.
 */
int
delete_free_block (uchar *buddy, uint i)
{
    dlist_st  *dlist;
    node_st   *node;

    dlist = &freelists_g[i];
    node = (node_st *) buddy;
    return (dlist_dequeue (dlist, node));
}

/*
 * is_available
 *
 * Walks the free list and checks to see if a block of memory is available.
 */
int
is_available (uchar *buddy, uint i)
{
    dlist_st  *dlist;
    node_st   *node;

    dlist = &freelists_g[i];
    node = (node_st *) buddy;
    return (dlist_find_node (dlist, dlist_ptr_cmp, node) != NULL);
}

/*
 * buddy_alloc_init
 *
 * Initialize the buddy allocation system.
 */
int
buddy_alloc_init ()
{
    uint       i;
    dlist_st  *dlist_i;

    for (i = 0; i < MAX_BLOCK_SIZE; i++) {
        dlist_i = &(freelists_g[i]);
        dlist_init (&dlist_i, NULL, NULL);
    }
    mem_init_g = 1;
    return (0);
}

int 
buddy_alloc_chunk (uint chunk_size)
{
    uint   real_size, i;
    uchar *buf;

    real_size = get_real_size (chunk_size);
    buf = (uchar *) malloc (real_size * sizeof(uchar));
    if (!buf) {
        return (-1);
    }

    for (i = 0; BLOCKSIZE(i) < real_size; i++) {
        ;
    }
    put_free_block(buf, i);
    return (0);
}

unsigned char* 
buddy_alloc (int size)
{
    uint   i, real_size;
    uchar *block, *buddy;

    real_size = get_real_size(size);

    /* 
     * compute i as the least integer such that i >= log2(size) 
     */
    for (i = 0; BLOCKSIZE(i) < real_size; i++);

    if (i >= MAX_BLOCK_SIZE) {
        log_error ( "no space available" );
        return (NULL);
    } else if (freelists_g[i].count != 0) {

        /* 
         * we already have the right size block on hand 
         */
        block = get_free_block(i);
        return (block);

    } else {

        /* 
         * we need to split a bigger block 
         */ 
        block = buddy_alloc(BLOCKSIZE(i + 1));

        if (block != NULL) {

            /* 
             * split and put extra on a free list 
             */ 
             buddy = BUDDYOF(block, i);
             put_free_block(buddy, i);
        }
        return (block);
    }
}

/*
 * coalesce
 *
 * In the buddy system coalesce returns the pointer
 * that is bigger. The 2 buddies join to become a bigger
 * size block.
 */
uchar *
coalesce(uchar *buddy_a, uchar *buddy_b)
{
    if (buddy_a > buddy_b)
        return (buddy_a);
    return (buddy_b);
}

/*
 * De-allocating a block of memory.
 * Coalesce with its buddy
 * Return to the appropriate free list.
 */
int 
buddy_dealloc (uchar *block, int size)
{
    uint   i;
    uchar *p, *buddy;

    /* 
     * compute i as the least integer such that i >= log2(size) 
     */ 
    for (i = 0; BLOCKSIZE(i) < size; i++);

    /* 
     * get its buddy 
     */
    buddy = BUDDYOF(block, i);

    /* 
     * see if this block's buddy is free. 
     * buddy not free, put block on its free list 
     */ 
    if (!is_available(buddy, i)) {
        put_free_block(buddy, i);
        return 0;
    }

    /* buddy found, remove it from its free list */
    delete_free_block(buddy, i);

    /* deallocate the block and its buddy as one block */
    return (buddy_dealloc (coalesce(block, buddy), BLOCKSIZE(i+1)));
}

