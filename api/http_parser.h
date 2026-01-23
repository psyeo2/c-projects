#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "types.h"



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