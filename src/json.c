#include "json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>

static void json_ensure(json_buf *jb, size_t extra)
{
    if (jb->len + extra + 1 > jb->cap) {
        size_t new_cap = jb->cap == 0 ? 1024 : jb->cap * 2;
        while (jb->len + extra + 1 > new_cap) {
            new_cap *= 2;
        }
        char *tmp = realloc(jb->data, new_cap);
        if (tmp == NULL) {
            jb->error = 1;
            return;
        }
        jb->data = tmp;
        jb->cap = new_cap;
    }
}

static void json_write(json_buf *jb, const char *text)
{
    size_t n = strlen(text);
    json_ensure(jb, n);
    if (jb->error) {
        return;
    }
    memcpy(jb->data + jb->len, text, n);
    jb->len += n;
    jb->data[jb->len] = '\0';
}

void json_init(json_buf *jb)
{
    jb->data = NULL;
    jb->len = 0;
    jb->cap = 0;
    jb->error = 0;
    json_ensure(jb, 1);
    if (!jb->error) {
        jb->data[0] = '\0';
    }
}

void json_free(json_buf *jb)
{
    free(jb->data);
    jb->data = NULL;
    jb->len = 0;
    jb->cap = 0;
}

const char *json_escape_append(json_buf *jb, const char *text)
{
    char tmp[8];
    for (const char *p = text; *p != '\0'; p++) {
        unsigned char c = (unsigned char)*p;
        switch (c) {
        case '"':  json_write(jb, "\\\""); break;
        case '\\': json_write(jb, "\\\\"); break;
        case '\b': json_write(jb, "\\b"); break;
        case '\f': json_write(jb, "\\f"); break;
        case '\n': json_write(jb, "\\n"); break;
        case '\r': json_write(jb, "\\r"); break;
        case '\t': json_write(jb, "\\t"); break;
        default:
            if (c < 0x20) {
                snprintf(tmp, sizeof(tmp), "\\u%04x", c);
                json_write(jb, tmp);
            } else {
                tmp[0] = (char)c;
                tmp[1] = '\0';
                json_write(jb, tmp);
            }
        }
    }
    return jb->data;
}

static void json_sep(json_buf *jb)
{
    if (jb->len == 0) {
        return;
    }
    char last = jb->data[jb->len - 1];
    if (last != '{' && last != '[' && last != ':') {
        json_write(jb, ",");
    }
}

void json_add_raw(json_buf *jb, const char *raw)
{
    json_write(jb, raw);
}

void json_add_string(json_buf *jb, const char *key, const char *value)
{
    json_sep(jb);
    json_write(jb, "\"");
    json_write(jb, key);
    json_write(jb, "\":\"");
    json_escape_append(jb, value);
    json_write(jb, "\"");
}

void json_add_int(json_buf *jb, const char *key, long long value)
{
    json_sep(jb);
    char num[32];
    snprintf(num, sizeof(num), "%lld", value);
    json_write(jb, "\"");
    json_write(jb, key);
    json_write(jb, "\":");
    json_write(jb, num);
}

void json_add_double(json_buf *jb, const char *key, double value)
{
    json_sep(jb);
    char num[64];
    snprintf(num, sizeof(num), "%.3f", value);
    json_write(jb, "\"");
    json_write(jb, key);
    json_write(jb, "\":");
    json_write(jb, num);
}

void json_begin_array(json_buf *jb, const char *key)
{
    json_sep(jb);
    if (key != NULL) {
        json_write(jb, "\"");
        json_write(jb, key);
        json_write(jb, "\":");
    }
    json_write(jb, "[");
}

void json_end_array(json_buf *jb)
{
    json_write(jb, "]");
}

void json_begin_object(json_buf *jb, const char *key)
{
    json_sep(jb);
    if (key != NULL) {
        json_write(jb, "\"");
        json_write(jb, key);
        json_write(jb, "\":");
    }
    json_write(jb, "{");
}

void json_end_object(json_buf *jb)
{
    json_write(jb, "}");
}
