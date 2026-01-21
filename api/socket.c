#include <stdio.h>
#include <stdlib.h>
// socket()
#include <sys/socket.h>
// sockaddr_in
#include <netinet/in.h>
// htons()
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "http_parser.h"

#define PORT 3500
#define BUFFER_LEN 1024

typedef struct
{
    int client_fd;
    struct sockaddr_in client_addr;
} ClientContext;

void *memmem_simple(const void *haystack, size_t hay_len,
                    const void *needle, size_t nee_len)
{
    if (nee_len == 0 || hay_len < nee_len)
        return NULL;

    const unsigned char *h = haystack;
    const unsigned char *n = needle;

    for (size_t i = 0; i <= hay_len - nee_len; i++)
    {
        if (memcmp(h + i, n, nee_len) == 0)
            return (void *)(h + i);
    }
    return NULL;
}

void *handle_client(void *arg)
{
    ClientContext *client_context = (ClientContext *)arg;
    int client_fd = client_context->client_fd;
    // struct sockaddr_in client_addr = client_context->client_addr;

    ParsedHttp parsed_http;
    parsed_http_init(&parsed_http);

    ssize_t used = 0;
    ssize_t r = 0;
    char tmp[BUFFER_LEN];
    char *out = NULL;
    char *needle_loc;

    while (1)
    {
        r = recv(client_fd, tmp, BUFFER_LEN, 0);
        if (r <= 0)
        {
            perror("recv failed");
            free(out);
            close(client_fd);
            free(arg);
            return NULL;
        }
        used += r;
        char *tmp_out = realloc(out, used * sizeof(char));
        if (!tmp_out)
        {
            fprintf(stderr, "realloc err\n");
            free(out);
            close(client_fd);
            free(arg);
            return NULL;
        }
        out = tmp_out;
        memcpy(out + used - r, tmp, r * sizeof(char));
        if ((needle_loc = (char *)memmem_simple(out, used, "\r\n\r\n", 4)))
        {
            break;
        }
    }
    int content_length = parse_headers(out, &parsed_http.request_line, &parsed_http.headers);
    if (content_length)
    {
        int body_idx = needle_loc - out + 4;
        int body_recieved = used - body_idx;
        int remaining = content_length - body_recieved;
        if (remaining > 0)
        {
            char *tmp_out = realloc(out, (used + remaining) * sizeof(char));
            if (!tmp_out)
            {
                fprintf(stderr, "realloc err\n");
                free(out);
                close(client_fd);
                free(arg);
                return NULL;
            }
            out = tmp_out;
            r = 0;
            while (r < remaining)
            {
                ssize_t want = remaining - r;
                ssize_t chunk = want > BUFFER_LEN ? BUFFER_LEN : want;

                ssize_t n = recv(client_fd, tmp, chunk, 0);
                if (n <= 0)
                {
                    perror("recv failed");
                    free(out);
                    close(client_fd);
                    free(arg);
                    return NULL;
                }
                memcpy(out + used + r, tmp, n * sizeof(char));
                r += n;
            }
            used += remaining;
        }
        parse_body(out, body_idx, content_length, &parsed_http.body);
    }

    parsed_http_print(parsed_http);

    parsed_http_free(&parsed_http);
    free(out);
    close(client_fd);
    free(arg);
    return NULL;
}

int main()
{
    int socket_fd;
    int opt = 1;
    int err = 0;
    struct sockaddr_in server_addr;

    signal(SIGPIPE, SIG_IGN);

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        err = errno;
        fprintf(stderr, "socket failed: %s\n", strerror(err));
        return 1;
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        err = errno;
        fprintf(stderr, "setsockopt failed: %s\n", strerror(err));
        close(socket_fd);
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        err = errno;
        fprintf(stderr, "bind failed: %s\n", strerror(err));
        close(socket_fd);
        return 1;
    }

    if (listen(socket_fd, 3) < 0)
    {
        err = errno;
        fprintf(stderr, "listen failed: %s\n", strerror(err));
        close(socket_fd);
        return 1;
    }

    printf("Server listening on %d\n", PORT);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    int client_fd;
    ClientContext *client_context;
    while (1)
    {
        client_addr_len = sizeof(client_addr);

        if ((client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0)
        {
            err = errno;
            fprintf(stderr, "accept failed: %s\n", strerror(err));
            continue;
        }

        client_context = malloc(sizeof(ClientContext));
        if (!client_context)
        {
            close(client_fd);
            continue;
        }

        client_context->client_fd = client_fd;
        client_context->client_addr = client_addr;

        pthread_t thread_id;
        if ((pthread_create(&thread_id, NULL, handle_client, (void *)client_context)) != 0)
        {
            err = errno;
            fprintf(stderr, "pthread_create failed: %s\n", strerror(err));
            free(client_context);
            close(client_fd);
            continue;
        }
        pthread_detach(thread_id);
    }

    close(socket_fd);
    return 0;
}