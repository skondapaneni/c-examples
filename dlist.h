#ifndef __DLIST_H__
#define __DLIST_H__

/* 
 * A doubly linked list implementation.
 */

#include "common.h"

/* 
 * Forward declaration.
 */
struct _node_st;

typedef void (*node_free_fn) (struct _node_st *);
typedef int  (*node_cmp_fn)(struct _node_st *cur_node, 
                               struct _node_st *to_node);

/* 
 * A doubly linked node.  A component can extend the _node_st
 * with any data it wants. The first 2 members have to be 
 * same as in this structure.
 */
typedef struct _node_st {
    struct _node_st *next;
    struct _node_st *prev;
} node_st;

/*
 * A header structure for storing the dlist info.
 */
typedef struct _dlist_st {
    node_st      *head;   /* pointer to the head of the  */
                           /* doubly linked list. */
    uint          count;
    node_free_fn  free_fn; /* A pointer to a function which is used */
                           /* to free the dlist structure when removed */
                           /* from the list. */
    node_cmp_fn   cmp_fn;  /* A pointer to a compare function to keep */
                           /* the list sorted. */
} dlist_st;


extern int dlist_ptr_cmp (node_st *cur_node, node_st *to_node);
extern int dlist_enqueue (dlist_st *dlist, node_st *node);
extern int dlist_dequeue (dlist_st *dlist, node_st *node);
extern int dlist_find_and_dequeue (dlist_st *dlist, node_cmp_fn cmp, 
                                   node_st *to_node);
extern node_st * dlist_dequeue_head (dlist_st *dlist);
extern node_st * dlist_dequeue_node_n (dlist_st *dlist, uint n);
extern node_st * dlist_find_node (dlist_st *dlist, node_cmp_fn cmp, 
        node_st *to_node);
extern int dlist_init (dlist_st **dlist, 
		node_free_fn free_fn, node_cmp_fn cmp_fn );

#endif
