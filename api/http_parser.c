#include "http_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int get_line_from_string(char **p, char *line)
{
    int i = 0;

    while (**p && **p != '\r')
    {
        line[i++] = **p;
        (*p)++;
    }

    if (**p == '\r')
        (*p) += 2;

    line[i] = '\0';

    return i;
}

void parse_request_line(char *line, RequestLine *request_line)
{
    int i = 0;
    int pos = 0;
    while (line[pos] && line[pos] != ' ')
    {
        request_line->method[i++] = line[pos++];
    }
    request_line->method[i] = '\0';

    i = 0;
    pos++;
    while (line[pos] && line[pos] != ' ')
    {
        request_line->target[i++] = line[pos++];
    }
    request_line->target[i] = '\0';

    i = 0;
    pos++;
    while (line[pos] && line[pos] != ' ')
    {
        request_line->version[i++] = line[pos++];
    }
    request_line->version[i] = '\0';
}

void parse_header(char *line, char *name, char *value)
{
    int i = 0;
    int j = 0;
    while (line[i] && line[i] != ':')
    {
        name[j++] = tolower(line[i++]);
    }
    name[j] = '\0';
    j = 0;
    i += 2;
    while (line[i] && line[i] != '\r')
    {
        value[j++] = line[i++];
    }
    value[j] = '\0';
}

void headers_init(Headers *h)
{
    h->headers = NULL;
    h->count = 0;
    h->max_count = 0;
}

int headers_append(Headers *h, char *line)
{
    char name[256];
    char value[256];
    parse_header(line, name, value);
    if (h->count == h->max_count)
    {
        size_t new_max = h->max_count ? h->max_count * 2 : 8;
        Header *new_headers = realloc(h->headers, new_max * sizeof(Header));
        if (!new_headers)
            return -1;

        h->headers = new_headers;
        h->max_count = new_max;
    }
    h->headers[h->count].name = strdup(name);
    h->headers[h->count].value = strdup(value);
    if (!h->headers[h->count].name || !h->headers[h->count].value)
        return -1;

    h->count++;
    return 0;
}

int headers_search(Headers *h, char *search_string)
{
    for (size_t i = 0; i < h->count; i++)
    {
        if (strcmp(h->headers[i].name, search_string) == 0)
            return i;
        // return h->headers[i].value;
    }
    return -1;
}

void headers_free(Headers *h)
{
    for (size_t i = 0; i < h->count; i++)
    {
        free(h->headers[i].name);
        free(h->headers[i].value);
    }
    free(h->headers);
}

void parsed_http_init(ParsedHttp *p)
{
    Headers h;
    headers_init(&h);

    p->headers = h;
    p->body = NULL;
}

void parsed_http_print(ParsedHttp p)
{
    printf("Method: %s, Target: %s, Version: %s\n",
           p.request_line.method,
           p.request_line.target,
           p.request_line.version);

    for (size_t i = 0; i < p.headers.count; i++)
    {
        printf("Header %ld: {Name: %s, Value: %s}\n", i + 1, p.headers.headers[i].name, p.headers.headers[i].value);
    }

    if (p.body)
    {
        printf("Body: %s\n", p.body);
    }
    else
    {
        printf("No body\n");
    }
}

void parsed_http_free(ParsedHttp *p)
{
    headers_free(&p->headers);
    free(p->body);
}

void parse_body_(Headers *h, long content_length_idx, char **p, char **body)
{
    char *end;
    int content_length = strtol(h->headers[content_length_idx].value, &end, 10);
    *body = malloc(content_length * sizeof(char) + 1);
    int i;
    for (i = 0; i < content_length; i++)
    {
        (*body)[i] = **p;
        (*p)++;
    }
    (*body)[i] = '\0';
}

void parse_request(char *request, ParsedHttp *parsed_http, int buffer_len)
{
    char line[buffer_len];
    char *p = request;

    int content_length_idx = -1;

    RequestLine request_line;
    Headers headers;
    headers_init(&headers);

    if (get_line_from_string(&p, line))
    {
        parse_request_line(line, &request_line);
        printf("Method: %s, Target: %s, Version: %s\n", request_line.method, request_line.target, request_line.version);
    }

    while (get_line_from_string(&p, line))
    {
        headers_append(&headers, line);
    }
    for (size_t i = 0; i < headers.count; i++)
    {
        printf("Header %ld: {Name: %s, Value: %s}\n", i + 1, headers.headers[i].name, headers.headers[i].value);
    }

    if ((content_length_idx = headers_search(&headers, "content-length")) >= 0)
    {
        parse_body_(&headers, content_length_idx, &p, &(parsed_http->body));

        printf("Body: %s\n", parsed_http->body);
    }
}

int parse_headers(char *buffer, RequestLine *request_line, Headers *headers)
{
    headers_init(headers);
    char line[1024];
    char *p = buffer;
    int content_length_idx = -1;
    char *end;

    if (get_line_from_string(&p, line))
    {
        parse_request_line(line, request_line);
    }
    while (get_line_from_string(&p, line))
    {
        headers_append(headers, line);
    }
    if ((content_length_idx = headers_search(headers, "content-length")) >= 0)
    {
        return strtol(headers->headers[content_length_idx].value, &end, 10);
    }
    return 0;
}

void parse_body(char *buffer, int body_idx, int content_length, char **body)
{
    *body = malloc(content_length * sizeof(char) + 1);
    int i;
    for (i = body_idx; i < body_idx + content_length; i++)
    {
        (*body)[i - body_idx] = buffer[i];
    }
    (*body)[i] = '\0';
}