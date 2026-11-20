#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "myalloc.h"

int main(void) {
    assert(my_calloc(0, 10) == NULL);
    assert(my_calloc(10, 0) == NULL);

    void *p1 = my_malloc(64);
    void *p2 = my_malloc(128);
    assert(p1 != NULL && p2 != NULL);
    memset(p1, 'A', 64);

    heap_dump();
    assert(heap_check() == 1);

    my_free(p1);
    my_free(p2);
    assert(heap_check() == 1);

    puts("Extra edge-case tests passed!");
    return 0;
}
