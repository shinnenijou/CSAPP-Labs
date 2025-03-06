#ifndef __REQUESTS_H__
#define __REQUESTS_H__

#include "csapp.h"

#define HTTP_DEFAULT_PORT "80"
#define HTTP_DEFAULT_URI "/"

/* status codes */
#define OK 200
#define BAD_REQUEST 400
#define INTERNAL_SERVER_ERROR 500
#define NOT_IMPLEMENTED 501
#define BAD_GATEWAY 502

#define HTTP_GET 0x1
#define HTTP_POST 0x2

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
size_t make_request_string(Request *request, char *usrbuf);

int read_request_line(rio_t *rp, void *usrbuf, size_t maxlen);
int end_of_request(const char *line);

const char *get_status_str(int status_code);

#endif