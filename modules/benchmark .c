/* Timing, input generation and benchmarking.
 *
 * Three jobs:
 *   1. A hand written pseudo random generator (linear congruential
 *      generator) so test arrays are reproducible with a fixed seed.
 *   2. A benchmark runner that measures operation counts and wall clock
 *      time for the sorting and recursion families.
 *   3. A ratio table: measured operations compared against n^2 and
 *      n log n so the report can show which complexity class each
 *      algorithm really belongs to.
 *
 * Endpoint handled here: POST /api/benchmark with body
 *   {"family":"sorting","sizes":[100,500,1000]}
 *   {"family":"recursion","sizes":[20,25]}
 * Response: {"family":..,"rows":[{"algorithm":..,"size":..,"result":..,
 *            "operations":..,"comparisons":..,"swaps":..,
 *            "ops_over_n2":..,"ops_over_nlogn":..,"time_ms":..}]}
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#include "json.h"
#include "ds.h"
#include "minijson.h"
#include "timing.h"
#include "sorting_api.h"
#include "recursion_api.h"

static unsigned long long lcg_state = 42ULL;

void benchmark_seed(unsigned long long seed)
{
    lcg_state = seed;
}

static int lcg_next(void)
{
    lcg_state = lcg_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return (int)((lcg_state >> 33) % 1000003ULL);
}

static void fill_random(int *arr, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        arr[i] = lcg_next();
    }
}

static const char *SORT_NAMES[] = {
    "bubble", "insertion", "selection", "merge", "quick", "heap"
};

static void run_sort_bench(json_buf *out, const size_t *sizes, size_t size_count)
{
    for (size_t s = 0; s < size_count; s++) {
        size_t n = sizes[s];
        if (n == 0 || n > SIZE_MAX / sizeof(int)) {
            return;
        }
        int *source = malloc(n * sizeof(int));
        int *work = malloc(n * sizeof(int));
        if (source == NULL || work == NULL) {
            free(source);
            free(work);
            return;
        }
        fill_random(source, n);

        for (size_t a = 0; a < sizeof(SORT_NAMES) / sizeof(SORT_NAMES[0]); a++) {
            const char *name = SORT_NAMES[a];
            memcpy(work, source, n * sizeof(int));

            Recorder r;
            recorder_init(&r);

            Timer t;
            timer_start(&t);
            sorting_run(name, work, n, &r);
            double ms = timer_stop_ms(&t);

            double n2 = (double)n * (double)n;
            double nlogn = (double)n * log2((double)n > 1.0 ? (double)n : 2.0);

            json_begin_object(out, NULL);
            json_add_string(out, "algorithm", name);
            json_add_int(out, "size", (long long)n);
            json_add_int(out, "operations", r.ops);
            json_add_int(out, "comparisons", r.comparisons);
            json_add_int(out, "swaps", r.swaps);
            json_add_int(out, "writes", r.writes);
            json_add_double(out, "ops_over_n2", (double)r.ops / n2);
            json_add_double(out, "ops_over_nlogn", (double)r.ops / nlogn);
            json_add_double(out, "time_ms", ms);
            json_end_object(out);
        }
        free(source);
        free(work);
    }
}

static void run_recursion_bench(json_buf *out, const size_t *sizes, size_t size_count)
{
    for (size_t s = 0; s < size_count; s++) {
        long long n = (long long)sizes[s];
        if (n > 90) {
            n = 90;
        }

        Recorder r;
        recorder_init(&r);
        Timer t;
        timer_start(&t);
        recursion_fib_naive(&r, n > 25 ? 25 : n);
        double ms = timer_stop_ms(&t);
        json_begin_object(out, NULL);
        json_add_string(out, "algorithm", "fibonacci_naive");
        json_add_int(out, "size", n > 25 ? 25 : n);
        json_add_int(out, "operations", r.ops);
        json_add_double(out, "time_ms", ms);
        json_end_object(out);

        recorder_init(&r);
        timer_start(&t);
        recursion_fib_memo(&r, n);
        ms = timer_stop_ms(&t);
        json_begin_object(out, NULL);
        json_add_string(out, "algorithm", "fibonacci_memo");
        json_add_int(out, "size", n);
        json_add_int(out, "operations", r.ops);
        json_add_double(out, "time_ms", ms);
        json_end_object(out);

        if (n > 15) {
            n = 15;
        }
        recorder_init(&r);
        timer_start(&t);
        long long moves = recursion_hanoi(&r, n);
        ms = timer_stop_ms(&t);
        json_begin_object(out, NULL);
        json_add_string(out, "algorithm", "hanoi");
        json_add_int(out, "size", n);
        json_add_int(out, "result", moves);
        json_add_int(out, "operations", r.ops);
        json_add_double(out, "time_ms", ms);
        json_end_object(out);
    }
}

int algo_benchmark_handle(const char *body, json_buf *out, int *status)
{
    char family[32] = {0};
    size_t *sizes = NULL;
    size_t n_sizes = 0;
    int handled = 1;

    if (body == NULL ||
        minijson_find_value(body, "family", family, sizeof(family)) != 0) {
        return 0;
    }

    int *tmp = NULL;
    if (minijson_extract_int_array(body, "sizes", (int **)&tmp, &n_sizes) != 0) {
        return 0;
    }
    if (n_sizes == 0 || n_sizes > 32 || n_sizes > SIZE_MAX / sizeof(*sizes)) {
        free(tmp);
        return 0;
    }
    sizes = malloc(n_sizes * sizeof(size_t));
    if (sizes == NULL) {
        free(tmp);
        return 0;
    }
    for (size_t i = 0; i < n_sizes; i++) {
        if (tmp[i] <= 0 || (unsigned long long)tmp[i] > 10000ULL) {
            free(tmp);
            free(sizes);
            return 0;
        }
        sizes[i] = (size_t)tmp[i];
    }
    free(tmp);

    json_init(out);
    json_begin_object(out, NULL);
    json_add_string(out, "family", family);
    json_begin_array(out, "sizes");
    for (size_t i = 0; i < n_sizes; i++) {
        json_add_int(out, NULL, (long long)sizes[i]);
    }
    json_end_array(out);
    json_begin_array(out, "rows");

    if (strcmp(family, "sorting") == 0) {
        run_sort_bench(out, sizes, n_sizes);
    } else if (strcmp(family, "recursion") == 0) {
        run_recursion_bench(out, sizes, n_sizes);
    } else {
        handled = 0;
    }

    json_end_array(out);
    json_end_object(out);
    free(sizes);
    if (!handled) {
        json_free(out);
        return 0;
    }
    *status = 200;
    return 1;
}

#ifdef STANDALONE

int main(void)
{
    json_buf out;
    int status = 200;

    printf("sorting benchmark:\n");
    if (algo_benchmark_handle("{\"family\":\"sorting\",\"sizes\":[100,200]}",
                              &out, &status)) {
        printf("%s\n\n", out.data);
        json_free(&out);
    }

    printf("recursion benchmark:\n");
    if (algo_benchmark_handle("{\"family\":\"recursion\",\"sizes\":[20,30]}",
                              &out, &status)) {
        printf("%s\n\n", out.data);
        json_free(&out);
    }
    return 0;
}

#endif
