#ifndef USERS_H
#define USERS_H

#include "../router.h"
#include "../http_parser.h"

void router_users(ParsedRequest req, HttpResponse *res);

#endif