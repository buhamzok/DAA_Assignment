#ifndef MINIJSON_H
#define MINIJSON_H

#include <stddef.h>

/*
 * Minimal JSON helpers shared by the request handlers.
 * The bodies we care about are flat objects such as:
 *   {"algorithm":"bubble","input":[5,3,1]}
 *   {"algorithm":"fibonacci","input":10}
 * Full JSON parsing is out of scope, these helpers scan for one key.
 */

int minijson_find_value(const char *body, const char *key,
                        char *out, size_t out_size);

int minijson_extract_int(const char *body, const char *key, long long *out);

int minijson_extract_int_array(const char *body, const char *key,
                               int **out, size_t *out_len);

#endif
