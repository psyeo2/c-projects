#include "ping.h"

#include <stdio.h>
#include <stdlib.h>

void ping(HttpRequest req, HttpResponse *res)
{
    (void)req;
    snprintf(res->response_line.version, sizeof(res->response_line.version), "HTTP/1.1");
    res->response_line.status = HTTP_STATUS_OK;
    snprintf(res->response_line.reason, sizeof(res->response_line.reason), "OK");

    headers_append(&res->headers, "Content-Type: application/json; charset=utf-8");

    http_response_set_body(res, "{\"message\": \"pong\"}\n");
}

void router_ping(HttpRequest req, HttpResponse *res)
{
    static const Route routes[] = {
        {"GET", "/ping", ping}
    };
    static const Routes table = {
        .routes = routes,
        .route_count = sizeof(routes) / sizeof(routes[0]),
    };

    routes_check(table, req, res);
}
