/* Hand-written data structures and the shared step recorder.
 *
 * Nothing here uses stdlib containers: the dynamic array, stack and queue
 * are all built by hand so the group can show every allocation and node.
 *
 * The Recorder is the heart of the project demo: every algorithm module
 * logs its steps, comparisons and operations here so
 * the web front end can display exactly what the algorithm did.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "ds.h"

void recorder_init(Recorder *r)
{
    memset(r, 0, sizeof(*r));
}

void recorder_reset(Recorder *r)
{
    memset(r, 0, sizeof(*r));
}

void recorder_log(Recorder *r, const char *fmt, ...)
{
    if (r->step_count >= REC_MAX_STEPS) {
        r->truncated = 1;
        return;
    }
    va_list args;
    va_start(args, fmt);
    vsnprintf(r->steps[r->step_count], REC_STEP_TEXT, fmt, args);
    va_end(args);
    r->step_count++;
}

void recorder_count_op(Recorder *r, long long n)
{
    r->ops += n;
}

void int_array_init(IntArray *a)
{
    a->data = NULL;
    a->size = 0;
    a->cap = 0;
}

int int_array_push(IntArray *a, int value)
{
    if (a->size == a->cap) {
        if (a->cap > SIZE_MAX / 2 ||
            (a->cap == 0 ? 4 : a->cap * 2) > SIZE_MAX / sizeof(*a->data)) {
            return -1;
        }
        size_t new_cap = a->cap == 0 ? 4 : a->cap * 2;
        int *tmp = realloc(a->data, new_cap * sizeof(int));
        if (tmp == NULL) {
            return -1;
        }
        a->data = tmp;
        a->cap = new_cap;
    }
    a->data[a->size++] = value;
    return 0;
}

int int_array_get(IntArray *a, size_t index)
{
    if (index >= a->size) {
        return 0;
    }
    return a->data[index];
}

void int_array_set(IntArray *a, size_t index, int value)
{
    if (index < a->size) {
        a->data[index] = value;
    }
}

void int_array_free(IntArray *a)
{
    free(a->data);
    a->data = NULL;
    a->size = 0;
    a->cap = 0;
}

void int_stack_init(IntStack *s)
{
    s->top = NULL;
    s->size = 0;
}

int int_stack_push(IntStack *s, int value)
{
    IntStackNode *node = malloc(sizeof(IntStackNode));
    if (node == NULL) {
        return -1;
    }
    node->value = value;
    node->next = s->top;
    s->top = node;
    s->size++;
    return 0;
}

int int_stack_pop(IntStack *s, int *out)
{
    if (s->top == NULL) {
        return -1;
    }
    IntStackNode *node = s->top;
    *out = node->value;
    s->top = node->next;
    free(node);
    s->size--;
    return 0;
}

int int_stack_peek(IntStack *s, int *out)
{
    if (s->top == NULL) {
        return -1;
    }
    *out = s->top->value;
    return 0;
}

int int_stack_empty(IntStack *s)
{
    return s->top == NULL;
}

void int_stack_free(IntStack *s)
{
    int value;
    while (int_stack_pop(s, &value) == 0) {
    }
}

void int_queue_init(IntQueue *q)
{
    q->front = NULL;
    q->back = NULL;
    q->size = 0;
}

int int_queue_enqueue(IntQueue *q, int value)
{
    IntQueueNode *node = malloc(sizeof(IntQueueNode));
    if (node == NULL) {
        return -1;
    }
    node->value = value;
    node->next = NULL;
    if (q->back == NULL) {
        q->front = node;
        q->back = node;
    } else {
        q->back->next = node;
        q->back = node;
    }
    q->size++;
    return 0;
}

int int_queue_dequeue(IntQueue *q, int *out)
{
    if (q->front == NULL) {
        return -1;
    }
    IntQueueNode *node = q->front;
    *out = node->value;
    q->front = node->next;
    if (q->front == NULL) {
        q->back = NULL;
    }
    free(node);
    q->size--;
    return 0;
}

int int_queue_empty(IntQueue *q)
{
    return q->front == NULL;
}

void int_queue_free(IntQueue *q)
{
    int value;
    while (int_queue_dequeue(q, &value) == 0) {
    }
}

#ifdef STANDALONE

static void demo_recorder(void)
{
    Recorder r;
    recorder_init(&r);
    recorder_log(&r, "start demo");
    recorder_count_op(&r, 5);
    recorder_log(&r, "counted %lld ops total", r.ops);
    for (size_t i = 0; i < r.step_count; i++) {
        printf("  step %zu: %s\n", i, r.steps[i]);
    }
}

int main(void)
{
    IntArray a;
    int_array_init(&a);
    int_array_push(&a, 10);
    int_array_push(&a, 20);
    int_array_push(&a, 30);
    printf("array: %d %d %d (size=%zu cap=%zu)\n",
           int_array_get(&a, 0), int_array_get(&a, 1), int_array_get(&a, 2),
           a.size, a.cap);
    int_array_free(&a);

    IntStack s;
    int_stack_init(&s);
    int_stack_push(&s, 1);
    int_stack_push(&s, 2);
    int value;
    int_stack_pop(&s, &value);
    printf("stack pop: %d\n", value);
    int_stack_free(&s);

    IntQueue q;
    int_queue_init(&q);
    int_queue_enqueue(&q, 100);
    int_queue_enqueue(&q, 200);
    int_queue_dequeue(&q, &value);
    printf("queue dequeue: %d\n", value);
    int_queue_free(&q);

    printf("recorder demo:\n");
    demo_recorder();
    return 0;
}

#endif
