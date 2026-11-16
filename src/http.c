#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#endif

static int send_all(http_sock_t fd, const char *data, size_t len)
{
    size_t sent = 0;
    while (sent < len) {
#ifdef _WIN32
        int n = send(fd, data + sent, (int)(len - sent), 0);
        if (n == SOCKET_ERROR) {
            return -1;
        }
#else
        ssize_t n = send(fd, data + sent, len - sent, 0);
        if (n <= 0) {
            return -1;
        }
#endif
        sent += (size_t)n;
    }
    return 0;
}

const char *http_status_text(int status)
{
    switch (status) {
    case 200: return "OK";
    case 400: return "Bad Request";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 413: return "Payload Too Large";
    case 500: return "Internal Server Error";
    default:  return "OK";
    }
}

static void parse_header_int(const char *headers, const char *name, size_t *out)
{
    size_t name_len = strlen(name);
    const char *p = headers;
    while ((p = strstr(p, name)) != NULL) {
        if ((p == headers || p[-1] == '\n' || p[-1] == ':') &&
            (strncasecmp(p, name, name_len) == 0)) {
            const char *colon = strchr(p, ':');
            if (colon && colon - p < (ptrdiff_t)name_len + 4) {
                *out = (size_t)strtoul(colon + 1, NULL, 10);
                return;
            }
        }
        p += name_len;
    }
}

int http_read_request(http_sock_t fd, http_request *req)
{
    size_t cap = 8192;
    size_t len = 0;
    char *buf = malloc(cap);
    size_t header_end = 0;

    if (buf == NULL) {
        return -1;
    }
    memset(req, 0, sizeof(*req));

    for (;;) {
        if (len + 4096 > cap) {
            if (cap >= HTTP_MAX_REQUEST) {
                free(buf);
                return -1;
            }
            cap *= 2;
            if (cap > HTTP_MAX_REQUEST) {
                cap = HTTP_MAX_REQUEST;
            }
            char *tmp = realloc(buf, cap);
            if (tmp == NULL) {
                free(buf);
                return -1;
            }
            buf = tmp;
        }
#ifdef _WIN32
        int n = recv(fd, buf + len, (int)(cap - len - 1), 0);
        if (n == SOCKET_ERROR || n == 0) {
            free(buf);
            return -1;
        }
#else
        ssize_t n = recv(fd, buf + len, cap - len - 1, 0);
        if (n <= 0) {
            free(buf);
            return -1;
        }
#endif
        len += (size_t)n;
        buf[len] = '\0';

        char *sep = strstr(buf, "\r\n\r\n");
        if (sep != NULL) {
            header_end = (size_t)(sep - buf) + 4;
            parse_header_int(buf, "Content-Length", &req->content_length);
            if (header_end + req->content_length > HTTP_MAX_REQUEST) {
                free(buf);
                return -1;
            }
            if (header_end + req->content_length <= len) {
                break;
            }
        }
        if (len >= HTTP_MAX_REQUEST) {
            free(buf);
            return -1;
        }
    }

    char *line_end = strstr(buf, "\r\n");
    if (line_end == NULL) {
        free(buf);
        return -1;
    }
    *line_end = '\0';
    if (sscanf(buf, "%7s %511s %15s", req->method, req->path, req->version) != 3) {
        free(buf);
        return -1;
    }

    req->body = malloc(req->content_length + 1);
    if (req->body == NULL) {
        free(buf);
        return -1;
    }
    memcpy(req->body, buf + header_end, req->content_length);
    req->body[req->content_length] = '\0';
    req->body_len = req->content_length;

    free(buf);
    return 0;
}

void http_free_request(http_request *req)
{
    free(req->body);
    req->body = NULL;
    req->body_len = 0;
}

int http_send_response(http_sock_t fd, int status, const char *content_type,
                       const char *body, size_t body_len)
{
    char header[512];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        status, http_status_text(status), content_type, body_len);
    if (header_len < 0 || (size_t)header_len >= sizeof(header)) {
        return -1;
    }
    if (send_all(fd, header, (size_t)header_len) != 0) {
        return -1;
    }
    if (body_len > 0 && send_all(fd, body, body_len) != 0) {
        return -1;
    }
    return 0;
}

int http_send_json(http_sock_t fd, int status, const char *json_body)
{
    return http_send_response(fd, status, "application/json",
                              json_body, strlen(json_body));
}

int http_send_error(http_sock_t fd, int status, const char *message)
{
    char body[256];
    snprintf(body, sizeof(body), "{\"error\":\"%s\"}", message);
    return http_send_json(fd, status, body);
}
