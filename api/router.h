#ifndef ROUTER_H
#define ROUTER_H

#include "types.h"
#include "http_parser.h"

// void routes_init(Routes *r);

// void routes_append(Routes *r, const char *method, const char *endpoint, void (*handler)(HttpRequest, HttpResponse *));

void routes_check(Routes r, HttpRequest req, HttpResponse *res);

// void routes_free(Routes *r);

void http_response_set_status(HttpResponse *h, HttpStatusCode status, const char *reason);

void http_response_init(HttpResponse *h);

void http_response_free(HttpResponse *r);

int http_response_set_body(HttpResponse *r, const char *body);

int http_response_set_text(HttpResponse *r, HttpStatusCode status, const char *reason, const char *body);

int http_response_set_json(HttpResponse *r, HttpStatusCode status, const char *reason, const char *body);

char *http_response_flatten(HttpResponse h, size_t *len);

void router(Connection *c);

#endif
