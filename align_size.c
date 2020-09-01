#include <stdio.h>

#define ALIGN(ptr, align_bytes) \
       ((void *)((((uint32_t)ptr) + (align_bytes-1)) & ~(align_bytes-1)))

void *
align_up (void *ptr, size_t size)
{
    void        *result;

    result = ptr + size - 1;
    result = (void *) (((unsigned int) result) & ~(size - 1));
    return(result);
}


int
main (int argc, char **argv)
{
     int a;
     void *p = &a;
     void *align_p = align_up(p, 32);
     printf("%p %p\n", p, align_p);
}
