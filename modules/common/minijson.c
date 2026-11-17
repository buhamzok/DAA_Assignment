/* Minimal JSON value extraction for flat request bodies.
 * Used by request handlers to read {"key": value} inputs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>

#include "minijson.h"

static const char *find_key_colon(const char *body, const char *key)
{
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = body;
    while ((p = strstr(p, pattern)) != NULL) {
        const char *q = p + strlen(pattern);
        while (*q == ' ' || *q == ':') {
            if (*q == ':') {
                return q + 1;
            }
            q++;
        }
        p += strlen(pattern);
    }
    return NULL;
}

int minijson_find_value(const char *body, const char *key,
                        char *out, size_t out_size)
{
    const char *p = find_key_colon(body, key);
    if (p == NULL || out_size == 0) {
        return -1;
    }
    while (*p == ' ') {
        p++;
    }
    if (*p == '"') {
        p++;
        size_t n = 0;
        while (*p != '\0' && *p != '"') {
            if (*p == '\\' && p[1] != '\0') {
                p++;
            }
            if (n + 1 < out_size) {
                out[n++] = *p;
            }
            p++;
        }
        out[n] = '\0';
        return *p == '"' ? 0 : -1;
    }
    const char *end = p;
    while (*end != '\0' && *end != ',' && *end != '}') {
        end++;
    }
    size_t len = (size_t)(end - p);
    if (len >= out_size) {
        len = out_size - 1;
    }
    memcpy(out, p, len);
    out[len] = '\0';
    return 0;
}

int minijson_extract_int(const char *body, const char *key, long long *out)
{
    char buf[64];
    if (minijson_find_value(body, key, buf, sizeof(buf)) != 0) {
        return -1;
    }
    char *end = NULL;
    long long value = strtoll(buf, &end, 10);
    if (end == buf) {
        return -1;
    }
    *out = value;
    return 0;
}

int minijson_extract_int_array(const char *body, const char *key,
                               int **out, size_t *out_len)
{
    const char *p = find_key_colon(body, key);
    if (p == NULL) {
        return -1;
    }
    while (*p == ' ') {
        p++;
    }
    if (*p != '[') {
        return -1;
    }
    p++;

    size_t cap = 16;
    size_t n = 0;
    int *values = malloc(cap * sizeof(int));
    if (values == NULL) {
        return -1;
    }

    for (;;) {
        while (*p == ' ' || *p == ',') {
            p++;
        }
        if (*p == ']') {
            break;
        }
        char *end = NULL;
        long long value = strtoll(p, &end, 10);
        if (end == p) {
            free(values);
            return -1;
        }
        if (value < INT_MIN || value > INT_MAX) {
            free(values);
            return -1;
        }
        if (n == cap) {
            if (cap > SIZE_MAX / 2 || cap * 2 > SIZE_MAX / sizeof(*values)) {
                free(values);
                return -1;
            }
            cap *= 2;
            int *tmp = realloc(values, cap * sizeof(int));
            if (tmp == NULL) {
                free(values);
                return -1;
            }
            values = tmp;
        }
        values[n++] = (int)value;
        p = end;
    }

    *out = values;
    *out_len = n;
    return 0;
}
