#ifndef JSON_H
#define JSON_H

#include <stddef.h>

typedef struct {
    char *data;
    size_t len;
    size_t cap;
    int error;
} json_buf;

void json_init(json_buf *jb);
void json_free(json_buf *jb);

void json_add_raw(json_buf *jb, const char *raw);
void json_add_string(json_buf *jb, const char *key, const char *value);
void json_add_int(json_buf *jb, const char *key, long long value);
void json_add_double(json_buf *jb, const char *key, double value);

void json_begin_array(json_buf *jb, const char *key);
void json_end_array(json_buf *jb);
void json_begin_object(json_buf *jb, const char *key);
void json_end_object(json_buf *jb);

const char *json_escape_append(json_buf *jb, const char *text);

#endif
