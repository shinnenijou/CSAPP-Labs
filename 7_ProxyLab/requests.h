#ifndef __REQUESTS_H__
#define __REQUESTS_H__

#include <time.h>
#include "csapp.h"

#define HTTP_DEFAULT_PORT "80"
#define HTTP_DEFAULT_URI "/"

/* status codes */
#define OK 200
#define BAD_REQUEST 400
#define INTERNAL_SERVER_ERROR 500
#define NOT_IMPLEMENTED 501
#define BAD_GATEWAY 502

/* Timer */
#define DEFAULT_TIMEOUT 3

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

Request *create_request();
void release_request(Request *request);

void debug_print_request(Request *request);
int parse_request_line(char *usrbuf, Request *request);
int parse_header_line(char *usrbuf, Request *request);
int parse_request(void *usrbuf, Request *request);
int parse_response(void *usrbuf);
size_t make_request_string(Request *request, char *usrbuf);

int request_writen(int fd, void *usrbuf, size_t maxlen);
int request_readn(int fd, void *usrbuf, size_t maxlen);
int read_headers(int fd, void *usrbuf, size_t maxlen);

const char *get_status_str(int status_code);

#endif