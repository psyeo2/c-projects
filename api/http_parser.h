#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stddef.h>

#define MAX_BODY_LENGTH 1048576

typedef enum
{
    OK = 0,
    ERR_BAD_REQUEST_LINE,
    ERR_METHOD_LENGTH,
    ERR_TARGET_LENGTH,
    ERR_VERSION_LENGTH,
    ERR_HEADER_NAME_LENGTH,
    ERR_HEADER_VALUE_LENGTH,
    ERR_BAD_HEADER,
    ERR_HEADERS_REALLOC_FAIL,
    ERR_HEADERS_ASSIGN_FAIL,
    ERR_CONTENT_LENGTH,
    ERR_PARSE_BODY_MALLOC_FAILED,
    ERR_LINE_OOB,
} ErrorCode;

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
} ParsedRequest;

void log_error(ErrorCode e);

// util

void parsed_request_init(ParsedRequest *p);

void parsed_request_print(ParsedRequest p);

void parsed_request_free(ParsedRequest *p);

// headers

void headers_init(Headers *h);

ErrorCode headers_append(Headers *h, char *line);

int headers_search(Headers *h, char *search_string);

void headers_free(Headers *h);

// parsing

ErrorCode parse_request(char *request, ParsedRequest *parsed_request, int buffer_len);

ErrorCode parse_headers(char *buffer, RequestLine *request_line, Headers *headers, int *content_length);

ErrorCode parse_body(char* buffer, int body_idx, int content_length, char **body);

#endif