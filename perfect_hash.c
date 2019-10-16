#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

uint8_t  **values;
int      *G;


inline unsigned 
ord (uint8_t c) {
    return (unsigned) c; 
}

/**
 * Calculates a distinct hash function for a given string. Each value of the
 * integer d results in a different hash value.
 */
unsigned long 
hash (unsigned d, const uint8_t *key, int len) {
   int i;
   if (d == 0) {
       d = 0x01000193;
   }

   for (i=0; i<len; i++) {
       d = ( ( d * 0x01000193) ^ ord(key[i]) ) & 0xffffffff;
   }

   return d;
}


/** ---------------------------------------------------------------------------- **/

typedef struct item_st {
   uint8_t *key;
   uint8_t *value;
} item_t;

typedef struct bucket_st {
   item_t           *items;
   uint16_t          nitems;
} bucket_t;

#define BUCKET_ELEMENT_SIZE 3

inline void 
append (bucket_t *bucket_p, item_t *item_p) { 
    if (bucket_p->nitems == 0) {
        bucket_p->items = calloc(BUCKET_ELEMENT_SIZE, sizeof(item_t));
    } else if (bucket_p->nitems % BUCKET_ELEMENT_SIZE == 0) {
        int multiple = bucket_p->nitems / BUCKET_ELEMENT_SIZE + 1;
        bucket_p->items = realloc(bucket_p->items, BUCKET_ELEMENT_SIZE * multiple * sizeof(item_t));
    }
    
    memcpy(&bucket_p->items[bucket_p->nitems], item_p, sizeof(item_t)); 
    bucket_p->nitems += 1;
}

inline int 
len (const bucket_t *bucket_p) {
    return bucket_p->nitems;
}

static int 
cmp_bucket (const void *p1, const void *p2) {
    return (((bucket_t *)p2)->nitems - ((bucket_t *)p1)->nitems);
}

static void
print_bucket (const bucket_t *bucket_p) {
    int i;
    for (i = 0; i < len(bucket_p); i++) {
        printf("%s", bucket_p->items[i].key);
        if (i != len(bucket_p)) {
            printf(",");
        } 
    }
    printf("\n");
}

/** ---------------------------------------------------------------------------- **/

typedef struct slot_vector_st {
   int       *slots;
   uint16_t  len;
   uint16_t  slice_begin;
} slot_vector_t;

inline void 
slot_vector_append (slot_vector_t *slot_p, int slot) {
    if (slot_p->len == 0) {
        slot_p->slots = calloc(BUCKET_ELEMENT_SIZE, sizeof(int));
    } else if (slot_p->len % BUCKET_ELEMENT_SIZE == 0) {
        int multiple = slot_p->len / BUCKET_ELEMENT_SIZE + 1;
        slot_p->slots = realloc(slot_p->slots, BUCKET_ELEMENT_SIZE * multiple * sizeof(int));
    }
    slot_p->slots[slot_p->len] = slot;
    slot_p->len += 1;
}

int
pop (slot_vector_t *slot_p) {
    if (slot_p->slice_begin < slot_p->len) {
        int slot = slot_p->slots[slot_p->slice_begin];
        slot_p->slice_begin += 1;
        return slot;
    }
    return -1;
}

void
free_slots (slot_vector_t *slot_p) {
    free(slot_p->slots);
    slot_p->slots = NULL;
    slot_p->len = 0;
}

void
deep_free (slot_vector_t *slot_p) {
    free_slots(slot_p);
    free(slot_p);
}

int 
member (slot_vector_t *slot_p, int slot) {
    int i;
    for (i = 0; i < slot_p->len; i++) {
        if (slot_p->slots[i] == slot) {
            return 1;
        }
    }
    return 0;
}

/** ---------------------------------------------------------------------------- **/

/**
 * Computes a minimal perfect hash table using the given python dictionary. It
 * returns a tuple (G, V). G and V are both arrays. G contains the intermediate
 * table of values needed to compute the index of the value in V. V contains the
 * values of the dictionary.
 */
void 
create_minimal_perfecthash (item_t *items, int size) {
    int          i,k;
    bucket_t     *buckets;

    buckets = calloc(size, sizeof(bucket_t));
    values = calloc(size, sizeof(uint8_t **));
    G = calloc(size, sizeof(int));
    
    // Step 1: Place all of the keys into buckets
    for (i = 0; i < size; i++) {
        append(&buckets[hash(0, items[i].key, strlen(items[i].key)) % size], &items[i]);
    }

    // Step 2: Sort the buckets and process the ones with the most items first.
    qsort(buckets, size, sizeof(bucket_t), cmp_bucket);

    // process the ones with the most items first.
    for (i = 0; i < size; i++) { 
        bucket_t *bucket_p = &buckets[i];
        if (len(bucket_p) <= 1) {
            break;
        }
        
        int d = 1;
        int item = 0;
        int slot;

        // A vector to cache the slots, before actually copying the values into these slots
        slot_vector_t *slots_vec_p = NULL;
        slots_vec_p = (slot_vector_t *) calloc(1, sizeof(slot_vector_t));

        // Repeatedly try different values of d until we find a hash function
        // that places all items in the bucket into free slots
        while (item < len(bucket_p)) {
            slot = hash(d, bucket_p->items[item].key, strlen(bucket_p->items[item].key)) % size;
            if (values[slot] != NULL || member(slots_vec_p, slot)) {
                d += 1;
                item = 0;
                free_slots(slots_vec_p);
            } else {
                // cache the slot
                slot_vector_append(slots_vec_p, slot);
                item += 1;
            }
        }

        G[hash(0, bucket_p->items[0].key, strlen(bucket_p->items[0].key)) % size] = d;

        for (k = 0; k < len(bucket_p); k++) {
            values[slots_vec_p->slots[k]] = bucket_p->items[k].value;
        }

        if (( i % 1000 ) == 0) {
            printf("bucket %d\n", i);    
            fflush(stdout);
        }

        deep_free(slots_vec_p);
    }

    //# Only buckets with 1 item remain. Process them more quickly by directly
    //# placing them into a free slot. Use a negative value of d to indicate
    //# this.
    slot_vector_t *freelist = (slot_vector_t *) calloc(1, sizeof(slot_vector_t));

    for (k = 0; k < size; k++) { 
        if (values[k] == 0) {
            slot_vector_append(freelist, k);
        }
    }

    for (; i < size; i++) { 
        bucket_t *bucket_p = &buckets[i];
        int slot;

        if (len(bucket_p) == 0) {
            break;
        }
        slot = pop(freelist);

        //# We subtract one to ensure it's negative even if the zeroeth slot was
        //# used.
        G[hash(0, bucket_p->items[0].key, strlen(bucket_p->items[0].key)) % size] = -slot-1;
        values[slot] = bucket_p->items[0].value;

        if (( i % 1000 ) == 0) {
            printf("bucket %d\n", i);    
            fflush(stdout);
        }
    }

    for (i = 0; i < size; i++) {
        bucket_t *bucket_p = &buckets[i];
        if (len(bucket_p) == 0) {
            continue;
        }
        printf("bucket[%d] ==>", i);
        print_bucket(bucket_p);
    }

    return; 
}

/**
 * Look up a value in the hash table, defined by G and V.
 */
uint8_t *perfect_hash_lookup(uint8_t *key, int size) {
    int d = G[hash(0, key, strlen(key)) % size];
    if (d < 0) {
       return values[-d-1];
    }
    return values[hash(d, key, strlen(key)) % size];
}

void test_lookup(uint8_t *key, int size) {
    char *lookup = perfect_hash_lookup(key, size);
    if (lookup) {
        printf("lookup=%s for key %s\n", lookup, key);
    }
}

int 
main (int argc, char **argv) {
    //uint8_t *keys[] = { "This", "is", "modern", "India", "Sir" };
    //uint8_t *values[] = { "10.0.0.1", "10.0.0.2", "10.0.0.3", "10.0.0.4", "10.0.0.5" };

    item_t items[] = {
        {"ab", "0"},
        {"cd", "1"},
        {"ef", "2"},
        {"gh", "3"},
        {"ij", "4"},
        {"kl", "5"},
    };

    int size =  sizeof(items)/sizeof(item_t);
    create_minimal_perfecthash(items, size); 

    test_lookup("ab", size);
    test_lookup("cd", size);
    test_lookup("ef", size);
    test_lookup("gh", size);
    test_lookup("ij", size);
    test_lookup("kl", size);

    exit(0);
}
