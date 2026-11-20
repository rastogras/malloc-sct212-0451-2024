#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "myalloc.h"

#define OPS 50000
#define SLOTS 512

static double elapsed_ms(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) * 1000.0 + (b.tv_nsec - a.tv_nsec) / 1e6;
}

static void run_custom(fit_policy_t pol, const char *label) {
    set_policy(pol);
    void *p[SLOTS] = {0};
    srand(424242);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int step = 0; step < OPS; step++) {
        int i = rand() % SLOTS;
        if (p[i] != NULL) {
            my_free(p[i]);
            p[i] = NULL;
        } else {
            size_t sz = 1 + (size_t)(rand() % 4096);
            p[i] = my_malloc(sz);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);

    heap_stats_t s;
    heap_stats(&s);
    double frag = (s.free_bytes > 0)
        ? (1.0 - (double)s.largest_free / (double)s.free_bytes)
        : 0.0;

    printf("| %s | %zu | %zu | %zu | %zu | %.4f | %.2f ms |\n",
           label, s.heap_bytes, s.free_bytes, s.free_blocks,
           s.largest_free, frag, elapsed_ms(t0, t1));

    for (int i = 0; i < SLOTS; i++) {
        my_free(p[i]);
    }
}

static void run_glibc(void) {
    void *p[SLOTS] = {0};
    srand(424242);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int step = 0; step < OPS; step++) {
        int i = rand() % SLOTS;
        if (p[i] != NULL) {
            free(p[i]);
            p[i] = NULL;
        } else {
            size_t sz = 1 + (size_t)(rand() % 4096);
            p[i] = malloc(sz);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);

    printf("| glibc malloc | N/A | N/A | N/A | N/A | N/A | %.2f ms |\n",
           elapsed_ms(t0, t1));

    for (int i = 0; i < SLOTS; i++) {
        free(p[i]);
    }
}

int main(void) {
    printf("| Policy | heap_bytes | free_bytes | free_blocks | largest_free | External Fragmentation | Time |\n");
    printf("|---|---|---|---|---|---|---|\n");
    fflush(stdout);

    if (fork() == 0) { run_custom(FIT_FIRST, "FIT_FIRST"); return 0; }
    wait(NULL);

    if (fork() == 0) { run_custom(FIT_BEST, "FIT_BEST"); return 0; }
    wait(NULL);

    if (fork() == 0) { run_glibc(); return 0; }
    wait(NULL);

    return 0;
}
