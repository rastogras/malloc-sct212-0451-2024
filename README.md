# Custom Dynamic Memory Allocator (`myalloc`)

- **Name:** Rastogras
- **Student ID:** SCT212-0451/2024

## Build and Run Commands
```bash
# Build and run the 29-check test harness
make test

# Run extra edge-case tests (heap_dump and calloc zero checks)
gcc -std=c11 -Wall -Wextra -g -O0 -o extra_tests myalloc.c extra_tests.c && ./extra_tests

# Run Task 8 performance & fragmentation benchmark
gcc -std=c11 -Wall -Wextra -O2 -o bench myalloc.c bench.c && ./bench

# Clean build artifacts
make clean
```

## Task Checklist

| Task | Description | Status |
|---|---|---|
| Task 1 | Block header (`struct block`) and 16-byte alignment helpers | Complete |
| Task 2 | Growing the heap with `sbrk(2)` in >= 64 KiB chunks | Complete |
| Task 3 | `my_malloc()` with first-fit search | Complete |
| Task 4 | Block splitting when remainder holds header + >= 16 bytes | Complete |
| Task 5 | `my_free()` with bidirectional coalescing and double-free/invalid pointer checks | Complete |
| Task 6 | `my_calloc()` overflow protection and in-place/reallocated `my_realloc()` | Complete |
| Task 7 | `FIT_BEST` policy, `heap_stats()`, `heap_check()`, and `heap_dump()` | Complete |
| Task 8 | `bench.c` 50,000-op experiment and `REPORT.md` analysis | Complete |

## Final `make test` Output
```text
T7 best-fit vs first-fit
  PASS  first-fit takes the first hole
  PASS  best-fit takes the tightest hole
T8 randomized stress test
  PASS  no data corruption during 20000 operations
  PASS  everything freed
  PASS  fully coalesced (few free blocks remain)
  PASS  heap consistent

Result: 29 passed, 0 failed
```

## Design Explanation
Our custom allocator manages heap memory requested from the operating system via `sbrk(2)` using an explicit doubly-linked list of blocks ordered by increasing memory address. Each block starts with a 32-byte metadata header (`struct block`) containing the aligned payload `size`, a `free` flag, and `prev` and `next` pointers. Because 32 is a multiple of 16 and every requested payload size is rounded up to a multiple of 16, aligning the initial program break ensures every returned user pointer (`header + 1`) sits on a 16-byte boundary.

When `my_malloc()` is called, it searches from `head` using either `FIT_FIRST` (stopping at the first free block large enough) or `FIT_BEST` (scanning the list for the smallest free block that fits). If no existing free block satisfies the request, `grow_heap()` extends the program break by at least 65,536 bytes to amortize system call overhead. Whenever a chosen free block has enough leftover room for a new 32-byte header plus at least 16 bytes of payload, `split_block()` carves out the excess into a new free block immediately following it.

On `my_free()`, the allocator first verifies that the pointer matches an active block header in the list and is not already free, writing a diagnostic message to `stderr` via `write(2)` on invalid or duplicate frees. Valid freed blocks are immediately coalesced with physically adjacent free neighbors (`prev` and `next`) by verifying exact byte adjacency before merging headers and sizes, preventing false merges across memory gaps.

## Questions

1. **Why must the header size be a multiple of 16 as well as the payload sizes?**
   Each payload begins immediately after its block header (`(char *)block + sizeof(struct block)`). If the header size were not a multiple of 16, adding it to a 16-byte-aligned block start address would misalign the returned payload pointer. Keeping both the header size (32 bytes) and rounded payload sizes as multiples of 16 guarantees every subsequent block and payload remains 16-byte aligned.

2. **Why can `printf()` inside `my_malloc()` cause infinite recursion?**
   Standard C library functions like `printf()` allocate internal stdout buffers on the heap by calling `malloc()`. If `printf()` is called inside an allocator routine (or if `my_malloc` replaces `malloc`), `printf()` triggers another allocation call, which calls `printf()` again in an endless cycle until the call stack overflows.

3. **Your `my_free()` is $O(n)$. Which extra field would let you find a block's previous neighbor in $O(1)$, and what is the name of the technique?**
   Adding a `struct block *prev` pointer to each block header lets `my_free()` reach the immediately preceding block in $O(1)$ time without scanning from `head`. This technique is a **doubly-linked list** (and in classic heap allocators, pairing it with a footer size is also known as **boundary-tag coalescing**).

4. **Explain the difference between internal and external fragmentation, and name one place in your allocator where each one comes from.**
   **Internal fragmentation** is wasted space inside an allocated block that the user cannot use; in our allocator, it occurs when `round_up()` pads a small request (such as 1 byte) up to 16 bytes, or when a remainder smaller than `sizeof(struct block) + 16` is left unsplit inside an allocated block. **External fragmentation** is free memory scattered across non-adjacent holes that cannot satisfy a large single request even though total `free_bytes` is sufficient; in our allocator, it arises when live allocated blocks sit between freed blocks (`USED` blocks separating `FREE` holes).

5. **Why does `sbrk()` memory never get returned to the OS in your allocator? Describe how you could give back memory at the end of the heap.**
   Our allocator only ever calls `sbrk()` with positive increments in `grow_heap()`, so the program break only moves upward. To return memory to the OS, `my_free()` could check whether `tail` is free, sits directly at the current program break (`sbrk(0)`), and exceeds a threshold (such as 64 KiB), then unlink `tail` and call `sbrk(-(intptr_t)(sizeof(struct block) + tail->size))` to lower the program break.

6. **Two blocks are consecutive in your list but not adjacent in memory. How did this happen, and what does your coalescing code do about it?**
   External library routines (such as `printf()` in the test harness) can call the system `malloc()`, which moves the OS program break (`sbrk`) between two of our own `grow_heap()` calls, leaving a gap of foreign memory between consecutive blocks in our list. Before merging two list neighbors, our coalescing code explicitly checks `(char *)(b + 1) + b->size == (char *)b->next` so non-adjacent blocks are never merged across that gap.

## Known Limitations
- **Linear Search Overhead**: Both `FIT_FIRST` and `FIT_BEST` walk a single list containing both allocated and free blocks ($O(n)$), rather than maintaining a free-only list or segregated size classes.
- **No Heap Shrinking**: Unused memory at the top of the heap is retained for future reuse rather than returned to the OS via negative `sbrk()`.
- **Single-Threaded**: Global list pointers (`head`, `tail`) are not protected by a mutex, so the allocator is not thread-safe.

## Sources and AI Tools Declaration
- **Sources Consulted**: Linux manual pages (`sbrk(2)`, `write(2)`, `clock_gettime(2)`), *Operating Systems: Three Easy Pieces* (Free-Space Management chapter), and the ICS 2305 assignment specification.
- **AI Tools Used**: Used an AI coding assistant to review C pointer arithmetic for block splitting/coalescing, structure the `bench.c` experiment, and verify edge-case coverage against the test harness.
