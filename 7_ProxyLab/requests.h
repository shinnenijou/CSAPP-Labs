#ifndef __REQUESTS_H__
#define __REQUESTS_H__

#include <time.h>
#include "csapp.h"

#define HTTP_DEFAULT_PORT "80"
#define HTTP_DEFAULT_URI "/"
#define HTTP_REQUEST_END "\r\n"

/* status codes */
#define OK 200
#define BAD_REQUEST 400
#define INTERNAL_SERVER_ERROR 500
#define NOT_IMPLEMENTED 501
#define BAD_GATEWAY 502

/* Timer */
#define DEFAULT_TIMEOUT 3

/* Request */
typedef struct
{
    char method[MAXLINE];
    char host[MAXLINE];
    char uri[MAXLINE];
    char scheme[MAXLINE];
    char port[MAXLINE];
    const char *user_agent;
    const char *connection;
    const char *proxy_connection;
    char **headers;
    size_t header_capacity;
    size_t header_size;
} Request;

/* Request struct helper */
void release_request(Request *request);
Request *parse_request(void *usrbuf);
void debug_print_request(Request *request);

/* Response */
typedef struct
{
    size_t content_length;
    int status_code;
    char *content;
    char content_type[MAXLINE];
    char status[MAXLINE];
} Response;

/* Response struct helper */
Response *parse_response(void *usrbuf);
void release_response(Response *response);
void debug_print_response(Response *response);

/* I/O wrappers */
int read_headers(rio_t *rp, char **buf);
int write_request(int fd, Request *request);

/* Misc */
const char *get_status_str(int status_code);

#endif