/* MULINDWA EMMANUEL S25B23/059: recursion algorithms.
 *
 * Four recursive algorithms, each one logging every call, return value and
 * base case into the shared Recorder so the browser shows the call stack
 * step by step:
 *   factorial        classic recursion, one call per decrement
 *   fibonacci        naive double recursion, shows exponential blow-up
 *   fibonacci_memo   recursion with a hand written memo table, shows why
 *                    caching kills the exponential cost
 *   hanoi            tower of hanoi, logs every disk move
 *
 * Endpoint handled here: POST /api/recursion with body
 *   {"algorithm":"factorial","input":5}
 * Response: {"algorithm":..,"input":..,"result":..,"operations":..,
 *            "running_time_ms":..,"steps":[...]}
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"
#include "ds.h"
#include "minijson.h"
#include "timing.h"
#include "recursion_api.h"

static long long rec_factorial(Recorder *r, long long n, int depth)
{
    recorder_log(r, "call factorial(%lld) depth %d", n, depth);
    recorder_count_op(r, 1);
    if (n <= 1) {
        recorder_log(r, "base case reached, return 1");
        return 1;
    }
    long long sub = rec_factorial(r, n - 1, depth + 1);
    long long result = n * sub;
    recorder_log(r, "return %lld * %lld = %lld", n, sub, result);
    return result;
}

static long long rec_fib_naive(Recorder *r, long long n, int depth)
{
    recorder_log(r, "call fib_naive(%lld) depth %d", n, depth);
    recorder_count_op(r, 1);
    if (n <= 1) {
        recorder_log(r, "base case fib(%lld) = %lld", n, n);
        return n;
    }
    return rec_fib_naive(r, n - 1, depth + 1) + rec_fib_naive(r, n - 2, depth + 1);
}

static long long rec_fib_memo(Recorder *r, long long n, long long *memo, int depth)
{
    recorder_log(r, "call fib_memo(%lld) depth %d", n, depth);
    recorder_count_op(r, 1);
    if (n <= 1) {
        recorder_log(r, "base case fib(%lld) = %lld", n, n);
        return n;
    }
    if (memo[n] >= 0) {
        recorder_log(r, "memo hit: fib(%lld) already known = %lld", n, memo[n]);
        return memo[n];
    }
    long long value = rec_fib_memo(r, n - 1, memo, depth + 1) +
                      rec_fib_memo(r, n - 2, memo, depth + 1);
    memo[n] = value;
    recorder_log(r, "store memo[%lld] = %lld", n, value);
    return value;
}

static void rec_hanoi(Recorder *r, int n, int from, int to, int via, long long *moves)
{
    if (n == 0) {
        return;
    }
    recorder_count_op(r, 1);
    rec_hanoi(r, n - 1, from, via, to, moves);
    recorder_log(r, "move disk %d: peg %d -> peg %d", n, from, to);
    (*moves)++;
    rec_hanoi(r, n - 1, via, to, from, moves);
}

long long member3_factorial(Recorder *r, long long n)
{
    return rec_factorial(r, n, 0);
}

long long member3_fib_naive(Recorder *r, long long n)
{
    return rec_fib_naive(r, n, 0);
}

long long member3_fib_memo(Recorder *r, long long n)
{
    long long result = 0;
    long long *memo = malloc((n + 1) * sizeof(long long));
    if (memo == NULL) {
        recorder_log(r, "out of memory for memo table");
        return -1;
    }
    for (long long i = 0; i <= n; i++) {
        memo[i] = -1;
    }
    result = rec_fib_memo(r, n, memo, 0);
    free(memo);
    return result;
}

long long member3_hanoi(Recorder *r, long long n)
{
    long long moves = 0;
    rec_hanoi(r, (int)n, 1, 3, 2, &moves);
    return moves;
}

int algo_recursion_handle(const char *body, json_buf *out, int *status)
{
    char algo[32] = {0};
    long long input = 0;

    if (body == NULL ||
        minijson_find_value(body, "algorithm", algo, sizeof(algo)) != 0 ||
        minijson_extract_int(body, "input", &input) != 0) {
        return 0;
    }

    Recorder r;
    recorder_init(&r);
    long long result = 0;
    int handled = 1;

    Timer t;
    timer_start(&t);

    if (strcmp(algo, "factorial") == 0) {
        if (input < 0 || input > 20) {
            handled = 0;
        } else {
            result = rec_factorial(&r, input, 0);
        }
    } else if (strcmp(algo, "fibonacci") == 0) {
        if (input < 0 || input > 25) {
            handled = 0;
        } else {
            result = rec_fib_naive(&r, input, 0);
        }
    } else if (strcmp(algo, "fibonacci_memo") == 0) {
        if (input < 0 || input > 90) {
            handled = 0;
        } else {
            result = member3_fib_memo(&r, input);
        }
    } else if (strcmp(algo, "hanoi") == 0) {
        if (input < 1 || input > 15) {
            handled = 0;
        } else {
            long long moves = 0;
            rec_hanoi(&r, (int)input, 1, 3, 2, &moves);
            result = moves;
        }
    } else {
        handled = 0;
    }

    if (!handled) {
        return 0;
    }

    double ms = timer_stop_ms(&t);

    json_init(out);
    json_begin_object(out, NULL);
    json_add_string(out, "algorithm", algo);
    json_add_int(out, "input", input);
    json_add_int(out, "result", result);
    json_add_int(out, "operations", r.ops);
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
    *status = 200;
    return 1;
}

#ifdef STANDALONE

static void demo(const char *body)
{
    json_buf out;
    int status = 200;
    if (algo_recursion_handle(body, &out, &status)) {
        printf("request %s\n", body);
        printf("response: %s\n\n", out.data);
        json_free(&out);
    } else {
        printf("request %s -> not handled\n\n", body);
    }
}

int main(void)
{
    demo("{\"algorithm\":\"factorial\",\"input\":5}");
    demo("{\"algorithm\":\"fibonacci\",\"input\":8}");
    demo("{\"algorithm\":\"fibonacci_memo\",\"input\":10}");
    demo("{\"algorithm\":\"hanoi\",\"input\":3}");
    demo("{\"algorithm\":\"unknown\",\"input\":3}");
    return 0;
}

#endif
