#ifndef TYPES_H
#define TYPES_H

#include <pthread.h>
#include <stddef.h>
#include <sys/types.h>

#define NUM_THREADS 4
#define MAX_CLIENTS 20
#define TIMEOUT 5000
// todo: BUFFER_LEN way too big, but can't be different to MAX_HEADER_LENGTH atm due to request() in socket.c
#define BUFFER_LEN 8192
#define MAX_BODY_LENGTH 1048576
#define MAX_HEADER_LENGTH 8192

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
    ERR_HEADER_LENGTH,
    ERR_CONTENT_LENGTH,
    ERR_PARSE_BODY_MALLOC_FAILED,
    ERR_LINE_OOB,
} ErrorCode;

typedef enum
{
    CONN_READING_HEADERS,
    CONN_READING_BODY,
    CONN_PROCESSING,
    CONN_RESPONSE_FLATTEN,
    CONN_RESPONSE_SEND,
} ConnectionState;

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
} HttpRequest;

typedef struct
{
    char version[16];
    int status;
    char reason[256];
} ResponseLine;

typedef struct
{
    ResponseLine response_line;
    Headers headers;
    char *body;
} HttpResponse;

typedef struct
{
    pthread_mutex_t mutex;
    int done;
    int fd;
    ConnectionState state;
    char buffer[BUFFER_LEN];
    size_t buffer_used;
    size_t content_length;
    HttpRequest req;
    HttpResponse res;
    size_t f_r_len;
    size_t f_r_sent;
    char *flattened_response;
} Connection;

typedef struct
{
    char method[8];
    char endpoint[256];
    void (*handler)(HttpRequest, HttpResponse *);
} Route;

typedef struct
{
    const Route *routes;
    const int route_count;
} Routes;

typedef enum
{
    /* 1xx Informational */
    HTTP_STATUS_CONTINUE = 100,
    HTTP_STATUS_SWITCHING_PROTOCOLS = 101,
    HTTP_STATUS_PROCESSING = 102,                /* WebDAV */
    HTTP_STATUS_EARLY_HINTS = 103,

    /* 2xx Success */
    HTTP_STATUS_OK = 200,
    HTTP_STATUS_CREATED = 201,
    HTTP_STATUS_ACCEPTED = 202,
    HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION = 203,
    HTTP_STATUS_NO_CONTENT = 204,
    HTTP_STATUS_RESET_CONTENT = 205,
    HTTP_STATUS_PARTIAL_CONTENT = 206,
    HTTP_STATUS_MULTI_STATUS = 207,              /* WebDAV */
    HTTP_STATUS_ALREADY_REPORTED = 208,          /* WebDAV */
    HTTP_STATUS_IM_USED = 226,

    /* 3xx Redirection */
    HTTP_STATUS_MULTIPLE_CHOICES = 300,
    HTTP_STATUS_MOVED_PERMANENTLY = 301,
    HTTP_STATUS_FOUND = 302,
    HTTP_STATUS_SEE_OTHER = 303,
    HTTP_STATUS_NOT_MODIFIED = 304,
    HTTP_STATUS_USE_PROXY = 305,                 /* Deprecated */
    HTTP_STATUS_UNUSED = 306,
    HTTP_STATUS_TEMPORARY_REDIRECT = 307,
    HTTP_STATUS_PERMANENT_REDIRECT = 308,

    /* 4xx Client Error */
    HTTP_STATUS_BAD_REQUEST = 400,
    HTTP_STATUS_UNAUTHORIZED = 401,
    HTTP_STATUS_PAYMENT_REQUIRED = 402,
    HTTP_STATUS_FORBIDDEN = 403,
    HTTP_STATUS_NOT_FOUND = 404,
    HTTP_STATUS_METHOD_NOT_ALLOWED = 405,
    HTTP_STATUS_NOT_ACCEPTABLE = 406,
    HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED = 407,
    HTTP_STATUS_REQUEST_TIMEOUT = 408,
    HTTP_STATUS_CONFLICT = 409,
    HTTP_STATUS_GONE = 410,
    HTTP_STATUS_LENGTH_REQUIRED = 411,
    HTTP_STATUS_PRECONDITION_FAILED = 412,
    HTTP_STATUS_CONTENT_TOO_LARGE = 413,
    HTTP_STATUS_URI_TOO_LONG = 414,
    HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE = 415,
    HTTP_STATUS_RANGE_NOT_SATISFIABLE = 416,
    HTTP_STATUS_EXPECTATION_FAILED = 417,
    HTTP_STATUS_IM_A_TEAPOT = 418,
    HTTP_STATUS_MISDIRECTED_REQUEST = 421,
    HTTP_STATUS_UNPROCESSABLE_CONTENT = 422,     /* WebDAV */
    HTTP_STATUS_LOCKED = 423,                    /* WebDAV */
    HTTP_STATUS_FAILED_DEPENDENCY = 424,         /* WebDAV */
    HTTP_STATUS_TOO_EARLY = 425,
    HTTP_STATUS_UPGRADE_REQUIRED = 426,
    HTTP_STATUS_PRECONDITION_REQUIRED = 428,
    HTTP_STATUS_TOO_MANY_REQUESTS = 429,
    HTTP_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
    HTTP_STATUS_UNAVAILABLE_FOR_LEGAL_REASONS = 451,

    /* 5xx Server Error */
    HTTP_STATUS_INTERNAL_SERVER_ERROR = 500,
    HTTP_STATUS_NOT_IMPLEMENTED = 501,
    HTTP_STATUS_BAD_GATEWAY = 502,
    HTTP_STATUS_SERVICE_UNAVAILABLE = 503,
    HTTP_STATUS_GATEWAY_TIMEOUT = 504,
    HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED = 505,
    HTTP_STATUS_VARIANT_ALSO_NEGOTIATES = 506,
    HTTP_STATUS_INSUFFICIENT_STORAGE = 507,      /* WebDAV */
    HTTP_STATUS_LOOP_DETECTED = 508,             /* WebDAV */
    HTTP_STATUS_NOT_EXTENDED = 510,
    HTTP_STATUS_NETWORK_AUTHENTICATION_REQUIRED = 511,
} HttpStatusCode;

#endif