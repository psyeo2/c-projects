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
    static Routes routes;
    static int init = 0;
    if (!init)
    {
        routes_init(&routes);

        routes_append(&routes, "GET", "/ping", ping);
        // routes_append(&routes, "GET", "/ping", ping);
        // routes_append(&routes, "GET", "/ping", ping);
        
        init = 1;
    }

    routes_check(routes, req, res);
}