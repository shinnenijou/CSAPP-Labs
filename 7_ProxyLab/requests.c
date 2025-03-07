#include <time.h>
#include "requests.h"
#include "csapp.h"
#include "tokens.h"

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3";
static const char *connection_hdr = "Connection: close";
static const char *proxy_conn_hdr = "Proxy-Connection: close";

/*
 * read_headers - read whole header part of HTTP request until continous /r/n
 * user responsibility to release memory
 */
int read_headers(rio_t *rp, char **buf)
{
    size_t capacity = MAXLINE;
    size_t readn = 0;
    char *buffer = (char *)Malloc(capacity);

    while (1)
    {
        int rc = rio_readlineb(rp, buffer + readn, capacity - readn);

        if (rc < 0)
        {
            Free(buffer);
            return -1;
        }
        else if (rc == 0) /* EOF */
        {
            break;
        }

        readn += rc;

        /* check request end continous /r/n */
        if (strncmp(buffer + readn - rc, "\r\n", 2) == 0)
        {
            break;
        }

        /* buffer ran out(too long requeast) */
        if (readn >= capacity)
        {
            Free(buffer);
            return -1;
        }
    }

    buffer[readn] = '\0';
    *buf = buffer;

    return readn;
}

const char *get_status_str(int status_code)
{
    switch (status_code)
    {
    case OK:
        return "OK";
    case BAD_REQUEST:
        return "Bad Request";
    case INTERNAL_SERVER_ERROR:
        return "Internal Server Error";
    case NOT_IMPLEMENTED:
        return "Not Implemented";
    case BAD_GATEWAY:
        return "Bad Gateway";
    default:
        return "Unknown Error";
    }
}

static int parse_request_line(char *usrbuf, size_t len, Request *request)
{
    token_t tokens[4];

    /* should not exceed or less than 3 tokens in HTTP request line */
    if (split_line(tokens, 4, usrbuf, len, ' ') != 3)
    {
        return 0;
    }

    /* method */
    memcpy(request->method, tokens[0].token, tokens[0].size);
    request->method[tokens[0].size] = '\0';

    /* split uri token again and discard other tokens */
    token_t uri_tokens[MAXLINE];
    size_t num = split_line(uri_tokens, MAXLINE, tokens[1].token, tokens[1].size, '/');

    if (num < 2)
    {
        return 0;
    }

    /* relative uri  */
    size_t uri_len = 0;

    for (size_t i = 2; i < num; ++i)
    {
        request->uri[uri_len++] = '/';
        memcpy(request->uri + uri_len, uri_tokens[i].token, uri_tokens[i].size);
        uri_len += uri_tokens[i].size;
        request->uri[uri_len] = '\0';
    }

    /* no specified relative uri */
    if (uri_len == 0)
    {
        request->uri[0] = '/';
    }

    token_t host_tokens[3];

    /* host and port, split by / then the second token is host and port */
    if (split_line(host_tokens, 3, uri_tokens[1].token, uri_tokens[1].size, ':') < 2)
    {
        size_t l = strlen(HTTP_DEFAULT_PORT);
        memcpy(request->port, HTTP_DEFAULT_PORT, l);
        request->port[l] = '\0';

        memcpy(request->host, host_tokens[0].token, host_tokens[0].size);
        request->host[host_tokens[0].size] = '\0';
    }
    else
    {
        memcpy(request->host, host_tokens[0].token, host_tokens[0].size);
        request->host[host_tokens[0].size] = '\0';

        memcpy(request->port, host_tokens[1].token, host_tokens[1].size);
        request->port[host_tokens[1].size] = '\0';
    }

    return 1;
}

static int parse_request_header(char *usrbuf, size_t len, Request *request)
{
    token_t tokens[3];
    int num = split_line(tokens, 3, usrbuf, len, ':');

    /* illegal header */
    if (num < 2)
    {
        return 1;
    }

    if (strncasecmp(tokens[0].token, user_agent_hdr, tokens[0].size) == 0)
    {
        return 1;
    }

    if (strncasecmp(tokens[0].token, connection_hdr, tokens[0].size) == 0)
    {
        return 1;
    }

    if (strncasecmp(tokens[0].token, proxy_conn_hdr, tokens[0].size) == 0)
    {
        return 1;
    }

    if (request->header_size == request->header_capacity)
    {
        request->header_capacity += request->header_capacity;
        request->headers = Realloc(request->headers, request->header_capacity * sizeof(char *));
    }

    request->headers[request->header_size] = Malloc(len + 1);
    memcpy(request->headers[request->header_size], usrbuf, len);
    request->headers[request->header_size][len] = '\0';
    ++request->header_size;

    return 1;
}

static Request *create_request()
{
    Request *request = Malloc(sizeof(Request));

    request->method[0] = '\0';
    request->host[0] = '\0';
    request->port[0] = '\0';
    request->uri[0] = '\0';
    request->user_agent = user_agent_hdr;
    request->connection = connection_hdr;
    request->proxy_connection = proxy_conn_hdr;

    request->header_capacity = 8;
    request->header_size = 0;
    request->headers = Malloc(request->header_capacity * sizeof(char *));

    return request;
}

void release_request(Request *request)
{
    if (!request)
    {
        return;
    }

    for (size_t i = 0; i < request->header_size; ++i)
    {
        free(request->headers[i]);
    }

    free(request->headers);
    free(request);
}

/* parse_request - parse a null-terminated string as a HTTP request */
Request *parse_request(void *usrbuf)
{
    if (usrbuf == NULL)
    {
        return NULL;
    }

    size_t len = strlen(usrbuf);
    token_t lines[MAXLINE];
    int line_cnt = split_line(lines, MAXLINE, usrbuf, len, '\n');

    if (line_cnt == 0)
    {
        return NULL;
    }

    /* tokens will contain \r, remove is manually*/
    for (size_t i = 0; i < line_cnt; ++i)
    {
        rstrip_token(lines + i);
    }

    Request *request = create_request();

    if (!parse_request_line(lines[0].token, lines[0].size, request))
    {
        release_request(request);
        return NULL;
    }

    for (size_t i = 1; i < line_cnt; ++i)
    {
        if (!parse_request_header(lines[i].token, lines[i].size, request))
        {
            release_request(request);
            return NULL;
        }
    }

    return request;
}

int write_request(int fd, Request *request)
{
    char buffer[MAXLINE + 100];

    sprintf(buffer, "%s", request->method);

    if (rio_writen(fd, buffer, strlen(buffer)) < 0)
    {
        return -1;
    }

    sprintf(buffer, " %s HTTP/1.0\r\n", request->uri);

    if (rio_writen(fd, buffer, strlen(buffer)) < 0)
    {
        return -1;
    }

    sprintf(buffer, "%s\r\n", request->user_agent);

    if (rio_writen(fd, buffer, strlen(buffer)) < 0)
    {
        return -1;
    }

    sprintf(buffer, "%s\r\n", request->connection);

    if (rio_writen(fd, buffer, strlen(buffer)) < 0)
    {
        return -1;
    }

    sprintf(buffer, "%s\r\n", request->proxy_connection);

    if (rio_writen(fd, buffer, strlen(buffer)) < 0)
    {
        return -1;
    }

    for (size_t i = 0; i < request->header_size; ++i)
    {
        sprintf(buffer, "%s\r\n", request->headers[i]);

        if (rio_writen(fd, buffer, strlen(buffer)) < 0)
        {
            return -1;
        }
    }

    if (rio_writen(fd, HTTP_REQUEST_END, strlen(HTTP_REQUEST_END)) < 0)
    {
        return -1;
    }

    return 0;
}

void debug_print_request(Request *request)
{
    printf("Method: %s\n", request->method);
    printf("Host: %s\n", request->host);
    printf("Port: %s\n", request->port);
    printf("URI: %s\n", request->uri);

    printf("--- Predefined Headers ----\n");
    printf("%s\n", request->connection);
    printf("%s\n", request->proxy_connection);
    printf("%s\n", request->user_agent);

    printf("--- User Request Headers ----\n");

    for (size_t i = 0; i < request->header_size; ++i)
    {
        printf("%s\n", request->headers[i]);
    }
}

static Response *create_response()
{
    Response *resp = Malloc(sizeof(Response));
    resp->content_type[0] = '\0';
    resp->content_length = 0;
    resp->status_code = 0;
    resp->content = NULL;
    return resp;
}

void release_response(Response *response)
{
    if (!response)
    {
        return;
    }
    Free(response->content);
    Free(response);
}

static int parse_status_line(char *usrbuf, size_t len, Response *response)
{
    token_t tokens[MAXLINE];
    int token_cnt = split_line(tokens, MAXLINE, usrbuf, len, ' ');

    /* illegal response */
    if (token_cnt < 3)
    {
        return 0;
    }

    response->status_code = atoi(tokens[1].token);

    if (response->status_code == 0)
    {
        return 0;
    }

    char *p = response->status;

    for (size_t i = 2; i < token_cnt; ++i)
    {
        memcpy(p, tokens[i].token, tokens[i].size);
        p[tokens[i].size] = ' ';
        p += tokens[i].size + 1;
    }

    *(p - 1) = '\0';

    return 1;
}

static int parse_response_header(char *usrbuf, size_t len, Response *response)
{
    token_t tokens[3];
    int token_cnt = split_line(tokens, 3, usrbuf, len, ':');

    if (token_cnt < 2)
    {
        return 1;
    }

    if (strncasecmp(tokens[0].token, "Content-type", tokens[0].size) == 0)
    {
        memcpy(response->content_type, usrbuf, len);
        response->content_type[len] = '\0';
        return 1;
    }

    if (strncasecmp(tokens[0].token, "Content-length", tokens[0].size) == 0)
    {
        int length = atoi(tokens[1].token);

        if (length < 0)
        {
            return 0;
        }

        response->content_length = length;
        response->content = Malloc(length);
        return 1;
    }

    return 1;
}

/*
 * parse_response - parse given null-terminated string as HTTP response headers
 * user responsibility to release returned pointer
 */
Response *parse_response(void *usrbuf)
{
    token_t lines[MAXLINE];
    int line_cnt = split_line(lines, MAXLINE, usrbuf, strlen(usrbuf), '\n');

    /* at least status line*/
    if (line_cnt < 1)
    {
        return NULL;
    }

    /* tokens will contain \r, remove is manually*/
    for (size_t i = 0; i < line_cnt; ++i)
    {
        rstrip_token(lines + i);
    }

    Response *resp = create_response();

    /* split status line */
    if (!parse_status_line(lines[0].token, lines[0].size, resp))
    {
        release_response(resp);
        return NULL;
    }

    for (size_t i = 0; i < line_cnt; ++i)
    {
        if (!parse_response_header(lines[i].token, lines[i].size, resp))
        {
            release_response(resp);
            return NULL;
        }
    }

    return resp;
}

void debug_print_response(Response *response)
{
    printf("Content-length: %ld\n", response->content_length);
    printf("Status_code: %d\n", response->status_code);
    printf("Content-type: %s\n", response->content_type);
    printf("Status: %s\n", response->status);
    printf("Ststus code: %d\n", response->status_code);
    printf("Content: %s\n", response->content);
}