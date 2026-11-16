# DAA_Assignment

Algorithm Explorer: a C web application that runs algorithms on user input and
shows the result, the steps taken, the number of operations and the running time.

## Build

Requires a C99 compiler (gcc, clang or MSVC via clang).

    make

The Makefile compiles the server together with the shared data-structure,
recursion, sorting, and benchmark modules in `../modules/`.

On Windows the Makefile links ws2_32 automatically (MinGW/gcc).

## Run

    make run

Then open http://localhost:8080 in a browser.

## Verify

Run the automated core-algorithm checks:

    make test

For memory-safety checks with GCC or Clang, rebuild with:

    make clean
    make CFLAGS="-std=c99 -Wall -Wextra -Werror -fsanitize=address,undefined -g -Isrc -I../modules/common" LDFLAGS="-fsanitize=address,undefined -lm"

On Windows/MinGW, use a sanitizer-enabled toolchain where available.

## Project layout

    src/server.c   socket listener, routing, static file serving
    src/http.c/.h  HTTP request parsing and response helpers
    src/json.c/.h  JSON response builder
    src/           socket server, HTTP and JSON support
    ../modules/    data structures, algorithms, timing, UI, benchmarks
    tests/         automated algorithm checks
    REPORT.md      design, complexity, testing and references

## API

    GET  /api/health   server status
    POST /api/echo     echoes the posted JSON body

    POST /api/recursion  factorial, fibonacci, fibonacci_memo, hanoi
    POST /api/sort       bubble, insertion, selection, merge, quick, heap
    POST /api/benchmark  measured algorithm comparison

Metric policy: `comparisons` counts value comparisons, `swaps` counts only
exchanges of two array elements, and `writes/moves` counts shifts or placements.
`operations` is their aggregate algorithm-work counter; it is not presented as a
synonym for swaps. Times are elapsed wall-clock milliseconds measured around the
algorithm call only.
