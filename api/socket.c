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

void *handle_client(void *arg)
{
    ClientContext *client_context = (ClientContext *)arg;
    int client_fd = client_context->client_fd;
    struct sockaddr_in client_addr = client_context->client_addr;
    char request[BUFFER_LEN];
    ParsedHttp parsed_http;
    char response[BUFFER_LEN];
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));

    printf("Thread openened for connection from %s\n", ip);

    ssize_t n = recv(client_fd, request, BUFFER_LEN, 0);
    if (n > 0)
    {
        request[n] = '\0';
    }
    printf("\"\"\"\n%s\n\"\"\"\n", request);
    parse_request(request, &parsed_http, BUFFER_LEN);

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