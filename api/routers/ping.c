#include "ping.h"

#include <string.h>
#include <stdlib.h>

void ping(ParsedRequest req, HttpResponse *res)
{
    strcpy(res->response_line.version, "HTTP/1.1");
    res->response_line.status = HTTP_STATUS_OK;
    strcpy(res->response_line.reason, "OK");

    headers_append(&res->headers, "Content-Type: application/json; charset=utf-8");

    res->body = "{\"message\": \"pong\"}\n";
}

void router_ping(ParsedRequest req, HttpResponse *res)
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