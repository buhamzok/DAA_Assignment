/* Hand-written sorting algorithms.
 *
 * Six classic sorts on plain int arrays. Every comparison and swap goes
 * through the Recorder so the front end can replay the algorithm move by
 * move and the operation count is visible. No qsort, no library sort.
 *
 *   bubble     adjacent swaps, O(n^2)
 *   insertion  shift into sorted prefix, O(n^2) but fast on nearly sorted
 *   selection  pick minimum of the rest, O(n^2)
 *   merge      divide and conquer with a temp buffer, O(n log n)
 *   quick      partition around pivot, O(n log n) average
 *   heap       max-heap built in place, O(n log n)
 *
 * Endpoint handled here: POST /api/sort with body
 *   {"algorithm":"merge","input":[5,3,1]}
 * Response: {"algorithm":..,"input":[..],"sorted":[..],"operations":..,
 *            "comparisons":..,"swaps":..,"running_time_ms":..,"steps":[..]}
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"
#include "ds.h"
#include "minijson.h"
#include "timing.h"
#include "sorting_api.h"

static void sort_bubble(Recorder *r, int *arr, size_t n);
static void sort_insertion(Recorder *r, int *arr, size_t n);
static void sort_selection(Recorder *r, int *arr, size_t n);
static void sort_merge(Recorder *r, int *arr, size_t n);
static void sort_quick(Recorder *r, int *arr, size_t n);
static void sort_quick_random(Recorder *r, int *arr, size_t n);
static void sort_heap(Recorder *r, int *arr, size_t n);

static void (*sort_lookup(const char *name))(Recorder *, int *, size_t)
{
    if (strcmp(name, "bubble") == 0)    return sort_bubble;
    if (strcmp(name, "insertion") == 0) return sort_insertion;
    if (strcmp(name, "selection") == 0) return sort_selection;
    if (strcmp(name, "merge") == 0)     return sort_merge;
    if (strcmp(name, "quick") == 0)     return sort_quick;
    if (strcmp(name, "quick_random") == 0) return sort_quick_random;
    if (strcmp(name, "heap") == 0)      return sort_heap;
    return NULL;
}

int sorting_run(const char *name, int *arr, size_t n, Recorder *r)
{
    void (*fn)(Recorder *, int *, size_t) = sort_lookup(name);
    if (fn == NULL || r == NULL || (arr == NULL && n != 0)) {
        return -1;
    }
    if (n == 0) {
        recorder_log(r, "%s sort on 0 elements", name);
        return 0;
    }
    fn(r, arr, n);
    return 0;
}

static void swap_log(Recorder *r, int *a, int *b, int *arr, size_t n)
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
    r->swaps++;
    recorder_count_op(r, 1);
    (void)arr;
    (void)n;
}

static void compare_log(Recorder *r, long long a, long long b)
{
    r->comparisons++;
    recorder_count_op(r, 1);
    (void)a;
    (void)b;
}

static void sort_bubble(Recorder *r, int *arr, size_t n)
{
    recorder_log(r, "bubble sort on %zu elements", n);
    for (size_t pass = 0; pass + 1 < n; pass++) {
        int swapped = 0;
        for (size_t i = 0; i + 1 < n - pass; i++) {
            compare_log(r, arr[i], arr[i + 1]);
            recorder_log(r, "pass %zu: compare a[%zu]=%d and a[%zu]=%d",
                         pass, i, arr[i], i + 1, arr[i + 1]);
            if (arr[i] > arr[i + 1]) {
                swap_log(r, &arr[i], &arr[i + 1], arr, n);
                recorder_log(r, "swap a[%zu] and a[%zu]", i, i + 1);
                swapped = 1;
            }
        }
        if (!swapped) {
            recorder_log(r, "pass %zu had no swaps, array sorted early", pass);
            break;
        }
    }
}

static void sort_insertion(Recorder *r, int *arr, size_t n)
{
    recorder_log(r, "insertion sort on %zu elements", n);
    for (size_t i = 1; i < n; i++) {
        int key = arr[i];
        size_t j = i;
        recorder_log(r, "insert a[%zu]=%d into sorted prefix", i, key);
        while (j > 0) {
            compare_log(r, arr[j - 1], key);
            recorder_log(r, "compare a[%zu]=%d with key %d", j - 1, arr[j - 1], key);
            if (arr[j - 1] <= key) {
                break;
            }
            arr[j] = arr[j - 1];
            r->writes++;
            recorder_count_op(r, 1);
            recorder_log(r, "shift a[%zu] right", j - 1);
            j--;
        }
        arr[j] = key;
        recorder_log(r, "place key %d at position %zu", key, j);
    }
}

static void sort_selection(Recorder *r, int *arr, size_t n)
{
    recorder_log(r, "selection sort on %zu elements", n);
    for (size_t i = 0; i + 1 < n; i++) {
        size_t min_index = i;
        for (size_t j = i + 1; j < n; j++) {
            compare_log(r, arr[j], arr[min_index]);
            recorder_log(r, "compare a[%zu]=%d with current min a[%zu]=%d",
                         j, arr[j], min_index, arr[min_index]);
            if (arr[j] < arr[min_index]) {
                min_index = j;
                recorder_log(r, "new minimum a[%zu]=%d", j, arr[j]);
            }
        }
        if (min_index != i) {
            swap_log(r, &arr[i], &arr[min_index], arr, n);
            recorder_log(r, "swap a[%zu] with a[%zu]", i, min_index);
        }
    }
}

static void sort_merge_rec(Recorder *r, int *arr, size_t left, size_t right)
{
    if (left >= right) {
        return;
    }
    size_t mid = left + (right - left) / 2;
    sort_merge_rec(r, arr, left, mid);
    sort_merge_rec(r, arr, mid + 1, right);

    size_t left_len = mid - left + 1;
    size_t right_len = right - mid;
    int *tmp = malloc((left_len + right_len) * sizeof(int));
    if (tmp == NULL) {
        recorder_log(r, "out of memory in merge, aborting");
        return;
    }
    for (size_t i = 0; i < left_len; i++) {
        tmp[i] = arr[left + i];
    }
    for (size_t j = 0; j < right_len; j++) {
        tmp[left_len + j] = arr[mid + 1 + j];
    }

    recorder_log(r, "merge range [%zu..%zu] and [%zu..%zu]",
                 left, mid, mid + 1, right);
    size_t i = 0, j = 0, k = left;
    while (i < left_len && j < right_len) {
        compare_log(r, tmp[i], tmp[left_len + j]);
        recorder_log(r, "compare temp %d and temp %d", tmp[i], tmp[left_len + j]);
        if (tmp[i] <= tmp[left_len + j]) {
            arr[k++] = tmp[i++];
        } else {
            arr[k++] = tmp[left_len + j];
            j++;
            r->writes++;
        }
        recorder_count_op(r, 1);
    }
    while (i < left_len) {
        arr[k++] = tmp[i++];
        recorder_count_op(r, 1);
    }
    while (j < right_len) {
        arr[k++] = tmp[left_len + j++];
        recorder_count_op(r, 1);
    }
    free(tmp);
}

static void sort_merge(Recorder *r, int *arr, size_t n)
{
    recorder_log(r, "merge sort on %zu elements", n);
    if (n > 0) {
        sort_merge_rec(r, arr, 0, n - 1);
    }
}

static unsigned long quick_seed = 2463534242UL;

static unsigned long quick_next_random(void)
{
    quick_seed = quick_seed * 1103515245UL + 12345UL;
    return quick_seed;
}

static void sort_quick_rec(Recorder *r, int *arr, long long low, long long high, int randomized)
{
    if (low >= high) {
        return;
    }
    if (randomized) {
        long long chosen = low + (long long)(quick_next_random() % (unsigned long)(high - low + 1));
        swap_log(r, &arr[low], &arr[chosen], arr, 0);
        recorder_log(r, "randomized pivot: move a[%lld] to first position", chosen);
    }
    int pivot = arr[low];
    recorder_log(r, "partition [%lld..%lld] first-element pivot %d", low, high, pivot);
    long long i = low + 1;
    for (long long j = low + 1; j <= high; j++) {
        compare_log(r, arr[j], pivot);
        recorder_log(r, "compare a[%lld]=%d with pivot %d", j, arr[j], pivot);
        if (arr[j] < pivot) {
            swap_log(r, &arr[i], &arr[j], arr, 0);
            recorder_log(r, "swap a[%lld] and a[%lld]", i, j);
            i++;
        }
    }
    swap_log(r, &arr[low], &arr[i - 1], arr, 0);
    recorder_log(r, "place pivot at position %lld", i - 1);

    sort_quick_rec(r, arr, low, i - 2, randomized);
    sort_quick_rec(r, arr, i, high, randomized);
}

static void sort_quick(Recorder *r, int *arr, size_t n)
{
    recorder_log(r, "quick sort on %zu elements", n);
    if (n > 0) {
        sort_quick_rec(r, arr, 0, (long long)n - 1, 0);
    }
}

static void sort_quick_random(Recorder *r, int *arr, size_t n)
{
    recorder_log(r, "randomized-pivot quick sort on %zu elements", n);
    if (n > 0) {
        sort_quick_rec(r, arr, 0, (long long)n - 1, 1);
    }
}

static void sort_heap(Recorder *r, int *arr, size_t n)
{
    recorder_log(r, "heap sort on %zu elements", n);
    for (long long i = (long long)n / 2 - 1; i >= 0; i--) {
        long long node = i;
        for (;;) {
            long long left = 2 * node + 1;
            long long right = 2 * node + 2;
            long long largest = node;
            if (left < (long long)n) {
                compare_log(r, arr[left], arr[largest]);
                recorder_log(r, "compare left child a[%lld]=%d with a[%lld]=%d",
                             left, arr[left], largest, arr[largest]);
                if (arr[left] > arr[largest]) {
                    largest = left;
                }
            }
            if (right < (long long)n) {
                compare_log(r, arr[right], arr[largest]);
                recorder_log(r, "compare right child a[%lld]=%d with a[%lld]=%d",
                             right, arr[right], largest, arr[largest]);
                if (arr[right] > arr[largest]) {
                    largest = right;
                }
            }
            if (largest == node) {
                break;
            }
            swap_log(r, &arr[node], &arr[largest], arr, 0);
            recorder_log(r, "sift down: swap a[%lld] and a[%lld]", node, largest);
            node = largest;
        }
    }
    recorder_log(r, "max heap built");
    for (long long end = (long long)n - 1; end > 0; end--) {
        swap_log(r, &arr[0], &arr[end], arr, 0);
        recorder_log(r, "move max %d to position %lld", arr[end], end);
        long long node = 0;
        for (;;) {
            long long left = 2 * node + 1;
            long long right = 2 * node + 2;
            long long largest = node;
            if (left < end) {
                compare_log(r, arr[left], arr[largest]);
                if (arr[left] > arr[largest]) {
                    largest = left;
                }
            }
            if (right < end) {
                compare_log(r, arr[right], arr[largest]);
                if (arr[right] > arr[largest]) {
                    largest = right;
                }
            }
            if (largest == node) {
                break;
            }
            swap_log(r, &arr[node], &arr[largest], arr, 0);
            recorder_log(r, "re-heapify: swap a[%lld] and a[%lld]", node, largest);
            node = largest;
        }
    }
}

int algo_sort_handle(const char *body, json_buf *out, int *status)
{
    char algo[32] = {0};
    int *values = NULL;
    size_t n = 0;

    if (body == NULL ||
        minijson_find_value(body, "algorithm", algo, sizeof(algo)) != 0 ||
        minijson_extract_int_array(body, "input", &values, &n) != 0) {
        return 0;
    }

    void (*fn)(Recorder *, int *, size_t) = sort_lookup(algo);

    if (fn == NULL || n > 10000) {
        free(values);
        return 0;
    }

    Recorder r;
    recorder_init(&r);

    Timer t;
    timer_start(&t);
    if (n > 0) {
        fn(&r, values, n);
    } else {
        recorder_log(&r, "%s sort on 0 elements", algo);
    }
    double ms = timer_stop_ms(&t);

    json_init(out);
    json_begin_object(out, NULL);
    json_add_string(out, "algorithm", algo);
    json_add_int(out, "size", (long long)n);
    json_begin_array(out, "sorted");
    for (size_t i = 0; i < n; i++) {
        json_add_int(out, NULL, values[i]);
    }
    json_end_array(out);
    json_add_int(out, "operations", r.ops);
    json_add_int(out, "comparisons", r.comparisons);
    json_add_int(out, "swaps", r.swaps);
    json_add_int(out, "writes", r.writes);
    json_add_double(out, "running_time_ms", ms);
    json_begin_array(out, "steps");
    for (size_t i = 0; i < r.step_count; i++) {
        json_add_string(out, NULL, r.steps[i]);
    }
    json_end_array(out);
    if (r.truncated) {
        json_add_string(out, "steps_note", "too many steps, trace truncated");
    }
    json_end_object(out);
    free(values);
    *status = 200;
    return 1;
}

#ifdef STANDALONE

static void demo(const char *body)
{
    json_buf out;
    int status = 200;
    if (algo_sort_handle(body, &out, &status)) {
        printf("request %s\n", body);
        printf("response: %s\n\n", out.data);
        json_free(&out);
    } else {
        printf("request %s -> not handled\n\n", body);
    }
}

int main(void)
{
    demo("{\"algorithm\":\"bubble\",\"input\":[5,2,9,1,7]}");
    demo("{\"algorithm\":\"insertion\",\"input\":[3,1,2]}");
    demo("{\"algorithm\":\"merge\",\"input\":[8,4,6,2]}");
    demo("{\"algorithm\":\"quick\",\"input\":[10,3,5,1]}");
    demo("{\"algorithm\":\"heap\",\"input\":[4,7,2,9]}");
    demo("{\"algorithm\":\"nope\",\"input\":[1,2,3]}");
    return 0;
}

#endif
