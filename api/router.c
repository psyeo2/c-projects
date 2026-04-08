#include "router.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "routers/ping.h"
#include "routers/users.h"

/* Set the HTTP status line fields on a response. */
void http_response_set_status(HttpResponse *h, HttpStatusCode status, const char *reason)
{
    snprintf(h->response_line.version, sizeof(h->response_line.version), "HTTP/1.1");
    h->response_line.status = status;
    snprintf(h->response_line.reason, sizeof(h->response_line.reason), "%s", reason);
}

/* Replace the response body with an owned heap copy of the provided string. */
int http_response_set_body(HttpResponse *r, const char *body)
{
    char *body_copy = NULL;

    if (body != NULL)
    {
        size_t body_len = strlen(body);
        body_copy = malloc(body_len + 1);
        if (body_copy == NULL)
            return -1;
        memcpy(body_copy, body, body_len + 1);
    }

    if (r->body != NULL)
        free(r->body);

    r->body = body_copy;
    return 0;
}

/* Set a JSON response with explicit status and a body owned by the response. */
int http_response_set_json(HttpResponse *r, HttpStatusCode status, const char *reason, const char *body)
{
    http_response_set_status(r, status, reason);

    if (headers_append(&r->headers, "Content-Type: application/json; charset=utf-8") != OK)
        goto fail;

    if (http_response_set_body(r, body) != 0)
        goto fail;

    return 0;

fail:
    headers_free(&r->headers);
    headers_init(&r->headers);
    if (r->body != NULL)
    {
        free(r->body);
        r->body = NULL;
    }
    http_response_set_status(r, HTTP_STATUS_INTERNAL_SERVER_ERROR, "Internal Server Error");
    return -1;
}

/* Populate a standard JSON 404 response. */
void not_found(HttpResponse *h)
{
    http_response_set_json(h, HTTP_STATUS_NOT_FOUND, "Not Found", "{\"error\": \"Not Found\"}\n");
}

/* Populate a standard JSON 405 response for matched paths with wrong methods. */
void method_not_allowed(HttpResponse *h)
{
    http_response_set_json(h, HTTP_STATUS_METHOD_NOT_ALLOWED, "Method Not Allowed", "{\"error\": \"Method Not Allowed\"}\n");
}

/* Resolve a route table against a request and dispatch the matched handler. */
void routes_check(Routes r, HttpRequest req, HttpResponse *res)
{
    int path_matched = 0;
    char *method = req.request_line.method;
    char *target = req.request_line.target;
    for (int i = 0; i < r.route_count; i++)
    {
        if (strcmp(target, r.routes[i].endpoint) == 0)
        {
            path_matched = 1;
            if (strcmp(method, r.routes[i].method) == 0)
            {
                r.routes[i].handler(req, res);
                return;
            }
        }
    }
    if (path_matched)
    {
        method_not_allowed(res);
    }
    else
    {
        not_found(res);
    }
}

/* Initialise a response object with a known fallback error state. */
void http_response_init(HttpResponse *h)
{
    http_response_set_status(h, HTTP_STATUS_INTERNAL_SERVER_ERROR, "HTTP Response Not Modified After Initialisation");

    Headers headers_;
    headers_init(&headers_);

    h->headers = headers_;
    h->body = NULL;
}

/* Release heap-owned response fields. */
void http_response_free(HttpResponse *r)
{
    headers_free(&r->headers);
    if (r->body)
        free(r->body);
    r->body = NULL;
}

/* Serialise an HTTP response struct into a wire-format buffer. */
char *http_response_flatten(HttpResponse h, size_t *len)
{
    char *flattened_response;

    char status_str[8];
    snprintf(status_str, sizeof(status_str), "%d", h.response_line.status);

    int body_len = h.body ? strlen(h.body) : 0;
    char body_len_str[16];
    snprintf(body_len_str, sizeof body_len_str, "%d", body_len);

    // calculate flat response string length
    int response_len = 0;

    response_len += strlen(h.response_line.version) + 1;
    response_len += strlen(status_str) + 1;
    response_len += strlen(h.response_line.reason) + 2;

    for (int i = 0; i < (int)h.headers.count; i++)
    {
        response_len += strlen(h.headers.headers[i].name);
        response_len += 2; // ": "
        response_len += strlen(h.headers.headers[i].value);
        response_len += 2; // "\r\n"
    }

    response_len += strlen("Content-Length: ");
    response_len += strlen(body_len_str) + 4; // "\r\n"
    response_len += body_len;

    response_len++; // '\0'

    // allocate
    flattened_response = malloc(response_len * sizeof(char));
    if (!flattened_response)
        return NULL;

    size_t remaining = response_len;
    char *p = flattened_response;

    int n = snprintf(p, remaining, "%s %s %s\r\n", h.response_line.version, status_str, h.response_line.reason);
    p += n;
    remaining -= n;

    for (int i = 0; i < (int)h.headers.count; i++)
    {
        n = snprintf(p, remaining, "%s: %s\r\n", h.headers.headers[i].name, h.headers.headers[i].value);
        p += n;
        remaining -= n;
    }

    n = snprintf(p, remaining, "Content-Length: %s\r\n\r\n", body_len_str);
    p += n;
    remaining -= n;

    if (body_len) {
        memcpy(p, h.body, body_len);
        p += body_len;
    }
    *p = '\0';

    *len = response_len - 1;
    return flattened_response;
}

/* Check whether a request path matches a route prefix boundary. */
int path_is(const char *path, const char *prefix)
{
    size_t len = strlen(prefix);
    return strncmp(path, prefix, len) == 0 &&
           (path[len] == '\0' || path[len] == '/');
}

/* Build the response for a connection by dispatching to the appropriate router. */
void router(Connection *c)
{
    HttpResponse res;
    http_response_init(&res);

    if (path_is(c->req.request_line.target, "/ping"))
    {
        router_ping(c->req, &res);
        c->res = res;
        return;
    }

    if (path_is(c->req.request_line.target, "/users"))
    {
        router_users(c->req, &res);
        c->res = res;
        return;
    }

    not_found(&res);
    c->res = res;
}
