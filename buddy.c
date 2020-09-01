#include "dlist.h"

/* The Buddy system ...
 * The idea of this method is to keep separate lists of
 * available blocks of each size 2 ^k, 0 <= k <= m.
 * -- pointer freelists[m];
 * The entire pool of memory space under allocation consists of 2^m words
 * Originally the entire block of 2^m words is available. Later,
 * when a block of 2^k words is desired, and if nothing of this size
 * is available, a larger available block is split into 2 equal parts;
 * ultimately a block of the right size 2^k will appear.
 */

/*
 * This is entire pool of memory space - 2 pow MAX_SIZE
 */
#define  MAX_SIZE   32

/* 
 * blocks in freelists[i] are of size 2**i. 
 */
#define BLOCKSIZE(i) (1 << (i))

/* 
 * the address of the buddy of a block from freelists[i]. 
 */ 
#define BUDDYOF(b,i) ( ((uint)b) ^ (1 << (i)) )

/* 
 * we use the first LSB bit in the size to figure out whether the block
 * is available or free.
 */
#define RESERVED(P) (((uint) (P) & 0x1) == 1)
#define AVAILABLE(P) (((uint) (P) & 0x1) == 0)

#define SET_RESERVED(P) ((uint) (P) |= 0x1)
#define SET_AVAILABLE(P) ((uint) (P) &= 0xFFFFFFFD)

/*
 * Available blocks also have 2 link fields, LINKF and LINKB, which
 * are the usual forward and backward links of a doubly linked list. and 
 * they also  have a KVAL Field to specify k when their size is 2^k.
 */

/* 
 * pointers to the free space lists 
 */
dlist_st freelists_g[MAX_SIZE];


void
log_error (char *err_msg)
{
    fprintf (stderr, "%s\n", err_msg);
}

int
buddy_init ()
{
    uint      i;
    dlist_st *dlist_i;

    for (i = 0; i < MAX_BLOCK_SIZE; i++) {
        dlist_i = &freelists_g[i];
        dlist_init (&dlist_i, NULL, NULL);
    }
    return (1);
}


unsigned char* 
buddy_alloc (int size)
{
     uint   i;
     uchar *block, *buddy;

     /* 
      * compute i as the least integer such that i >= log2(size) 
      */
     for (i = 0; BLOCKSIZE( i ) < size; i++);

     if (i >= MAX_SIZE) {

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
void 
buddy_dealloc (uchar *block, int size)
{
    uint   i;
    uchar *p, *buddy;

    /* 
     * compute i as the least integer such that i >= log2(size) 
     */ 
    for (i = 0; BLOCKSIZE( i ) < size; i++);

    /* 
     * see if this block's buddy is free 
     */
    buddy = BUDDYOF(block, i);


    /* buddy not free, put block on its free list */
    if (ALLOCATED(buddy)) {
        SET_AVAILABLE(block);
        dlist_add (&freelists[i], block);
    }

    /* buddy found, remove it from its free list */
    dlist_delete (&freelists[i], buddy);

    /* deallocate the block and its buddy as one block */
    return (buddy_dealloc (coalesce(block, buddy)), BLOCKSIZE (i+1));

}
