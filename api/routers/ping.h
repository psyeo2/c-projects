#ifndef PING_H
#define PING_H

#include "../router.h"
#include "../http_parser.h"

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

void router_ping(ParsedRequest req, HttpResponse *res);

#endif