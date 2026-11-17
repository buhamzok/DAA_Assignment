#ifndef DS_H
#define DS_H

#include <stddef.h>
#include <stdarg.h>

/*
 * Hand-written data structures with no standard-library containers.
 * Algorithm modules include this shared header.
 */

#define REC_MAX_STEPS 4096
#define REC_STEP_TEXT 96

typedef struct {
    long long comparisons;
    long long swaps;
    long long writes;
    long long ops;
    char steps[REC_MAX_STEPS][REC_STEP_TEXT];
    size_t step_count;
    int truncated;
} Recorder;

void recorder_init(Recorder *r);
void recorder_reset(Recorder *r);
void recorder_log(Recorder *r, const char *fmt, ...);
void recorder_count_op(Recorder *r, long long n);

typedef struct {
    int *data;
    size_t size;
    size_t cap;
} IntArray;

void int_array_init(IntArray *a);
int int_array_push(IntArray *a, int value);
int int_array_get(IntArray *a, size_t index);
void int_array_set(IntArray *a, size_t index, int value);
void int_array_free(IntArray *a);

typedef struct IntStackNode {
    int value;
    struct IntStackNode *next;
} IntStackNode;

typedef struct {
    IntStackNode *top;
    size_t size;
} IntStack;

void int_stack_init(IntStack *s);
int int_stack_push(IntStack *s, int value);
int int_stack_pop(IntStack *s, int *out);
int int_stack_peek(IntStack *s, int *out);
int int_stack_empty(IntStack *s);
void int_stack_free(IntStack *s);

typedef struct IntQueueNode {
    int value;
    struct IntQueueNode *next;
} IntQueueNode;

typedef struct {
    IntQueueNode *front;
    IntQueueNode *back;
    size_t size;
} IntQueue;

void int_queue_init(IntQueue *q);
int int_queue_enqueue(IntQueue *q, int value);
int int_queue_dequeue(IntQueue *q, int *out);
int int_queue_empty(IntQueue *q);
void int_queue_free(IntQueue *q);

#endif
