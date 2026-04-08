#ifndef ROUTER_H
#define ROUTER_H

#include "types.h"
#include "http_parser.h"

// void routes_init(Routes *r);

// void routes_append(Routes *r, const char *method, const char *endpoint, void (*handler)(HttpRequest, HttpResponse *));

void routes_check(Routes r, HttpRequest req, HttpResponse *res);

// void routes_free(Routes *r);

void http_response_init(HttpResponse *h);

void http_response_free(HttpResponse *r);

char *http_response_flatten(HttpResponse h, size_t *len);

void router(Connection *c);

#endif