#include "ping.h"

#include <stdio.h>
#include <stdlib.h>

void ping(HttpRequest req, HttpResponse *res)
{
    (void)req;
    http_response_set_json(res, HTTP_STATUS_OK, "OK", "{\"message\": \"pong\"}\n");
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
