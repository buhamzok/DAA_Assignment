#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#if defined(_MSC_VER)
#pragma comment(lib, "ws2_32.lib")
#endif
#else
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "http.h"
#include "json.h"

#ifdef _WIN32
typedef SOCKET server_sock_t;
#define SERVER_INVALID INVALID_SOCKET
#define SERVER_CLOSESOCK closesocket
#else
typedef int server_sock_t;
#define SERVER_INVALID (-1)
#define SERVER_CLOSESOCK close
#endif

#define PORT 8080
#define BACKLOG 16

static void handle_api(const http_request *req, json_buf *out, int *status)
{
    if (strcmp(req->path, "/api/health") == 0) {
        json_init(out);
        json_begin_object(out, NULL);
        json_add_string(out, "status", "ok");
        json_add_string(out, "app", "Algorithm Explorer");
        json_end_object(out);
        *status = 200;
        return;
    }
    if (strcmp(req->path, "/api/echo") == 0 && strcmp(req->method, "POST") == 0) {
        json_init(out);
        if (req->body_len > 0) {
            json_begin_object(out, NULL);
            json_add_string(out, "received", req->body);
            json_end_object(out);
        } else {
            json_add_string(out, "error", "empty body");
            *status = 400;
            return;
        }
        *status = 200;
        return;
    }
    json_init(out);
    json_begin_object(out, NULL);
    json_add_string(out, "error", "endpoint not found");
    json_end_object(out);
    *status = 404;
}

static const char *mime_from_path(const char *path)
{
    const char *ext = strrchr(path, '.');
    if (ext == NULL) {
        return "application/octet-stream";
    }
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
    if (strcmp(ext, ".js") == 0)  return "application/javascript";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".svg") == 0) return "image/svg+xml";
    if (strcmp(ext, ".ico") == 0) return "image/x-icon";
    return "application/octet-stream";
}

static char *read_file(const char *path, size_t *out_len)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 0) {
        fclose(f);
        return NULL;
    }
    char *data = malloc((size_t)size + 1);
    if (data == NULL) {
        fclose(f);
        return NULL;
    }
    size_t read = fread(data, 1, (size_t)size, f);
    fclose(f);
    data[read] = '\0';
    *out_len = read;
    return data;
}

static void serve_static(int fd, const http_request *req)
{
    if (strstr(req->path, "..") != NULL) {
        http_send_error(fd, 400, "invalid path");
        return;
    }
    const char *rel = req->path;
    if (strcmp(rel, "/") == 0) {
        rel = "/index.html";
    }
    char full[768];
    snprintf(full, sizeof(full), "www%s", rel);
    size_t len = 0;
    char *data = read_file(full, &len);
    if (data == NULL) {
        http_send_error(fd, 404, "file not found");
        return;
    }
    http_send_response(fd, 200, mime_from_path(rel), data, len);
    free(data);
}

static void handle_connection(http_sock_t fd)
{
    http_request req;
    if (http_read_request(fd, &req) != 0) {
        http_send_error(fd, 400, "malformed request");
        HTTP_CLOSESOCK(fd);
        return;
    }

    if (strncmp(req.path, "/api/", 5) == 0) {
        json_buf out;
        int status = 200;
        handle_api(&req, &out, &status);
        http_send_json(fd, status, out.data);
        json_free(&out);
    } else if (strcmp(req.method, "GET") == 0) {
        serve_static(fd, &req);
    } else {
        http_send_error(fd, 405, "method not allowed");
    }

    http_free_request(&req);
    HTTP_CLOSESOCK(fd);
}

#ifdef _WIN32
static DWORD WINAPI connection_thread(LPVOID param)
{
    http_sock_t fd = (http_sock_t)(intptr_t)param;
    handle_connection(fd);
    return 0;
}
#else
static void *connection_thread(void *param)
{
    http_sock_t fd = (http_sock_t)(intptr_t)param;
    handle_connection(fd);
    return NULL;
}
#endif

static server_sock_t server_socket_init(void)
{
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return SERVER_INVALID;
    }
#endif
    server_sock_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == SERVER_INVALID) {
        return SERVER_INVALID;
    }
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        return SERVER_INVALID;
    }
    if (listen(sock, BACKLOG) < 0) {
        return SERVER_INVALID;
    }
    return sock;
}

int main(void)
{
    server_sock_t server = server_socket_init();
    if (server == SERVER_INVALID) {
        fprintf(stderr, "failed to start server on port %d\n", PORT);
        return 1;
    }
    printf("Algorithm Explorer running at http://localhost:%d\n", PORT);

    for (;;) {
#ifdef _WIN32
        SOCKET client = accept(server, NULL, NULL);
        if (client == INVALID_SOCKET) {
            continue;
        }
        HANDLE thread = CreateThread(NULL, 0, connection_thread,
                                     (LPVOID)(intptr_t)client, 0, NULL);
        if (thread != NULL) {
            CloseHandle(thread);
        } else {
            closesocket(client);
        }
#else
        int client = accept(server, NULL, NULL);
        if (client < 0) {
            continue;
        }
        pthread_t thread;
        if (pthread_create(&thread, NULL, connection_thread,
                           (void *)(intptr_t)client) == 0) {
            pthread_detach(thread);
        } else {
            close(client);
        }
#endif
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
