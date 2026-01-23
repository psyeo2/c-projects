#include "router.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "routers/ping.h"
#include "routers/users.h"

void not_found(HttpResponse *h)
{
    strcpy(h->response_line.version, "HTTP/1.1");
    h->response_line.status = HTTP_STATUS_NOT_FOUND;
    strcpy(h->response_line.reason, "Not Found");

    headers_append(&h->headers, "Content-Type: application/json; charset=utf-8");

    h->body = "{\"error\": \"Not Found\"}\n";
}

void method_not_allowed(HttpResponse *h)
{
    strcpy(h->response_line.version, "HTTP/1.1");
    h->response_line.status = HTTP_STATUS_METHOD_NOT_ALLOWED;
    strcpy(h->response_line.reason, "Method Not Allowed");

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
    strcpy(h->response_line.version, "HTTP/1.1");
    h->response_line.status = HTTP_STATUS_INTERNAL_SERVER_ERROR;
    strcpy(h->response_line.reason, "HTTP Response Not Modified After Initialisation");

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
        response_len += strlen(": ");
        response_len += strlen(h.headers.headers[i].value);
        response_len += strlen("\r\n");
    }

    response_len += strlen("Content-Length: ");
    response_len += strlen(body_len_str) + 4;
    response_len += body_len;

    // allocate
    flattened_response = malloc((response_len + 1) * sizeof(char));
    if (!flattened_response)
        return NULL;
    flattened_response[0] = '\0';

    // construct flat response
    strcat(flattened_response, h.response_line.version);
    strcat(flattened_response, " ");
    strcat(flattened_response, status_str);
    strcat(flattened_response, " ");
    strcat(flattened_response, h.response_line.reason);
    strcat(flattened_response, "\r\n");

    for (int i = 0; i < (int)h.headers.count; i++)
    {
        strcat(flattened_response, h.headers.headers[i].name);
        strcat(flattened_response, ": ");
        strcat(flattened_response, h.headers.headers[i].value);
        strcat(flattened_response, "\r\n");
    }

    strcat(flattened_response, "Content-Length: ");
    strcat(flattened_response, body_len_str);
    strcat(flattened_response, "\r\n\r\n");
    if (body_len)
    {
        strcat(flattened_response, h.body);
    }

    *len = response_len;
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
