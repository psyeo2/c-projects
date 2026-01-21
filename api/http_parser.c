#include "http_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// void init_parsed_http(ParsedHttp *p)
// {
//     p->request_line = NULL;
//     p->headers = NULL;
//     p->body = NULL;
// }

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

void parse_request(char *request, ParsedHttp *parsed_http, int buffer_len)
{
    char line[buffer_len];
    char *p = request;

    int content_length_idx;
    long content_length;
    char *end;

    RequestLine request_line;
    Headers headers;
    headers_init(&headers);
    char *body;

    if (get_line_from_string(&p, line))
    {
        parse_request_line(line, &request_line);
        printf("Method: %s, Target: %s, Version: %s\n", request_line.method, request_line.target, request_line.version);
    }
    
    while (get_line_from_string(&p, line))
    {
        headers_append(&headers, line);
    }
    for (int i = 0; i < headers.count; i++)
    {
        printf("Header %d: {Name: %s, Value: %s}\n", i + 1, headers.headers[i].name, headers.headers[i].value);
    }

    if ((content_length_idx = headers_search(&headers, "content-length")) >= 0)
    {
        content_length = strtol(headers.headers[content_length_idx].value, &end, 10);
        body = malloc(content_length * sizeof(char) + 1);
        int i;
        for (i = 0; i < content_length; i++)
        {
            body[i] = *p;
            p++;
        }
        body[i] = '\0';

        printf("Body: %s\n", body);
    }
}