#include <stdio.h>

typedef unsigned int uint;

static uint binary_mask[] = { 0xFFFF0000, 0x0000FF00, 0x000000F0, 0xC, 0x2 };
static uint binary_mask_ffs[] = { 0x0000FFFF, 0x000000FF, 0x0000000F, 0xC, 0x1 };
static short binary_tree[] = { 16, 8, 4, 2, 1 };

#define NMASK  sizeof(binary_mask)/sizeof(binary_mask[0])


/**
 * ffs - find last bit set
 * @x: the word to search
 */
static __inline__ int 
BIT_ffs (int x)
{
    int  y, rshift;
    uint mask2;

    /* 
     * we kind of do a binary search. 
     */ 
    mask2 = 0x0000FFFF;
    rshift = 0;

    y = (x & mask2);
    if (!y) {
        rshift = 16;
        x = (x >> 16);
    } 

    mask2 = 0x000000FF;

    y = (x & mask2);
    if (!y) {
        rshift += 8;
        x = (x >> 8);
    } 

    mask2 = 0x0000000F;

    y = (x & mask2);
    if (!y) {
        rshift += 4;
        x = (x >> 4);
    } 

    mask2 = 0x3;

    y = (x & mask2);
    if (!y) {
        rshift += 2;
        x = (x >> 2);
    } 

    mask2 = 0x1;

    y = (x & mask2);
    if (!y) {
       rshift += 1;
    }

    return rshift;
}

/**
 * fls - find last bit set
 * @x: the word to search
 */
static __inline__ int 
BIT_fls (int x)
{
    int  i, y, rshift;

    /* 
     * we do a binary search. 
     */ 
    rshift = 0;

    for (i = 0; i < NMASK; i++) {
        y = (x & binary_mask[i]);
        if (y) {
           rshift += binary_tree[i];
           x = (y >> binary_tree[i]);
        } 
    }

    return rshift;
}


int 
main (unsigned int argc, char **argv)
{
     int tval;
     sscanf (argv[1], "%d", &tval);
     printf ("shift count for the last bit set: %d\n", BIT_fls(tval));
     printf ("first bit set: %d\n", BIT_ffs(tval));
}

