#include "ping.h"

#include <string.h>
#include <stdlib.h>

typedef struct
{
    char method[8];
    char endpoint[256];
    void (*handler)(ParsedRequest, HttpResponse *);
} Route;

typedef struct
{
    Route *routes;
    int route_count;
    int max_count;
} Routes;

void routes_init(Routes *r)
{
    r->routes = NULL;
    r->route_count = 0;
    r->max_count = 0;
}

void routes_append(Routes *r, const char *method, const char *endpoint, void (*handler)(ParsedRequest, HttpResponse *))
{
    Route route;
    strncpy(route.method, method, sizeof(route.method) - 1);
    route.method[sizeof(route.method) - 1] = '\0';

    strncpy(route.endpoint, endpoint, sizeof(route.endpoint) - 1);
    route.endpoint[sizeof(route.endpoint) - 1] = '\0';

    route.handler = handler;

    if (r->route_count == r->max_count)
    {
        size_t new_max = r->max_count ? r->max_count * 2 : 8;
        Route *new_routes = realloc(r->routes, new_max * sizeof(Route));
        if (!new_routes)
            return;

        r->routes = new_routes;
        r->max_count = new_max;
    }

    r->routes[r->route_count] = route;
    // if (!r->routes[r->route_count])
    //     return;
    r->route_count++;
}

void routes_check(Routes r, ParsedRequest req, HttpResponse *res)
{
    char *method = req.request_line.method;
    char *target = req.request_line.target;
    for (int i = 0; i < r.route_count; i++)
    {
        if (strcmp(target, r.routes[i].endpoint) == 0 && strcmp(method, r.routes[i].method) == 0)
        {
            r.routes[i].handler(req, res);
            return;
        }
    }
}

void routes_free(Routes *r)
{
    free(r->routes);
}

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

    // routes_free(&routes);
}