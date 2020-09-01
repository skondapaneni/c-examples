#include "dlist.h"

/*
 * get_last_node.
 *
 * Return a ptr to the last node.
 */
inline static node_st *
get_last_node (node_st *node)
{
    if (!node) return (NULL);
    while (node->next != NULL) {
        node = node->next;
    }
    return (node);
}

/*
 * get_nth_node 
 *
 * traverse and return a ptr to the n'th node.
 */
inline static node_st *
get_nth_node (node_st *cur_node, int tcount)
{
    int i;

    if (!cur_node) return (NULL);

    for (i = 0; i < tcount-1; i++) {
        cur_node = cur_node->next;
        if (!cur_node) break;
    }
    
    return (cur_node);
}

/*
 * is_member
 *
 * Checks if the node pointer is a member of this
 * dlist.
 */
inline static boolean 
is_member (dlist_st *dlist, node_st *node)
{
    node_st *cur_node;

    cur_node = dlist->head;
    do {
        if (node == cur_node)
            return 1;
        cur_node = cur_node->next;
    } while (cur_node);

    return 0;
}

/*
 * do_delete
 *
 * Checks to see if the free function pointer
 * is available and invokes the free function
 * on the node.
 */
inline static int
do_delete (dlist_st *dlist, node_st *node)
{
    if (dlist->free_fn) {
        (*dlist->free_fn)(node);
    }
    return 0;
}


/*
 * binary_search
 *
 * Does a binary search on the list and return's the
 * node, if found. Return's NULL on failure.
 */
static node_st * 
binary_search (dlist_st *dlist, node_cmp_fn cmp, node_st *to_node)
{
    node_st   *cur_node, *cmp_node;
    uint       node_count, half;
    int        cmp_result;

    node_count = dlist->count;
    cur_node = dlist->head;

    /* 
     *  there is just one node.
     */
    if (node_count == 1) {
        cmp_result = cmp(cur_node, to_node);

        /* 
         * if value's are equal, then return the cur_node. 
         */
        if (cmp_result == 0)
            return (cur_node);

        return (NULL);
    }

    do {
        half = ROUND2(node_count)/2;
        cmp_node = get_nth_node (cur_node, half);

        if (!cmp_node) {
            return (NULL); 
        }

        cmp_result = cmp(cmp_node, to_node);

        if (cmp_result == 0)
            return (cmp_node);

        if (cmp_result < 0) {

            /* 
             * cmp_node is smaller than the to_node
             * hook point is to the right.
             * search further in the right half
             */
            node_count = node_count - half;

            /*
             * no more right half 
             */
            if (cmp_node->next == NULL)
                return (NULL);

            /*
             * re-adjust the right node.
             */
            cur_node = cmp_node->next;

        } else {

            /* 
             * search in the left half 
             */ 
            node_count = half;

            /* 
             * no_more_left_half.
             */
            if (node_count == 1) return (NULL);
        }

    } while (cur_node != NULL);

    return (NULL);
}

/*
 * find_insert_hook
 *
 * Does a binary search to find the node
 * which is less than the node_to_be_inserted.
 */
static node_st * 
find_insert_hook (dlist_st *dlist, node_cmp_fn cmp, node_st *to_node)
{
    node_st   *cur_node, *hook_node;
    uint       node_count, half;
    int        cmp_result;


    if (!dlist->head || dlist->count == 0)
        return NULL;

    node_count = dlist->count;
    cur_node = dlist->head;


    /* 
     *  there is just one node.
     */
    if (node_count == 1) {
        cmp_result = cmp(cur_node, to_node);

        /* 
         * if value of cur_node is less than the 
         * to_node (node to be inserted), then 
         * return the cur_node as the hook for 
         * insertion.
         */
        if (cmp_result < 0)
            return (cur_node);

        return (NULL);
    }

    do {
        half = ROUND2(node_count)/2;
        hook_node = get_nth_node (cur_node, half);

        if (!hook_node) {
            break; 
        }

        cmp_result = cmp(hook_node, to_node);
        if (cmp_result < 0) {

            /* 
             * hook_node is smaller than the to_node
             * hook point is to the right.
             * search further in the right half
             */
            node_count = node_count - half;
            if (hook_node->next == NULL)
                return (hook_node);
            cur_node = hook_node->next;

        } else {

            /* 
             * search in the left half 
             */ 
            node_count = half;

            /* 
             * if value of cur_node is greater than the 
             * to_node (node to be inserted), then 
             * return the previous node as the hook for 
             * insertion.
             */
            if (node_count == 1) return (cur_node->prev);
        }

    } while (cur_node != NULL);

    return NULL;
}

/*
 * dequeue_local
 *
 * A private method to dequeue a node from the list.
 */
static int
dequeue_local (dlist_st *dlist, node_st *node, boolean is_a_member)
{
    if (!dlist  || !node)
        return -1;

    /*
     * Make sure that node is a member of our list.
     */
    if (!is_a_member && !is_member (dlist, node)) {
        return -1; 
    }

    if (dlist->head == node) 
        dlist->head = node->next;

    /*
     * skip this node and re-adjust the pointers
     */
    if (node->prev)
        node->prev->next = node->next;

    if (node->next)
        node->next->prev = node->prev;

    dlist->count--;
    return (do_delete (dlist, node));

}

/*
 * dlist_init
 *
 * Init a new doubly linked list.
 */
int
dlist_init (dlist_st **dlist, node_free_fn free_fn, node_cmp_fn cmp_fn ) 
{
    if (!dlist) return -1;

    if (*dlist == NULL) {
        *dlist = (dlist_st *) malloc (sizeof (dlist_st));
    }

    if (*dlist == NULL) {
        return (-1);
    }

    (*dlist)->head = NULL;
    (*dlist)->free_fn = free_fn;
    (*dlist)->cmp_fn = cmp_fn;
    (*dlist)->count = 0;
    return 0;
}

/*
 * find_node
 *
 * Find a node in the list given a compare method.
 */
node_st* 
dlist_find_node (dlist_st *dlist, node_cmp_fn cmp, node_st *to_node)
{
    node_st *cur_node;

    if (!dlist || !dlist->head) return (NULL);

    cur_node = dlist->head;

    /* 
     * was the list sorted during insert.
     */
    if (dlist->cmp_fn) {
	return (binary_search (dlist, cmp, to_node));
    }

    /*
     * do a linear search.
     */
    do {
        if (cmp(cur_node, to_node) == 0)
            return cur_node;
        cur_node = cur_node->next;
    } while (cur_node);

    return (NULL);
}

int
dlist_ptr_cmp ( struct _node_st *cur_node, struct _node_st *to_node)
{
    return (cur_node - to_node);
}

/*
 * enqueue
 *
 * Enqueue a new node into the dlist.
 */
int 
dlist_enqueue (dlist_st *dlist, node_st *node)
{
    node_st *node_hook;

    if (!dlist  || !node)
        return -1;

    if (dlist->cmp_fn) 
        node_hook = find_insert_hook (dlist, dlist->cmp_fn, node);
    else 
        node_hook = get_last_node (dlist->head);

    /* add to head */
    if (!node_hook) {
        node->next = dlist->head;
        node->prev = NULL;
        dlist->head = node;
    if (node->next)
        node->next->prev = node;
        dlist->count++;
        return (0);
    }

    /* add in the middle */
    if (node_hook->next) {
        node->next = node_hook->next;
        node_hook->next->prev = node;
    }

    node_hook->next = node;
    node->prev = node_hook;
    dlist->count++;
    return 0; 
}

/*
 * dequeue
 *
 * Dequeue's a node from the list.
 * The node to be deleted should be a member of 
 * this list.
 */
int
dlist_dequeue (dlist_st *dlist, node_st *node)
{
    return (dequeue_local (dlist, node, FALSE));
}

/*
 * dequeue_head
 *
 * Dequeue's the head node from the list.
 */
node_st *
dlist_dequeue_head (dlist_st *dlist)
{
    node_st  *node;
    node = dlist->head;
    if (node && dequeue_local (dlist, node, TRUE) == 0)
        return (node);
    return (NULL);
}

/*
 * dequeue
 *
 * Dequeue's a node from the list.
 */
node_st *
dlist_dequeue_node_n (dlist_st *dlist, uint n)
{
    node_st  *node;
    node = get_nth_node(dlist->head, n);
    if (node && dequeue_local (dlist, node, TRUE) == 0)
        return (node);
    return (NULL);
}

/*
 * dequeue
 *
 * Dequeue's a node from the list.  The function take's a compare method 
 * as an arg to search for the node to be deleted.
 */
int
dlist_find_and_dequeue (dlist_st *dlist, node_cmp_fn cmp, node_st *to_node)
{
    node_st  *node;

    if (!dlist  || !cmp || !to_node)
       return -1;

    node = dlist_find_node (dlist, cmp, to_node); 

    if (!node) {
        return -1;
    }

    return (dequeue_local( dlist, node, TRUE));
}

/* --------------------------------------------*/
/* Example doubly linked list.                 */
/* --------------------------------------------*/

/* 
 * This shows an example of extending _node_st.
 */
typedef struct _foo_node_st {
    struct _node_st    *next;
    struct _node_st    *prev;
    int                 data;
} foo_node_st;


int 
foo_cmp (node_st *cur_node, node_st *to_node)
{
    foo_node_st *cur_foo, *to_foo;

    cur_foo = (foo_node_st *)cur_node;
    to_foo = (foo_node_st *)to_node;
    if (cur_foo->data == to_foo->data) {
        printf ("foo_cmp , return TRUE\n");
        fflush(stdout);
        return 0;
    }
    return (cur_foo->data - to_foo->data);
}

void
foo_print (foo_node_st *node)
{
    printf ("%d\n", node->data);
}

void
foo_print_list (dlist_st *dlist)
{
    foo_node_st  *cur_node;
    int           i;

    cur_node = (foo_node_st *) dlist->head;

    for (i = 0; i < dlist->count; i++) {
    if (!cur_node) break;
    foo_print (cur_node);
    cur_node = (foo_node_st *) cur_node->next;
    }
    if (cur_node)
        foo_print (cur_node);
}


int 
_main (int argc, char ** argv)
{
    foo_node_st  foo_node_arry[10];
    dlist_st     *dlist;
    int           status, i;

    status = dlist_init (&dlist, NULL, foo_cmp);

    for (i = 0; i < 10; i++) {
        foo_node_arry[i].data = i;
        foo_node_arry[i].next = NULL;
        foo_node_arry[i].prev = NULL;
    }

    status |= dlist_enqueue (dlist, (node_st *) &foo_node_arry[4]);
    status |= dlist_enqueue (dlist, (node_st *) &foo_node_arry[3]);
    status |= dlist_enqueue (dlist, (node_st *) &foo_node_arry[2]);
    status |= dlist_enqueue (dlist, (node_st *) &foo_node_arry[5]);
    status |= dlist_enqueue (dlist, (node_st *) &foo_node_arry[1]);
    status |= dlist_enqueue (dlist, (node_st *) &foo_node_arry[8]);
    status |= dlist_enqueue (dlist, (node_st *) &foo_node_arry[9]);
    status |= dlist_enqueue (dlist, (node_st *) &foo_node_arry[7]);
    status |= dlist_enqueue (dlist, (node_st *) &foo_node_arry[6]);
    status |= dlist_enqueue (dlist, (node_st *) &foo_node_arry[0]);

    foo_print_list (dlist);

    status |= dlist_find_and_dequeue (dlist, foo_cmp, (node_st *) &foo_node_arry[7]);
    foo_print_list (dlist);

    exit (status);
    return (status);
}

