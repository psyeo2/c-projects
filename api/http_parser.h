#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "types.h"



void log_error(ErrorCode e);

// util

void parsed_request_init(HttpRequest *req);

void parsed_request_print(HttpRequest req);

void parsed_request_free(HttpRequest *req);

// headers

void headers_init(Headers *h);

ErrorCode headers_append(Headers *h, char *line);

int headers_search(Headers *h, char *search_string);

void headers_free(Headers *h);

// parsing

ErrorCode parse_headers(char *buffer, RequestLine *request_line, Headers *headers, size_t *content_length);

#endif