#include "http_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void log_error(ErrorCode e)
{
    switch (e)
    {
    case OK:
        fprintf(stderr, "No error! (How did you get here?)\n");
        break;
    case ERR_BAD_REQUEST_LINE:
        fprintf(stderr, "Request line was malformed!\n");
        break;
    case ERR_METHOD_LENGTH:
        fprintf(stderr, "Method was too long.\n");
        break;
    case ERR_TARGET_LENGTH:
        fprintf(stderr, "Endpoint exceeded 256 chars.\n");
        break;
    case ERR_VERSION_LENGTH:
        fprintf(stderr, "Was your version HTTP/1.1?\n");
        break;
    case ERR_HEADER_NAME_LENGTH:
        fprintf(stderr, "Header name exceeded 256 chars.\n");
        break;
    case ERR_HEADER_VALUE_LENGTH:
        fprintf(stderr, "Header value exceeded 256 chars.\n");
        break;
    case ERR_BAD_HEADER:
        fprintf(stderr, "A header was malformed!\n");
    case ERR_HEADERS_REALLOC_FAIL:
        fprintf(stderr, "realloc failed, blame C.\n");
        break;
    case ERR_HEADERS_ASSIGN_FAIL:
        fprintf(stderr, "Header name or value assignment failed, blame C.\n");
        break;
    case ERR_CONTENT_LENGTH:
        fprintf(stderr, "content-length exceeded %d or could not be parsed\n", MAX_BODY_LENGTH);
        break;
    case ERR_PARSE_BODY_MALLOC_FAILED:
        fprintf(stderr, "malloc failed, blame C.\n");
        break;
    case ERR_LINE_OOB:
        fprintf(stderr, "A single header exceeded 1024 chars.\n");
        break;
    default:
        fprintf(stderr, "Something went wrong... (Very wrong!)\n");
    }
}

int get_line_from_string(char **p, char *line, int buffer_len, int *i)
{
    *i = 0;

    while (**p && **p != '\r')
    {
        if (*i >= buffer_len)
            return ERR_LINE_OOB;
        line[(*i)++] = **p;
        (*p)++;
    }

    if (**p == '\r')
        (*p) += 2;

    line[*i] = '\0';

    return OK;
}

ErrorCode parse_request_line(char *line, RequestLine *request_line)
{
    size_t i = 0;
    size_t pos = 0;
    while (line[pos] && line[pos] != ' ')
    {
        if (i > 6)
            return ERR_METHOD_LENGTH;
        request_line->method[i++] = line[pos++];
    }
    request_line->method[i] = '\0';

    if (line[pos] != ' ')
        return ERR_BAD_REQUEST_LINE;

    i = 0;
    pos++;
    while (line[pos] && line[pos] != ' ')
    {
        if (i > 254)
            return ERR_TARGET_LENGTH;
        request_line->target[i++] = line[pos++];
    }
    request_line->target[i] = '\0';

    if (line[pos] != ' ')
        return ERR_BAD_REQUEST_LINE;

    i = 0;
    pos++;
    while (line[pos] && line[pos] != ' ')
    {
        if (i > 14)
            return ERR_VERSION_LENGTH;
        request_line->version[i++] = line[pos++];
    }
    request_line->version[i] = '\0';

    if (line[pos] != '\0')
        return ERR_BAD_REQUEST_LINE;

    return OK;
}

ErrorCode parse_header(char *line, char *name, char *value)
{
    int i = 0;
    int j = 0;
    while (line[i] && line[i] != ':')
    {
        if (j > 254)
            return ERR_HEADER_NAME_LENGTH;
        name[j++] = tolower((unsigned char)line[i++]);
    }
    name[j] = '\0';
    j = 0;

    if (line[++i] != ' ')
        return ERR_BAD_HEADER;
    i++;
    while (line[i] && line[i] != '\r')
    {
        if (j > 254)
            return ERR_HEADER_VALUE_LENGTH;
        value[j++] = line[i++];
    }
    value[j] = '\0';
    return OK;
}

void headers_init(Headers *h)
{
    h->headers = NULL;
    h->count = 0;
    h->max_count = 0;
}

ErrorCode headers_append(Headers *h, char *line)
{
    char name[256];
    char value[256];
    ErrorCode error_code = OK;
    if ((error_code = parse_header(line, name, value)))
        return error_code;
    if (h->count == h->max_count)
    {
        size_t new_max = h->max_count ? h->max_count * 2 : 8;
        Header *new_headers = realloc(h->headers, new_max * sizeof(Header));
        if (!new_headers)
            return ERR_HEADERS_REALLOC_FAIL;

        h->headers = new_headers;
        h->max_count = new_max;
    }
    h->headers[h->count].name = strdup(name);
    h->headers[h->count].value = strdup(value);
    if (!h->headers[h->count].name || !h->headers[h->count].value)
    {
        free(h->headers[h->count].name);
        free(h->headers[h->count].value);
        return ERR_HEADERS_ASSIGN_FAIL;
    }
    h->count++;
    return OK;
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

ErrorCode parse_request(char *request, ParsedHttp *parsed_http, int buffer_len)
{
    char line[buffer_len];
    char *p = request;

    int content_length_idx = -1;

    ErrorCode error_code = OK;

    RequestLine request_line;
    Headers headers;
    headers_init(&headers);

    int line_chars = 0;

    error_code = get_line_from_string(&p, line, buffer_len, &line_chars);
    if (error_code)
        return error_code;
    if (line_chars)
    {
        if ((error_code = parse_request_line(line, &request_line)) != OK)
            return error_code;
        printf("Method: %s, Target: %s, Version: %s\n", request_line.method, request_line.target, request_line.version);
    }

    line_chars = 0;
    error_code = get_line_from_string(&p, line, buffer_len, &line_chars);
    if (error_code)
        return error_code;
    while (line_chars)
    {
        error_code = headers_append(&headers, line);
        if (error_code)
            return error_code;

        error_code = get_line_from_string(&p, line, buffer_len, &line_chars);
        if (error_code)
            return error_code;
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
    return error_code;
}

ErrorCode parse_headers(char *buffer, RequestLine *request_line, Headers *headers, int *content_length)
{
    headers_init(headers);
    int buffer_len = 1024;
    char line[buffer_len];
    char *p = buffer;
    int content_length_idx = -1;
    char *end;
    ErrorCode error_code = OK;
    int line_chars = 0;

    error_code = get_line_from_string(&p, line, buffer_len, &line_chars);
    if (error_code)
        return error_code;
    if (line_chars)
    {
        error_code = parse_request_line(line, request_line);
        if (error_code)
            return error_code;
    }
    line_chars = 0;
    error_code = get_line_from_string(&p, line, buffer_len, &line_chars);
    if (error_code)
        return error_code;
    while (line_chars)
    {
        error_code = headers_append(headers, line);
        if (error_code)
            return error_code;

        error_code = get_line_from_string(&p, line, buffer_len, &line_chars);
        if (error_code)
            return error_code;
    }
    if ((content_length_idx = headers_search(headers, "content-length")) >= 0)
    {
        *content_length = strtol(headers->headers[content_length_idx].value, &end, 10);
        if (*content_length > MAX_BODY_LENGTH || *content_length < 0)
            error_code = ERR_CONTENT_LENGTH;
    }
    return error_code;
}

ErrorCode parse_body(char *buffer, int body_idx, int content_length, char **body)
{
    ErrorCode error_code = OK;
    if (content_length > MAX_BODY_LENGTH || content_length < 0)
        return ERR_CONTENT_LENGTH;
    *body = malloc((content_length + 1) * sizeof(char));
    if (!*body)
        return ERR_PARSE_BODY_MALLOC_FAILED;
    int i;
    for (i = body_idx; i < body_idx + content_length; i++)
    {
        (*body)[i - body_idx] = buffer[i];
    }
    (*body)[content_length] = '\0';
    return error_code;
}