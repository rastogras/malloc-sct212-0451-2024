#define _DEFAULT_SOURCE
#include <stdio.h>
#include "myalloc.h"
#include <stdint.h>
#include <string.h>
#include <unistd.h>

struct block {
    size_t size;
    int free;
    struct block *prev;
    struct block *next;
};

static struct block *head = NULL;
static struct block *tail = NULL;
static size_t total_heap = 0;
static fit_policy_t policy = FIT_FIRST;

static size_t round_up(size_t size) {
    size_t rem = size % 16;
    if (rem == 0) {
        return size;
    }
    return size + (16 - rem);
}

static struct block *grow_heap(size_t size) {
    void *cur = sbrk(0);
    if (cur == (void *)-1) {
        return NULL;
    }

    size_t pad = (16 - ((uintptr_t)cur % 16)) % 16;
    size_t total = sizeof(struct block) + size;
    if (total < 65536) {
        total = 65536;
    }

    void *ptr = sbrk(pad + total);
    if (ptr == (void *)-1) {
        return NULL;
    }

    total_heap += pad + total;
    struct block *b = (struct block *)((char *)ptr + pad);
    b->size = total - sizeof(struct block);
    b->free = 1;
    b->prev = tail;
    b->next = NULL;

    if (tail != NULL) {
        tail->next = b;
    } else {
        head = b;
    }
    tail = b;

    return b;
}

static void split_block(struct block *b, size_t size) {
    if (b->size >= size + sizeof(struct block) + 16) {
        struct block *extra = (struct block *)((char *)(b + 1) + size);
        extra->size = b->size - size - sizeof(struct block);
        extra->free = 1;
        extra->prev = b;
        extra->next = b->next;

        if (b->next != NULL) {
            b->next->prev = extra;
        } else {
            tail = extra;
        }
        b->next = extra;
        b->size = size;

        if (extra->next != NULL && extra->next->free &&
            (char *)(extra + 1) + extra->size == (char *)extra->next) {
            struct block *nxt = extra->next;
            extra->size += sizeof(struct block) + nxt->size;
            extra->next = nxt->next;
            if (nxt->next != NULL) {
                nxt->next->prev = extra;
            } else {
                tail = extra;
            }
        }
    }
}

void *my_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    size = round_up(size);
    struct block *curr = NULL;
    struct block *scan = head;

    while (scan != NULL) {
        if (scan->free && scan->size >= size) {
            if (policy == FIT_FIRST) {
                curr = scan;
                break;
            }
            if (curr == NULL || scan->size < curr->size) {
                curr = scan;
            }
        }
        scan = scan->next;
    }

    if (curr == NULL) {
        curr = grow_heap(size);
        if (curr == NULL) {
            return NULL;
        }
    }

    split_block(curr, size);
    curr->free = 0;
    return (void *)(curr + 1);
}

void set_policy(fit_policy_t p) {
    policy = p;
}

void my_free(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    struct block *b = (struct block *)ptr - 1;
    b->free = 1;

    /* Merge with next block if free and physically adjacent */
    if (b->next != NULL && b->next->free &&
        (char *)(b + 1) + b->size == (char *)b->next) {
        struct block *nxt = b->next;
        b->size += sizeof(struct block) + nxt->size;
        b->next = nxt->next;
        if (nxt->next != NULL) {
            nxt->next->prev = b;
        } else {
            tail = b;
        }
    }

    /* Merge with previous block if free and physically adjacent */
    if (b->prev != NULL && b->prev->free &&
        (char *)(b->prev + 1) + b->prev->size == (char *)b) {
        struct block *prv = b->prev;
        prv->size += sizeof(struct block) + b->size;
        prv->next = b->next;
        if (b->next != NULL) {
            b->next->prev = prv;
        } else {
            tail = prv;
        }
    }
}

void *my_calloc(size_t nmemb, size_t size) {
    if (nmemb == 0 || size == 0) {
        return NULL;
    }
    if (nmemb > SIZE_MAX / size) {
        return NULL;
    }

    size_t total = nmemb * size;
    void *ptr = my_malloc(total);
    if (ptr != NULL) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void *my_realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return my_malloc(size);
    }
    if (size == 0) {
        my_free(ptr);
        return NULL;
    }

    struct block *b = (struct block *)ptr - 1;
    size_t aligned = round_up(size);

    if (b->size >= aligned) {
        split_block(b, aligned);
        return ptr;
    }

    void *new_ptr = my_malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }

    memcpy(new_ptr, ptr, b->size);
    my_free(ptr);
    return new_ptr;
}

void heap_stats(heap_stats_t *out) {
    if (out == NULL) {
        return;
    }
    out->heap_bytes = total_heap;
    out->used_bytes = 0;
    out->free_bytes = 0;
    out->blocks = 0;
    out->free_blocks = 0;
    out->largest_free = 0;

    struct block *curr = head;
    while (curr != NULL) {
        out->blocks++;
        if (curr->free) {
            out->free_blocks++;
            out->free_bytes += curr->size;
            if (curr->size > out->largest_free) {
                out->largest_free = curr->size;
            }
        } else {
            out->used_bytes += curr->size;
        }
        curr = curr->next;
    }
}

int heap_check(void) {
    struct block *curr = head;
    struct block *prev = NULL;

    while (curr != NULL) {
        if (curr->prev != prev) {
            return 0;
        }
        if (((uintptr_t)(curr + 1) % 16) != 0) {
            return 0;
        }
        if (curr->free && curr->next != NULL && curr->next->free &&
            (char *)(curr + 1) + curr->size == (char *)curr->next) {
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }

    if (prev != tail) {
        return 0;
    }
    return 1;
}

void heap_dump(void) {
    char buf[128];
    struct block *curr = head;
    int i = 0;

    while (curr != NULL) {
        int len = snprintf(buf, sizeof(buf), "block %d: addr=%p size=%zu %s\n",
                           i, (void *)(curr + 1), curr->size,
                           curr->free ? "FREE" : "USED");
        if (len > 0) {
            ssize_t ignored = write(STDOUT_FILENO, buf, (size_t)len);
            (void)ignored;
        }
        curr = curr->next;
        i++;
    }
}
