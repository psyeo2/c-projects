# HTTP API Server - Complete Documentation

## Overview

This is a from-scratch HTTP/1.1 API server written in C that handles HTTP requests using raw TCP sockets. The server implements a complete HTTP parser, routing system, and response generator without relying on external HTTP libraries. It uses multi-threading (via pthreads) to handle concurrent client connections and supports JSON responses.

**Key Features:**
- Raw TCP socket implementation with multi-threaded request handling
- Complete HTTP/1.1 request parser (headers, body, request line)
- Dynamic routing system with method and endpoint matching
- JSON response formatting using yyjson library
- Comprehensive HTTP status code support
- Request body parsing with configurable size limits (up to 1MB by default)
- Environment-based port configuration

---

## Architecture

### Core Components

1. **socket.c** - TCP server and connection handling
2. **http_parser.c/h** - HTTP request parsing
3. **router.c/h** - Request routing and response generation
4. **types.h** - Data structures and type definitions
5. **routers/** - Individual route handlers (ping.c, users.c)

### Data Flow

```
Client Request → TCP Socket → Multi-threaded Handler →
HTTP Parser → Router → Route Handler → HTTP Response →
Flatten Response → Send to Client
```

---

## Core Data Structures

### Request Parsing Structures

```c
typedef struct {
    char method[8];      // GET, POST, etc.
    char target[256];    // /ping, /users, etc.
    char version[16];    // HTTP/1.1
} RequestLine;

typedef struct {
    char *name;
    char *value;
} Header;

typedef struct {
    Header *headers;
    size_t count;
    size_t max_count;
} Headers;

typedef struct {
    RequestLine request_line;
    Headers headers;
    char *body;
} HttpRequest;
```

### Response Structures

```c
typedef struct {
    char version[16];
    int status;
    char reason[256];
} ResponseLine;

typedef struct {
    ResponseLine response_line;
    Headers headers;
    char *body;
} HttpResponse;
```

### Routing Structures

```c
typedef struct {
    char method[8];
    char endpoint[256];
    void (*handler)(HttpRequest, HttpResponse *);
} Route;

typedef struct {
    const Route *routes;
    const int route_count;
} Routes;
```

---

## Socket Server (socket.c)

### Server Initialization

The server starts by:
1. Reading PORT from environment variable (default: 3500)
2. Creating a TCP socket with `AF_INET` and `SOCK_STREAM`
3. Setting `SO_REUSEADDR` to allow port reuse
4. Binding to `INADDR_ANY` (all interfaces)
5. Listening for connections (backlog of 3)
6. Ignoring `SIGPIPE` signals to prevent crashes on broken connections

```c
int get_port() {
    const char *env = getenv("PORT");
    if (!env) {
        fprintf(stderr, "Missing PORT environment variable, falling back to 3500.\n");
        return 3500;
    }
    char *end;
    int port = (int)strtol(env, &end, 10);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Port out of range, falling back to 3500.\n");
        return 3500;
    }
    return port;
}
```

### Multi-threaded Connection Handling

Each client connection spawns a new detached pthread:

```c
while (1) {
    client_addr_len = sizeof(client_addr);
    
    if ((client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
        err = errno;
        fprintf(stderr, "accept failed: %s\n", strerror(err));
        continue;
    }
    
    client_context = malloc(sizeof(ClientContext));
    if (!client_context) {
        close(client_fd);
        continue;
    }
    
    client_context->client_fd = client_fd;
    client_context->client_addr = client_addr;
    
    pthread_t thread_id;
    if ((pthread_create(&thread_id, NULL, handle_client, (void *)client_context)) != 0) {
        err = errno;
        fprintf(stderr, "pthread_create failed: %s\n", strerror(err));
        free(client_context);
        close(client_fd);
        continue;
    }
    pthread_detach(thread_id);
}
```

### Request Reception Strategy

The `handle_client` function implements a sophisticated two-phase reception strategy:

#### Phase 1: Read Until Headers Complete

```c
ssize_t used = 0;
ssize_t r = 0;
char tmp[BUFFER_LEN];
char *out = NULL;
char *needle_loc;

while (1) {
    if (used > MAX_BODY_LENGTH) {
        error_code = ERR_CONTENT_LENGTH;
        log_error(error_code);
        // cleanup and return
    }
    r = recv(client_fd, tmp, BUFFER_LEN, 0);
    if (r <= 0) {
        perror("recv failed");
        // cleanup and return
    }
    used += r;
    char *tmp_out = realloc(out, used * sizeof(char));
    if (!tmp_out) {
        fprintf(stderr, "realloc err\n");
        // cleanup and return
    }
    out = tmp_out;
    memcpy(out + used - r, tmp, r * sizeof(char));
    
    // Look for end of headers marker
    if ((needle_loc = (char *)memmem_simple(out, used, "\r\n\r\n", 4))) {
        break;
    }
}
```

The server uses a custom `memmem_simple` implementation to search for `\r\n\r\n` (the HTTP headers terminator) in the received data buffer.

#### Phase 2: Read Body if Content-Length Present

After parsing headers, if a `Content-Length` header exists:

```c
int content_length = 0;
error_code = parse_headers(out, &parsed_request.request_line, 
                          &parsed_request.headers, &content_length);

if (content_length) {
    int body_idx = needle_loc - out + 4;  // Skip past \r\n\r\n
    int body_recieved = used - body_idx;
    int remaining = content_length - body_recieved;
    
    if (remaining > 0) {
        // Reallocate buffer to fit complete body
        char *tmp_out = realloc(out, (used + remaining) * sizeof(char));
        out = tmp_out;
        
        r = 0;
        while (r < remaining) {
            ssize_t want = remaining - r;
            ssize_t chunk = want > BUFFER_LEN ? BUFFER_LEN : want;
            
            ssize_t n = recv(client_fd, tmp, chunk, 0);
            if (n <= 0) {
                perror("recv failed");
                // cleanup
            }
            memcpy(out + used + r, tmp, n * sizeof(char));
            r += n;
        }
        used += remaining;
    }
    error_code = parse_body(out, body_idx, content_length, &parsed_request.body);
}
```

This ensures the complete request body is received before processing.

---

## HTTP Parser (http_parser.c)

### Request Line Parsing

The request line parser extracts method, target (endpoint), and HTTP version:

```c
ErrorCode parse_request_line(char *line, RequestLine *request_line) {
    size_t i = 0;
    size_t pos = 0;
    
    // Parse method (e.g., "GET")
    while (line[pos] && line[pos] != ' ') {
        if (i > 6) return ERR_METHOD_LENGTH;
        request_line->method[i++] = line[pos++];
    }
    request_line->method[i] = '\0';
    
    if (line[pos] != ' ') return ERR_BAD_REQUEST_LINE;
    
    // Parse target (e.g., "/ping")
    i = 0;
    pos++;
    while (line[pos] && line[pos] != ' ') {
        if (i > 254) return ERR_TARGET_LENGTH;
        request_line->target[i++] = line[pos++];
    }
    request_line->target[i] = '\0';
    
    if (line[pos] != ' ') return ERR_BAD_REQUEST_LINE;
    
    // Parse version (e.g., "HTTP/1.1")
    i = 0;
    pos++;
    while (line[pos] && line[pos] != ' ') {
        if (i > 14) return ERR_VERSION_LENGTH;
        request_line->version[i++] = line[pos++];
    }
    request_line->version[i] = '\0';
    
    if (line[pos] != '\0') return ERR_BAD_REQUEST_LINE;
    
    return OK;
}
```

### Header Parsing

Headers are parsed one line at a time, with names converted to lowercase and stored dynamically:

```c
ErrorCode parse_header(char *line, char *name, char *value) {
    int i = 0;
    int j = 0;
    
    // Parse header name (converted to lowercase)
    while (line[i] && line[i] != ':') {
        if (j > 254) return ERR_HEADER_NAME_LENGTH;
        name[j++] = tolower((unsigned char)line[i++]);
    }
    name[j] = '\0';
    j = 0;
    
    // Expect ": " after header name
    if (line[++i] != ' ') return ERR_BAD_HEADER;
    i++;
    
    // Parse header value
    while (line[i] && line[i] != '\r') {
        if (j > 254) return ERR_HEADER_VALUE_LENGTH;
        value[j++] = line[i++];
    }
    value[j] = '\0';
    return OK;
}
```

### Dynamic Headers Storage

Headers are stored in a dynamically growing array:

```c
ErrorCode headers_append(Headers *h, char *line) {
    char name[256];
    char value[256];
    ErrorCode error_code = OK;
    
    if ((error_code = parse_header(line, name, value)))
        return error_code;
        
    // Grow array if needed (doubling strategy)
    if (h->count == h->max_count) {
        size_t new_max = h->max_count ? h->max_count * 2 : 8;
        Header *new_headers = realloc(h->headers, new_max * sizeof(Header));
        if (!new_headers) return ERR_HEADERS_REALLOC_FAIL;
        
        h->headers = new_headers;
        h->max_count = new_max;
    }
    
    // Duplicate strings for storage
    h->headers[h->count].name = strdup(name);
    h->headers[h->count].value = strdup(value);
    if (!h->headers[h->count].name || !h->headers[h->count].value) {
        free(h->headers[h->count].name);
        free(h->headers[h->count].value);
        return ERR_HEADERS_ASSIGN_FAIL;
    }
    h->count++;
    return OK;
}
```

### Complete Header Parsing Flow

```c
ErrorCode parse_headers(char *buffer, RequestLine *request_line, 
                       Headers *headers, int *content_length) {
    headers_init(headers);
    int buffer_len = 1024;
    char line[buffer_len];
    char *p = buffer;
    int content_length_idx = -1;
    char *end;
    ErrorCode error_code = OK;
    int line_chars = 0;
    
    // Parse request line
    error_code = get_line_from_string(&p, line, buffer_len, &line_chars);
    if (error_code) return error_code;
    if (line_chars) {
        error_code = parse_request_line(line, request_line);
        if (error_code) return error_code;
    }
    
    // Parse all headers
    line_chars = 0;
    error_code = get_line_from_string(&p, line, buffer_len, &line_chars);
    if (error_code) return error_code;
    while (line_chars) {
        error_code = headers_append(headers, line);
        if (error_code) return error_code;
        
        error_code = get_line_from_string(&p, line, buffer_len, &line_chars);
        if (error_code) return error_code;
    }
    
    // Extract Content-Length if present
    if ((content_length_idx = headers_search(headers, "content-length")) >= 0) {
        *content_length = strtol(headers->headers[content_length_idx].value, &end, 10);
        if (*content_length > MAX_BODY_LENGTH || *content_length < 0)
            error_code = ERR_CONTENT_LENGTH;
    }
    return error_code;
}
```

### Body Parsing

```c
ErrorCode parse_body(char *buffer, int body_idx, int content_length, char **body) {
    ErrorCode error_code = OK;
    if (content_length > MAX_BODY_LENGTH || content_length < 0)
        return ERR_CONTENT_LENGTH;
        
    *body = malloc((content_length + 1) * sizeof(char));
    if (!*body) return ERR_PARSE_BODY_MALLOC_FAILED;
    
    int i;
    for (i = body_idx; i < body_idx + content_length; i++) {
        (*body)[i - body_idx] = buffer[i];
    }
    (*body)[content_length] = '\0';
    return error_code;
}
```

---

## Router System (router.c)

### Main Router Function

The router uses a path-prefix matching system:

```c
HttpResponse router(HttpRequest req) {
    HttpResponse res;
    http_response_init(&res);
    
    if (path_is(req.request_line.target, "/ping")) {
        router_ping(req, &res);
        return res;
    }
    
    if (path_is(req.request_line.target, "/users")) {
        router_users(req, &res);
        return res;
    }
    
    not_found(&res);
    return res;
}

int path_is(const char *path, const char *prefix) {
    size_t len = strlen(prefix);
    return strncmp(path, prefix, len) == 0 &&
           (path[len] == '\0' || path[len] == '/');
}
```

This allows matching both `/ping` and `/ping/whatever`.

### Sub-Router Pattern

Each endpoint has its own sub-router that handles method-specific routing:

```c
void router_ping(HttpRequest req, HttpResponse *res) {
    static const Route routes[] = {
        {"GET", "/ping", ping}
    };
    static const Routes table = {
        .routes = routes,
        .route_count = sizeof(routes) / sizeof(routes[0]),
    };
    
    routes_check(table, req, res);
}
```

### Route Checking Logic

```c
void routes_check(Routes r, HttpRequest req, HttpResponse *res) {
    int path_matched = 0;
    char *method = req.request_line.method;
    char *target = req.request_line.target;
    
    for (int i = 0; i < r.route_count; i++) {
        if (strcmp(target, r.routes[i].endpoint) == 0) {
            path_matched = 1;
            if (strcmp(method, r.routes[i].method) == 0) {
                r.routes[i].handler(req, res);
                return;
            }
        }
    }
    
    if (path_matched) {
        method_not_allowed(res);  // 405
    } else {
        not_found(res);  // 404
    }
}
```

This properly distinguishes between:
- **404 Not Found** - No route matches the path
- **405 Method Not Allowed** - Path exists but method doesn't match

### Response Flattening

Responses are converted to raw HTTP strings before sending:

```c
char *http_response_flatten(HttpResponse h, int *len) {
    char *flattened_response;
    
    char status_str[8];
    snprintf(status_str, sizeof(status_str), "%d", h.response_line.status);
    
    int body_len = h.body ? strlen(h.body) : 0;
    char body_len_str[16];
    snprintf(body_len_str, sizeof body_len_str, "%d", body_len);
    
    // Calculate total response length
    int response_len = 0;
    response_len += strlen(h.response_line.version) + 1;
    response_len += strlen(status_str) + 1;
    response_len += strlen(h.response_line.reason) + 2;
    
    for (int i = 0; i < (int)h.headers.count; i++) {
        response_len += strlen(h.headers.headers[i].name);
        response_len += strlen(": ");
        response_len += strlen(h.headers.headers[i].value);
        response_len += strlen("\r\n");
    }
    
    response_len += strlen("Content-Length: ");
    response_len += strlen(body_len_str) + 4;
    response_len += body_len;
    
    // Allocate and construct
    flattened_response = malloc((response_len + 1) * sizeof(char));
    if (!flattened_response) return NULL;
    flattened_response[0] = '\0';
    
    // Build response string
    strcat(flattened_response, h.response_line.version);
    strcat(flattened_response, " ");
    strcat(flattened_response, status_str);
    strcat(flattened_response, " ");
    strcat(flattened_response, h.response_line.reason);
    strcat(flattened_response, "\r\n");
    
    for (int i = 0; i < (int)h.headers.count; i++) {
        strcat(flattened_response, h.headers.headers[i].name);
        strcat(flattened_response, ": ");
        strcat(flattened_response, h.headers.headers[i].value);
        strcat(flattened_response, "\r\n");
    }
    
    strcat(flattened_response, "Content-Length: ");
    strcat(flattened_response, body_len_str);
    strcat(flattened_response, "\r\n\r\n");
    if (body_len) {
        strcat(flattened_response, h.body);
    }
    
    *len = response_len;
    return flattened_response;
}
```

Output format:
```
HTTP/1.1 200 OK
Content-Type: application/json; charset=utf-8
Content-Length: 23

{"message": "pong"}
```

---

## Route Handlers

### Example: Ping Handler

```c
void ping(HttpRequest req, HttpResponse *res) {
    strcpy(res->response_line.version, "HTTP/1.1");
    res->response_line.status = HTTP_STATUS_OK;
    strcpy(res->response_line.reason, "OK");
    
    headers_append(&res->headers, "Content-Type: application/json; charset=utf-8");
    
    res->body = "{\"message\": \"pong\"}\n";
}
```

### Error Handlers

```c
void not_found(HttpResponse *h) {
    strcpy(h->response_line.version, "HTTP/1.1");
    h->response_line.status = HTTP_STATUS_NOT_FOUND;
    strcpy(h->response_line.reason, "Not Found");
    
    headers_append(&h->headers, "Content-Type: application/json; charset=utf-8");
    
    h->body = "{\"error\": \"Not Found\"}\n";
}

void method_not_allowed(HttpResponse *h) {
    strcpy(h->response_line.version, "HTTP/1.1");
    h->response_line.status = HTTP_STATUS_METHOD_NOT_ALLOWED;
    strcpy(h->response_line.reason, "Method Not Allowed");
    
    headers_append(&h->headers, "Content-Type: application/json; charset=utf-8");
    
    h->body = "{\"error\": \"Method Not Allowed\"}\n";
}
```

---

## Error Handling

### Error Codes

Defined in `types.h`:

```c
typedef enum {
    OK = 0,
    ERR_BAD_REQUEST_LINE,
    ERR_METHOD_LENGTH,
    ERR_TARGET_LENGTH,
    ERR_VERSION_LENGTH,
    ERR_HEADER_NAME_LENGTH,
    ERR_HEADER_VALUE_LENGTH,
    ERR_BAD_HEADER,
    ERR_HEADERS_REALLOC_FAIL,
    ERR_HEADERS_ASSIGN_FAIL,
    ERR_CONTENT_LENGTH,
    ERR_PARSE_BODY_MALLOC_FAILED,
    ERR_LINE_OOB,
} ErrorCode;
```

### Error Logging

```c
void log_error(ErrorCode e) {
    switch (e) {
        case ERR_BAD_REQUEST_LINE:
            fprintf(stderr, "Request line was malformed!\n");
            break;
        case ERR_CONTENT_LENGTH:
            fprintf(stderr, "Request exceeded %d bytes or could not be parsed\n", 
                   MAX_BODY_LENGTH);
            break;
        // ... more cases
    }
}
```

Errors cause immediate connection termination with proper cleanup of allocated resources.

---

## HTTP Status Codes

The server includes comprehensive HTTP status code support (from `types.h`):

- **1xx Informational**: 100-103
- **2xx Success**: 200-226
- **3xx Redirection**: 300-308
- **4xx Client Error**: 400-451
- **5xx Server Error**: 500-511

Example status codes:
```c
HTTP_STATUS_OK = 200
HTTP_STATUS_CREATED = 201
HTTP_STATUS_NOT_FOUND = 404
HTTP_STATUS_METHOD_NOT_ALLOWED = 405
HTTP_STATUS_INTERNAL_SERVER_ERROR = 500
HTTP_STATUS_IM_A_TEAPOT = 418  // RFC 2324
```

---

## Configuration

### Constants (types.h)

```c
#define BUFFER_LEN 1024          // Read buffer size
#define MAX_BODY_LENGTH 1048576  // 1MB max request body
```

### Environment Variables

- **PORT**: Server port (default: 3500)

---

## Memory Management

The server carefully manages memory throughout the request lifecycle:

1. **Request buffer**: Dynamically grown with `realloc` as data arrives
2. **Headers**: Dynamically allocated array with doubling growth strategy
3. **Header strings**: Individual `strdup` allocations for names and values
4. **Body**: Separate allocation based on Content-Length
5. **Response**: Flattened string allocation

### Cleanup Pattern

Every code path that exits `handle_client` includes:

```c
parsed_request_free(&parsed_request);
http_response_free(&http_response);
free(out);
free(flattened_response);
close(client_fd);
free(arg);  // ClientContext
```

---

## Adding New Routes

To add a new route:

1. Create handler files in `routers/`:
```c
// routers/myroute.h
#ifndef MYROUTE_H
#define MYROUTE_H
#include "../types.h"
#include "../router.h"
void router_myroute(HttpRequest req, HttpResponse *res);
#endif
```

2. Implement the handler:
```c
// routers/myroute.c
#include "myroute.h"

void get_myroute(HttpRequest req, HttpResponse *res) {
    strcpy(res->response_line.version, "HTTP/1.1");
    res->response_line.status = HTTP_STATUS_OK;
    strcpy(res->response_line.reason, "OK");
    headers_append(&res->headers, "Content-Type: application/json; charset=utf-8");
    res->body = "{\"data\": \"value\"}\n";
}

void router_myroute(HttpRequest req, HttpResponse *res) {
    static const Route routes[] = {
        {"GET", "/myroute", get_myroute}
    };
    static const Routes table = {
        .routes = routes,
        .route_count = sizeof(routes) / sizeof(routes[0]),
    };
    routes_check(table, req, res);
}
```

3. Add to main router in `router.c`:
```c
#include "routers/myroute.h"

HttpResponse router(HttpRequest req) {
    HttpResponse res;
    http_response_init(&res);
    
    if (path_is(req.request_line.target, "/myroute")) {
        router_myroute(req, &res);
        return res;
    }
    
    // ... other routes
}
```

---

## External Dependencies

- **yyjson.h/yyjson.c**: High-performance JSON library for parsing and generation
- **pthread**: POSIX threads for concurrent client handling
- Standard C library (stdio, stdlib, string, etc.)
- POSIX socket APIs (sys/socket.h, netinet/in.h, arpa/inet.h)

---

## Building and Running

Compilation likely requires:
```bash
gcc -o server socket.c http_parser.c router.c http_response_codes.c \
    routers/*.c yyjson.c -lpthread
```

Running:
```bash
PORT=8080 ./server
# or
export PORT=3500
./server
```

Testing:
```bash
curl http://localhost:3500/ping
# {"message": "pong"}
```

---

## Security Considerations

1. **Request Size Limits**: MAX_BODY_LENGTH prevents memory exhaustion
2. **Header Size Limits**: Individual headers limited to 256 chars
3. **SIGPIPE Handling**: Prevents crashes from broken connections
4. **Input Validation**: All parsing returns error codes
5. **Resource Cleanup**: Proper cleanup on all error paths

**Known Limitations**:
- No request timeout implementation
- No rate limiting
// - String operations use unsafe functions (`strcat`, `strcpy`) - should use safer variants
- No SSL/TLS support
- Fixed-size buffers for request line components

---

## Performance Characteristics

- **Threading Model**: One thread per connection (not ideal for high concurrency)
- **Parsing**: Linear scan through headers (O(n) per header)
- **Memory**: Dynamic allocation minimizes waste but adds overhead
- **Buffer Strategy**: Incremental realloc reduces large upfront allocations

**Potential Improvements**:
- Thread pool instead of per-connection threads
- Connection keep-alive support
- More efficient buffer management
- Zero-copy parsing where possible

---

## Summary

This HTTP server demonstrates a complete implementation of HTTP/1.1 request/response handling from first principles. It showcases:

- Low-level socket programming
- HTTP protocol parsing
- Multi-threaded server architecture
- Dynamic memory management
- Modular routing system
- Proper error handling and resource cleanup

The codebase is well-structured for learning HTTP internals and could serve as a foundation for more advanced features like WebSocket support, chunked transfer encoding, or HTTP/2.
