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
#include <stdint.h>
#include <sys/eventfd.h>

#include "types.h"
#include "http_parser.h"
#include "router.h"
#include "lib/thread_pool.h"

typedef struct
{
    int client_fd;
    struct sockaddr_in client_addr;
} ClientContext;

static int worker_done_fd = -1;

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

int request(Connection *c)
{
    char buffer[BUFFER_LEN];
    ssize_t r = recv(c->fd, buffer, BUFFER_LEN - 1, 0);
    if (r > 0)
    {
        buffer[r] = '\0';
    }
    if (r == 0)
    {
        return -1; // should not be -1
    }
    if (r < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
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
        // if (r > 0)
        // {
        //     c->buffer[c->buffer_used] = '\0'; // todo: this is good notionally, but might overwrite body bytes
        // }
        if ((needle_loc = (char *)memmem_simple(c->buffer, c->buffer_used, "\r\n\r\n", 4)))
        {
            ErrorCode e = parse_headers(c->buffer, &c->req.request_line, &c->req.headers, &c->content_length);
            if (e)
            {
                log_error(e);
                return -1;
            }
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
                if (!c->req.body)
                {
                    perror("body malloc failed");
                    return -1;
                }
                if (c->buffer_used > c->content_length)
                {
                    log_error(ERR_CONTENT_LENGTH);
                    return -1;
                }
                memcpy(c->req.body, p, c->buffer_used);
                if (c->buffer_used == c->content_length)
                {
                    c->req.body[c->content_length] = '\0';
                    c->state = CONN_PROCESSING;
                    return 1;
                }

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
        size_t remaining = c->content_length - c->buffer_used;
        if (remaining <= 0)
        {
            c->req.body[c->content_length] = '\0';
            c->state = CONN_PROCESSING;
            return 1;
        }
        size_t to_copy = (size_t)r > remaining ? remaining : (size_t)r;
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
        fprintf(stderr, "Rogue state in request()\n");
        return -1;
    }
}

int response(Connection *c)
{
    switch (c->state)
    {
    case CONN_RESPONSE_FLATTEN:
        c->flattened_response = http_response_flatten(c->res, &c->f_r_len);
        if (!c->flattened_response)
            return -1;
        c->state = CONN_RESPONSE_SEND;
        c->f_r_sent = 0;
        return 0;
    case CONN_RESPONSE_SEND:
        if (c->f_r_sent >= c->f_r_len)
            return 1;
        ssize_t s = send(
            c->fd,
            c->flattened_response + c->f_r_sent,
            c->f_r_len - c->f_r_sent,
            0);
        if (s <= 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return 0;
            return -1;
        }
        c->f_r_sent += s;
        if (c->f_r_sent < c->f_r_len)
            return 0;
        return 1;
    default:
        fprintf(stderr, "Rogue state in response()\n");
        return -1;
    }
    return -1;
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
    pthread_mutex_init(&c->mutex, NULL);
    c->done = 0;
    c->fd = -1;
    c->state = CONN_READING_HEADERS;
    c->buffer[0] = '\0';
    c->buffer_used = 0;
    c->content_length = 0;
    parsed_request_init(&c->req);
    http_response_init(&c->res);
    c->f_r_len = 0;
    c->f_r_sent = 0;
    c->flattened_response = NULL;
}

void connection_free(Connection *c)
{
    pthread_mutex_destroy(&c->mutex);
    parsed_request_free(&c->req);
    http_response_free(&c->res);
    if (c->flattened_response)
        free(c->flattened_response);
}

void connection_reset(Connection *c)
{
    connection_free(c);
    connection_init(c);
}

void drop_connection(struct pollfd *pfd, Connection *c)
{
    connection_reset(c);
    close(pfd->fd);
    pfd->fd = -1;
    pfd->events = 0;
}

void process_request(void *arg)
{
    Connection *c = (Connection *)arg;
    router(c);

    pthread_mutex_lock(&c->mutex);
    c->state = CONN_RESPONSE_FLATTEN;
    c->done = 1;
    pthread_mutex_unlock(&c->mutex);

    uint64_t signal_value = 1;
    if (write(worker_done_fd, &signal_value, sizeof(signal_value)) < 0)
    {
        perror("eventfd write failed");
    }
}

int main()
{
    int port = get_port();
    int socket_fd;
    int opt = 1;
    struct sockaddr_in server_addr;

    signal(SIGPIPE, SIG_IGN);

    // setup listening socket
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed");
        return 1;
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
        close(socket_fd);
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        close(socket_fd);
        return 1;
    }

    if (listen(socket_fd, 3) < 0)
    {
        perror("listen failed");
        close(socket_fd);
        return 1;
    }

    printf("Server listening on %d\n", port);

    worker_done_fd = eventfd(0, EFD_NONBLOCK);
    if (worker_done_fd < 0)
    {
        perror("eventfd failed");
        close(socket_fd);
        return 1;
    }

    // setup thread pool
    tpool_t *thread_pool = tpool_create(NUM_THREADS);
    if (!thread_pool)
    {
        fprintf(stderr, "Thread pool creation failed\n");
        close(worker_done_fd);
        close(socket_fd);
        return 1;
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd;

    struct pollfd fds[MAX_CLIENTS + 2];
    Connection connections[MAX_CLIENTS + 1];
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("flags init failed");
    }
    for (int i = 1; i < MAX_CLIENTS + 1; i++)
    {
        connection_init(&connections[i]);
    }
    for (int i = 0; i < MAX_CLIENTS + 2; i++)
    {
        fds[i].fd = -1;
    }
    fds[0].fd = socket_fd;
    fds[0].events = POLLIN;
    fds[1].fd = worker_done_fd;
    fds[1].events = POLLIN;

    while (1)
    {
        if (poll(fds, MAX_CLIENTS + 2, TIMEOUT) < 0)
        {
            perror("poll failed");
            continue;
        }

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
                close(client_fd);
                continue;
            }
            int assigned = 0;
            for (int i = 2; i < MAX_CLIENTS + 2; i++)
            {
                if (fds[i].fd < 0)
                {
                    int conn_idx = i - 1;
                    fds[i].fd = client_fd;
                    fds[i].events = POLLIN;
                    connection_reset(&connections[conn_idx]);
                    connections[conn_idx].fd = client_fd;
                    assigned = 1;
                    break;
                }
            }
            if (!assigned)
                close(client_fd);
            // todo: timeout stale connections?
            // todo: logic for if clients is full? make em wait or at least close the connection
        }

        if (fds[1].revents & POLLIN)
        {
            uint64_t ready_count;
            while (read(worker_done_fd, &ready_count, sizeof(ready_count)) > 0)
            {
            }
        }

        for (int i = 2; i < MAX_CLIENTS + 2; i++)
        {
            int conn_idx = i - 1;
            if (fds[i].fd == -1)
                continue;

            pthread_mutex_lock(&connections[conn_idx].mutex);
            if (connections[conn_idx].done)
            {
                connections[conn_idx].done = 0;
                fds[i].events = POLLOUT;
            }
            pthread_mutex_unlock(&connections[conn_idx].mutex);

            if (fds[i].revents & (POLLHUP | POLLERR | POLLNVAL))
            {
                drop_connection(&fds[i], &connections[conn_idx]);
                continue;
            }
            if (fds[i].revents & POLLIN)
            {
                int req_ret = request(&connections[conn_idx]);
                switch (req_ret)
                {
                case -1:
                    perror("read request failed");
                    drop_connection(&fds[i], &connections[conn_idx]);
                    continue;
                case 0:
                    continue;
                case 1:
                    connections[conn_idx].state = CONN_PROCESSING;
                    tpool_add_work(thread_pool, process_request, &connections[conn_idx]);
                    fds[i].events = 0;
                    continue;
                }
            }
            if (fds[i].revents & POLLOUT)
            {
                int res_ret = response(&connections[conn_idx]);
                switch (res_ret)
                {
                case -1:
                    perror("response failed");
                    drop_connection(&fds[i], &connections[conn_idx]);
                    continue;
                case 0:
                    continue;
                case 1:
                    drop_connection(&fds[i], &connections[conn_idx]);
                    continue;
                }
            }
        }
    }

    for (int i = 1; i < MAX_CLIENTS + 1; i++)
    {
        connection_free(&connections[i]);
    }
    tpool_destroy(thread_pool);
    close(worker_done_fd);
    close(socket_fd);
    return 0;
}
