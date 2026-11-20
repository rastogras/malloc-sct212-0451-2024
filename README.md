# Custom Dynamic Memory Allocator (`myalloc`)

A custom heap memory allocator in C implementing `my_malloc`, `my_free`, `my_calloc`, and `my_realloc` on top of `sbrk(2)`.

## Features
- **16-Byte Alignment**: All returned payload pointers are 16-byte aligned.
- **Doubly-Linked Block List**: Tracks block payload size, free status, and `prev`/`next` neighbors.
- **Block Splitting & Bidirectional Coalescing**: Splits oversized free blocks on allocation and merges adjacent free neighbors on `my_free()` to prevent fragmentation.
- **Placement Policies**: Supports both `FIT_FIRST` and `FIT_BEST` via `set_policy()`.
- **Diagnostics**: Implements `heap_stats()`, `heap_check()`, and `heap_dump()` (using `write(2)` without internal heap allocations).

## Build & Run Tests
```bash
make test
