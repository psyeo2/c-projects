#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stddef.h>

typedef struct
{
    char method[8];
    char target[256];
    char version[16];
} RequestLine;

typedef struct
{
    char *name;
    char *value;
} Header;

typedef struct
{
    Header *headers;
    size_t count;
    size_t max_count;
} Headers;

typedef struct
{
    RequestLine request_line;
    Headers headers;
    char *body;
} ParsedHttp;

typedef struct
{
    char version[16];
    int status;
    char reason[256];
} ResponseLine;

void parse_request(char *request, ParsedHttp *parsed_http, int buffer_len);

#endif