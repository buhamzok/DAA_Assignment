#ifndef RECURSION_API_H
#define RECURSION_API_H

#include "ds.h"
#include "json.h"

/*
 * Shared interface to the recursion algorithms.
 * The benchmark module calls these directly.
 * Each function logs its calls into the Recorder and returns the result.
 */

long long recursion_factorial(Recorder *r, long long n);
long long recursion_fib_naive(Recorder *r, long long n);
long long recursion_fib_memo(Recorder *r, long long n);
long long recursion_hanoi(Recorder *r, long long n);
int algo_recursion_handle(const char *body, json_buf *out, int *status);

#endif
