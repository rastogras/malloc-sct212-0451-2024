# Task 8 Experiment Report: Placement Policies and Fragmentation

## Benchmark Setup
We executed the exact same deterministic sequence of **50,000 random `malloc` and `free` operations** (sizes uniformly distributed between 1 and 4,096 bytes across 512 active slots, seeded with `srand(424242)`) in isolated child processes using `FIT_FIRST`, `FIT_BEST`, and the system's `glibc malloc`/`free`[cite: 11].

## Measured Results

| Policy | `heap_bytes` | `free_bytes` | `free_blocks` | `largest_free` | External Fragmentation (`1 - largest_free / free_bytes`) | Time |
|---|---:|---:|---:|---:|---:|---:|
| `FIT_FIRST` | 720,896 | 207,024 | 117 | 84,784 | 0.5905 | 39.67 ms |
| `FIT_BEST` | 720,896 | 207,552 | 91 | 99,344 | 0.5214 | 79.61 ms |
| `glibc malloc` | N/A | N/A | N/A | N/A | N/A | 4.10 ms |

## Policy Choice & Comparison with `glibc`
For general-purpose workloads where throughput matters, I would choose **`FIT_FIRST`**, whereas for memory-constrained systems with long-lived mixed-size allocations, **`FIT_BEST`** is preferable[cite: 11]. In our experiment, `FIT_BEST` lowered external fragmentation from `0.5905` to `0.5214`, reduced free-block fragmentation from 117 holes down to 91, and preserved a larger contiguous `largest_free` block (`99,344` bytes vs `84,784` bytes). However, because `FIT_BEST` must scan every block in the list to find the tightest hole, it took roughly twice as long (`79.61 ms` vs `39.67 ms`). Both policies are significantly slower than `glibc malloc` (`4.10 ms`) because our allocator performs an $O(n)$ linear walk over all blocks (both allocated and free), whereas `glibc` maintains segregated size bins (fastbins, tcache, and unsorted/small/large bins) that locate matching free chunks in $O(1)$ amortized time[cite: 11].
