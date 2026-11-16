#include <stdio.h>
#include <string.h>

#include "ds.h"
#include "sorting_api.h"
#include "recursion_api.h"

static int failures;

static void expect(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        failures++;
    }
}

static int sorted(const int *values, size_t n)
{
    for (size_t i = 1; i < n; i++) {
        if (values[i - 1] > values[i]) return 0;
    }
    return 1;
}

int main(void)
{
    const char *names[] = {"bubble", "insertion", "selection", "merge", "quick", "heap"};
    const int input[] = {3, -1, 3, 0, -7, 2};
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        int values[6];
        Recorder r;
        memcpy(values, input, sizeof(values));
        recorder_init(&r);
        expect(sorting_run(names[i], values, 6, &r) == 0, "sort dispatch succeeds");
        expect(sorted(values, 6), "sort handles duplicates and negatives");
    }

    Recorder r;
    recorder_init(&r);
    expect(sorting_run("quick", NULL, 0, &r) == 0, "empty input is accepted");
    expect(recursion_factorial(&r, 0) == 1, "factorial base case");
    recorder_init(&r);
    expect(recursion_fib_naive(&r, 8) == 21, "naive fibonacci result");
    recorder_init(&r);
    expect(recursion_fib_memo(&r, 10) == 55, "memoized fibonacci result");
    recorder_init(&r);
    expect(recursion_hanoi(&r, 4) == 15, "hanoi move count");
    expect(sorting_run("not-a-sort", NULL, 0, &r) == -1, "unknown sort is rejected");

    if (failures != 0) return 1;
    puts("All algorithm tests passed.");
    return 0;
}
