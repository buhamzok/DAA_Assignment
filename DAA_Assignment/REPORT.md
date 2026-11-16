# Algorithm Explorer Report

## Purpose and design

Algorithm Explorer is a web application that accepts browser input, runs a
hand-written algorithm, and returns the result, a step trace, precise counters,
and elapsed wall-clock time. The C server uses raw sockets and routes requests to
the recursion, sorting, and benchmark modules. The browser sends JSON using
`fetch` and renders the returned result and trace.

All data structures and algorithms are implemented manually in C. The project
does not use `qsort`, external containers, or algorithm libraries. The shared
`Recorder` stores a bounded trace and counts comparisons, swaps, writes/moves,
and total operations. Trace storage is capped so large runs cannot consume
unbounded memory; counters continue after the trace is truncated.

## Algorithms and complexity

| Algorithm | Best | Average | Worst | Extra space |
|---|---:|---:|---:|---:|
| Bubble sort (early exit) | O(n) | O(n²) | O(n²) | O(1) |
| Insertion sort | O(n) | O(n²) | O(n²) | O(1) |
| Selection sort | O(n²) | O(n²) | O(n²) | O(1) |
| Merge sort | O(n log n) | O(n log n) | O(n log n) | O(n) |
| Quick sort | O(n log n) | O(n log n) | O(n²) | O(log n) expected |
| Heap sort | O(n log n) | O(n log n) | O(n log n) | O(1) |

Factorial has linear recursion depth. Naive Fibonacci is exponential and is
limited to 25; memoized Fibonacci is linear and limited to 90. Hanoi has
2^n - 1 moves and is limited to 15. These bounds prevent unsafe or impractical
requests.

## Metric and timing policy

- A comparison is one comparison of two values.
- A swap is only an exchange of two positions.
- A write/move is a shift or placement; it is not labelled as a swap.
- Operations are the algorithm's explicit unit-of-work counter and are used for
  relative complexity trends, not as a replacement for formal analysis.
- Timing uses `QueryPerformanceCounter` on Windows and `CLOCK_MONOTONIC` on
  POSIX. It measures elapsed algorithm time, excluding JSON construction.

## Verification

Made test runs deterministic checks for all six sorts on duplicates and
negative numbers, empty input, factorial's base case, both Fibonacci versions,
Hanoi, and unknown-sort rejection. The server can be verified with:

```text
curl http://localhost:8080/api/health
curl -X POST http://localhost:8080/api/sort -d "{\"algorithm\":\"quick\",\"input\":[5,2,9,1,7]}"
curl -X POST http://localhost:8080/api/recursion -d "{\"algorithm\":\"factorial\",\"input\":5}"
curl -X POST http://localhost:8080/api/benchmark -d "{\"family\":\"sorting\",\"sizes\":[100,200]}"
```


## References

1. T. H. Cormen, C. E. Leiserson, R. L. Rivest, and C. Stein, *Introduction to
   Algorithms*, 4th ed., MIT Press, 2022.
2. B. W. Kernighan and D. M. Ritchie, *The C Programming Language*, 2nd ed.,
   Prentice Hall, 1988.
