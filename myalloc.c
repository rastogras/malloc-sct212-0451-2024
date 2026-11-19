#define _DEFAULT_SOURCE
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
    }
}

void *my_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    size = round_up(size);
    struct block *curr = head;

    while (curr != NULL) {
        if (curr->free && curr->size >= size) {
            break;
        }
        curr = curr->next;
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
