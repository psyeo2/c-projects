#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <poll.h>
#include <fcntl.h>

#include "types.h"
#include "http_parser.h"
#include "router.h"

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
    ErrorCode error_code = OK;
    ClientContext *client_context = (ClientContext *)arg;
    int client_fd = client_context->client_fd;
    // struct sockaddr_in client_addr = client_context->client_addr;

    ParsedRequest parsed_request;
    parsed_request_init(&parsed_request);

    ssize_t used = 0;
    ssize_t r = 0;
    char tmp[BUFFER_LEN];
    char *out = NULL;
    char *needle_loc;

    while (1)
    {
        if (used > MAX_BODY_LENGTH)
        {
            error_code = ERR_CONTENT_LENGTH;
            log_error(error_code);
            parsed_request_free(&parsed_request);
            free(out);
            close(client_fd);
            free(arg);
            return NULL;
        }
        r = recv(client_fd, tmp, BUFFER_LEN, 0);
        if (r <= 0)
        {
            perror("recv failed");
            parsed_request_free(&parsed_request);
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
            parsed_request_free(&parsed_request);
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
    int content_length = 0;
    error_code = parse_headers(
        out,
        &parsed_request.request_line,
        &parsed_request.headers,
        &content_length);
    if (error_code)
    {
        log_error(error_code);
        parsed_request_free(&parsed_request);
        free(out);
        close(client_fd);
        free(arg);
        return NULL;
    }
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
                parsed_request_free(&parsed_request);
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
                    parsed_request_free(&parsed_request);
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
        error_code = parse_body(out, body_idx, content_length, &parsed_request.body);
        if (error_code)
        {
            log_error(error_code);
            parsed_request_free(&parsed_request);
            free(out);
            close(client_fd);
            free(arg);
            return NULL;
        }
    }

    parsed_request_print(parsed_request);

    HttpResponse http_response = router(parsed_request);
    char *flattened_response;
    int len = 0;
    flattened_response = http_response_flatten(http_response, &len);
    send(client_fd, flattened_response, len, 0);

    free(flattened_response);
    http_response_free(&http_response);
    parsed_request_free(&parsed_request);
    free(out);
    close(client_fd);
    free(arg);
    return NULL;
}

int request(Connection *c)
{
    char buffer[BUFFER_LEN];
    ssize_t r = recv(c->fd, buffer, BUFFER_LEN, 0);
    if (r == 0)
    {
        close(c->fd);
        return -1;
    }
    if (r < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
        close(c->fd);
        return -1;
    }

    char *needle_loc;
    switch (c->state)
    {
    case CONN_READING_HEADERS:
        char *p = c->buffer + c->buffer_used;
        if (c->buffer_used + r > MAX_HEADER_LENGTH)
        {
            log_error(ERR_HEADER_LENGTH);
            return -1;
        }
        memcpy(p, buffer, r);
        c->buffer_used += r;
        if ((needle_loc = (char *)memmem_simple(c->buffer, c->buffer_used, "\r\n\r\n", 4)))
        {
            ErrorCode e = parse_headers(c->buffer, &c->req.request_line, &c->req.headers, &c->content_length);
            log_error(e);
            if (c->content_length)
            {
                if (c->content_length > MAX_BODY_LENGTH)
                {
                    log_error(ERR_CONTENT_LENGTH);
                    return -1;
                }
                int body_idx = needle_loc - c->buffer + 4;
                c->buffer_used = c->buffer_used - body_idx;
                p = c->buffer + body_idx;
                c->req.body = malloc((c->content_length + 1) * sizeof(char));
                memcpy(c->req.body, p, c->buffer_used);
                c->state = CONN_READING_BODY;
                return 0;
            }
            c->state = CONN_PROCESSING;
            return 1;
        }
        return 0;
    case CONN_READING_BODY:
        if (c->buffer_used + r > MAX_BODY_LENGTH)
        {
            log_error(ERR_CONTENT_LENGTH);
            return -1;
        }
        int remaining = c->content_length - c->buffer_used;
        if (remaining <= 0)
        {
            c->state = CONN_PROCESSING;
            return 1;
        }
        size_t to_copy = r > remaining ? remaining : r;
        memcpy(c->req.body + c->buffer_used, buffer, to_copy);
        c->buffer_used += to_copy;
        if (c->buffer_used >= c->content_length)
        {
            c->req.body[c->content_length] = '\0';
            c->state = CONN_PROCESSING;
            return 1;
        }
        return 0;
    default:
        printf("How did you get here?\n");
        return -1;
    }
}

void response(Connection *c)
{
}

int get_port()
{
    const char *env = getenv("PORT");
    if (!env)
    {
        fprintf(stderr, "Missing PORT environment variable, falling back to 3500.\n");
        return 3500;
    }
    char *end;
    int port = (int)strtol(env, &end, 10);
    if (port <= 0 || port > 65535)
    {
        fprintf(stderr, "Port out of range, falling back to 3500.\n");
        return 3500;
    }
    return port;
}

void connection_init(Connection *c)
{
    c->fd = -1;
    c->state = CONN_READING_HEADERS;
    c->buffer[0] = '\0';
    c->buffer_used = 0;
    c->content_length = 0;
    parsed_request_init(&c->req);
    http_response_init(&c->res);
}

void connection_reset(Connection *c)
{
    connection_free(c);
    connection_init(c);
}

void connection_free(Connection *c)
{
    if (c->fd > -1)
        close(c->fd);
    parsed_request_free(&c->req);
    http_response_free(&c->res);
}

int main()
{
    int port = get_port();
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
    server_addr.sin_port = htons(port);
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

    printf("Server listening on %d\n", port);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd;
    ClientContext *client_context;

    struct pollfd fds[MAX_CLIENTS + 1];
    Connection connections[MAX_CLIENTS + 1];
    int flags = fcntl(client_fd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("flags init failed");
    }
    for (int i = 1; i < MAX_CLIENTS + 1; i++)
    {
        connection_init(&connections[i]);
    }
    for (int i = 0; i < MAX_CLIENTS + 1; i++)
    {
        fds[i].fd = -1;
    }
    fds[0].fd = socket_fd;
    fds[0].events = POLLIN;

    while (1)
    {
        poll(fds, MAX_CLIENTS + 1, TIMEOUT);

        // handle new incoming connection
        if (fds[0].revents & POLLIN)
        {
            if ((client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0)
            {
                perror("accept failed");
                continue;
            }
            if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1)
            {
                perror("set non blocking failed");
                continue;
            }
            for (int i = 1; i < MAX_CLIENTS + 1; i++)
            {
                if (fds[i].fd < 0)
                {
                    fds[i].fd = client_fd;
                    fds[i].events = POLLIN;
                    connection_reset(&connections[i]);
                    connections[i].fd = client_fd;
                    break;
                }
            }
        }
        // client_context = malloc(sizeof(ClientContext));
        // if (!client_context)
        // {
        //     close(client_fd);
        //     continue;
        // }

        // client_context->client_fd = client_fd;
        // client_context->client_addr = client_addr;

        for (int i = 1; i < MAX_CLIENTS + 1; i++)
        {
            if (fds[i].fd == -1)
                continue;

            if (fds[i].revents & POLLIN)
            {
                int req_ret = request(&connections[i]);
                switch (req_ret)
                {
                case -1:
                    connection_reset(&fds[i]);
                    continue;
                case 0:
                    continue;
                case 1:
                    router(connections[i]);
                    break;
                }
            }
            if (fds[i].revents & POLLOUT)
            {
                response(&connections[i]);
            }
            if (fds[i].revents & (POLLHUP | POLLERR | POLLNVAL))
            {
                close(fds[i].fd);
                fds[i].fd = -1;
            }
        }
    }

    for (int i = 1; i < MAX_CLIENTS + 1; i++)
    {
        connection_free(&connections[i]);
    }

    close(socket_fd);
    return 0;
}