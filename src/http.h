#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET http_sock_t;
#define HTTP_CLOSESOCK closesocket
#else
typedef int http_sock_t;
#define HTTP_CLOSESOCK close
#endif

#define HTTP_MAX_REQUEST (1024 * 1024)

typedef struct {
    char method[8];
    char path[512];
    char version[16];
    size_t content_length;
    char *body;
    size_t body_len;
} http_request;

int http_read_request(http_sock_t fd, http_request *req);
void http_free_request(http_request *req);

int http_send_response(http_sock_t fd, int status, const char *content_type,
                       const char *body, size_t body_len);
int http_send_json(http_sock_t fd, int status, const char *json_body);
int http_send_error(http_sock_t fd, int status, const char *message);

const char *http_status_text(int status);

#endif
