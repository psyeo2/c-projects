#ifndef PING_H
#define PING_H

#include "../types.h"
#include "../router.h"
#include "../http_parser.h"

void router_ping(ParsedRequest req, HttpResponse *res);

#endif