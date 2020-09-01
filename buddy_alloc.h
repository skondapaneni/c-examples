#ifndef __BUDDY_ALLOC_H__
#define __BUDDY_ALLOC_H__

#include <inttypes.h>


/* 
 * The Buddy system ...
 * The idea of this method is to keep separate lists of
 * available blocks of each size 2 ^k, 0 <= k <= m.
 * The entire pool of memory space under allocation consists of 2^m words
 * Originally the entire block of 2^m words is available. Later,
 * when a block of 2^k words is desired, and if nothing of this size
 * is available, a larger available block is split into 2 equal parts;
 * ultimately a block of the right size 2^k will appear.
 */

#include "dlist.h"

/*
 * This is entire pool of memory space - 2 pow MAX_BLOCK_SIZE
 * All memory blocks are aligned to 2 power k
 */
#define  MAX_BLOCK_SIZE         32

/* 
 * blocks in freelists[i] are of size 2**i. 
 */
#define BLOCKSIZE(i)            (1 << (i))

/* 
 * the address of the buddy of a block from freelists[i]. 
 */ 
#define BUDDYOF(b,i)           (uchar *) ( ((uint64_t) b) ^ (1 << (i)) )

#define GET(p)                 (*(uint64_t *)(p))
#define PUT(p, val)            (*(uint64_t *)(p) = (val))


#define  MIN_SIZE_REQUIRED     (sizeof (struct _node_st))
#define  ROUND8(N)             (8 * ((N+7)/8))

int buddy_alloc_init ();
int buddy_alloc_chunk (uint chunk_size);
unsigned char* buddy_alloc (int size);
int buddy_dealloc (uchar *block, int size);

#endif

