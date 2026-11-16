# DAA_Assignment

Algorithm Explorer: a C web application that runs algorithms on user input and
shows the result, the steps taken, the number of operations and the running time.

## Build

Requires a C99 compiler (gcc, clang or MSVC via clang).

    make

On Windows the Makefile links ws2_32 automatically (MinGW/gcc).

## Run

    make run

Then open http://localhost:8080 in a browser.

## Project layout

    src/server.c   socket listener, routing, static file serving
    src/http.c/.h  HTTP request parsing and response helpers
    src/json.c/.h  JSON response builder
    src/ds/        data structures (stack, queue, list, step recorder)
    src/algo/      recursion and sorting algorithms
    src/metrics/   timing and benchmark code
    www/           browser front end
    report/        explanation document
    tests/         test scripts

## API

    GET  /api/health   server status
    POST /api/echo     echoes the posted JSON body

More endpoints are added as algorithm modules land.
