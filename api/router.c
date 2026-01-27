#include "router.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "routers/ping.h"
#include "routers/users.h"

void not_found(HttpResponse *h)
{
    snprintf(h->response_line.version, sizeof(h->response_line.version), "HTTP/1.1");
    h->response_line.status = HTTP_STATUS_NOT_FOUND;
    snprintf(h->response_line.reason, sizeof(h->response_line.reason), "Not Found");

    headers_append(&h->headers, "Content-Type: application/json; charset=utf-8");

    h->body = "{\"error\": \"Not Found\"}\n";
}

void method_not_allowed(HttpResponse *h)
{
    snprintf(h->response_line.version, sizeof(h->response_line.version), "HTTP/1.1");
    h->response_line.status = HTTP_STATUS_METHOD_NOT_ALLOWED;
    snprintf(h->response_line.reason, sizeof(h->response_line.reason), "Method Not Allowed");

    headers_append(&h->headers, "Content-Type: application/json; charset=utf-8");

    h->body = "{\"error\": \"Method Not Allowed\"}\n";
}

void routes_check(Routes r, ParsedRequest req, HttpResponse *res)
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

void http_response_init(HttpResponse *h)
{
    snprintf(h->response_line.version, sizeof(h->response_line.version), "HTTP/1.1");
    h->response_line.status = HTTP_STATUS_INTERNAL_SERVER_ERROR;
    snprintf(h->response_line.reason, sizeof(h->response_line.reason), "HTTP Response Not Modified After Initialisation");

    Headers headers_;
    headers_init(&headers_);

    h->headers = headers_;
    h->body = NULL;
}

void http_response_free(HttpResponse *r)
{
    headers_free(&r->headers);
    // free(r->body);
}

char *http_response_flatten(HttpResponse h, int *len)
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

int path_is(const char *path, const char *prefix)
{
    size_t len = strlen(prefix);
    return strncmp(path, prefix, len) == 0 &&
           (path[len] == '\0' || path[len] == '/');
}

HttpResponse router(ParsedRequest req)
{
    HttpResponse res;
    http_response_init(&res);

    if (path_is(req.request_line.target, "/ping"))
    {
        router_ping(req, &res);
        return res;
    }

    if (path_is(req.request_line.target, "/users"))
    {
        router_users(req, &res);
        return res;
    }

    not_found(&res);
    return res;
}
