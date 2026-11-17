#ifndef SORTING_API_H
#define SORTING_API_H

#include <stddef.h>
#include "ds.h"
#include "json.h"

/*
 * Shared interface to the sorting algorithms.
 * The benchmark module calls sorting_run directly.
 * Returns 0 when the sort ran, -1 when the name is unknown.
 */

int sorting_run(const char *name, int *arr, size_t n, Recorder *r);
int algo_sort_handle(const char *body, json_buf *out, int *status);

#endif
