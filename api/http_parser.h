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

void parsed_http_init(ParsedHttp *p);

void parsed_http_print(ParsedHttp p);

void parsed_http_free(ParsedHttp *p);

int parse_headers(char *buffer, RequestLine *request_line, Headers *headers);

void parse_body(char* buffer, int body_idx, int content_length, char **body);

void parse_request(char *request, ParsedHttp *parsed_http, int buffer_len);

#endif