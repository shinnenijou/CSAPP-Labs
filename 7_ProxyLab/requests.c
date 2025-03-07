#include <time.h>
#include "requests.h"
#include "csapp.h"
#include "tokens.h"

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_conn_hdr = "Proxy-Connection: close\r\n";

int end_of_request(const char *buf, size_t len)
{
    for (size_t i = len - 4; i < len; --i)
    {
        if (strncmp(buf + i, "\r\n\r\n", 4) == 0)
        {
            return 1;
        }
    }

    return 0;
}

static int try_readn(int fd, void *usrbuf, size_t n)
{
    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(fd, &read_set);

    struct timeval timev;
    timev.tv_sec = DEFAULT_TIMEOUT;
    timev.tv_usec = 0;

    int nready = select(fd + 1, &read_set, NULL, NULL, &timev);

    if (nready <= 0)
    {
        return -1;
    }

    int rc = read(fd, usrbuf, n);

    if (rc < 0)
    {
        unix_error("read error");
    }

    return rc;
}

int request_readn(int fd, void *usrbuf, size_t maxlen)
{
    size_t rest_size = maxlen;
    char *buf = usrbuf;

    while (rest_size > 0)
    {
        int rc = try_readn(fd, buf, rest_size);

        /* timeout */
        if (rc < 0)
        {
            return -1;
        }

        /* EOF */
        if (rc == 0)
        {
            break;
        }

        rest_size -= rc;
        buf += rc;
    }

    *buf = '\0';

    return maxlen - rest_size;
}

/* read_headers - read whole header part of HTTP request until continous /r/n
 * MAY contains body except headers
 */
int read_headers(int fd, void *usrbuf, size_t maxlen)
{
    size_t rest_size = maxlen;
    char *buf = usrbuf;

    while (rest_size > 0)
    {
        int rc = try_readn(fd, buf, rest_size);

        /* timeout */
        if (rc < 0)
        {
            return -1;
        }

        /* EOF */
        if (rc == 0)
        {
            break;
        }

        rest_size -= rc;
        buf += rc;

        /* read successfully. check continous /r/n */
        if (end_of_request(buf - rc, rc))
        {
            break;
        }
    }

    *buf = '\0';

    return maxlen - rest_size;
}

/*
 * request_writen -
 */
int request_writen(int fd, void *usrbuf, size_t maxlen)
{
    return rio_writen(fd, usrbuf, maxlen);
}

Request *create_request()
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

void debug_print_request(Request *request)
{
    printf("Method: %s\n", request->method);
    printf("Host: %s\n", request->host);
    printf("Port: %s\n", request->port);
    printf("URI: %s\n", request->uri);

    printf("--- Predefined Headers ----\n");
    printf("%s", request->connection);
    printf("%s", request->proxy_connection);
    printf("%s", request->user_agent);

    printf("--- User Request Headers ----\n");

    for (size_t i = 0; i < request->header_size; ++i)
    {
        printf("%s", request->headers[i]);
    }
}

/* parse_response - parse response headers, return remaining bytes of body */
int parse_response(void *usrbuf, void *content_type)
{
    char *buf = usrbuf;
    int content_len = 0;

    for (size_t i = 1; buf[i] != '\0'; ++i)
    {
        if (buf[i - 1] != '\r' || buf[i] != '\n')
        {
            continue;
        }

        /* end of headers */
        if (i == 1)
        {
            buf += 2;
            break;
        }

        if (strncasecmp(buf, "Content-length:", 15) == 0)
        {
            content_len = atoi(buf + 15);

            if (content_len == 0)
            {
                return -1;
            }
        }
        else if (strncasecmp(buf, "Content-type:", 13) == 0)
        {
            char c = buf[i + 1];
            buf[i + 1] = '\0';
            strcpy(content_type, buf);
            buf[i + 1] = c;
        }

        buf += i + 1;
        i = 0;
    }

    size_t read_len = strlen(buf);
    return content_len - read_len;
}

size_t make_request_string(Request *request, char *usrbuf)
{
    int n;
    char *bufptr = usrbuf;

    if ((n = sprintf(bufptr, "%s %s HTTP/1.0\r\n", request->method, request->uri)) < 0)
    {
        return 0;
    }

    bufptr += n;

    if ((n = sprintf(bufptr, "Host: %s\r\n", request->host)) < 0)
    {
        return 0;
    }

    bufptr += n;

    if ((n = sprintf(bufptr, "%s", request->user_agent)) < 0)
    {
        return 0;
    }

    bufptr += n;

    if ((n = sprintf(bufptr, "%s", request->connection)) < 0)
    {
        return 0;
    }

    bufptr += n;

    if ((n = sprintf(bufptr, "%s", request->proxy_connection)) < 0)
    {
        return 0;
    }

    bufptr += n;

    for (size_t i = 0; i < request->header_size; ++i)
    {
        if ((n = sprintf(bufptr, "%s", request->headers[i])) < 0)
        {
            return 0;
        }

        bufptr += n;
    }

    if ((n = sprintf(bufptr, "\r\n")) < 0)
    {
        return 0;
    }

    bufptr += n;

    return bufptr - usrbuf;
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

        memcpy(request->host, host_tokens[1].token, host_tokens[1].size);
        request->host[host_tokens[1].size] = '\0';
    }

    return 1;
}

static int parse_header_line(char *usrbuf, size_t len, Request *request)
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

    if (strncasecmp(tokens[0].token, "Host:", tokens[0].size) == 0)
    {
        return 1;
    }

    size_t size = strlen(usrbuf);

    if (request->header_size == request->header_capacity)
    {
        request->header_capacity += request->header_capacity;
        request->headers = Realloc(request->headers, request->header_capacity * sizeof(char *));
    }

    request->headers[request->header_size] = Malloc(size + 1);
    memcpy(request->headers[request->header_size], usrbuf, size);
    request->headers[request->header_size][size] = '\0';
    ++request->header_size;

    return 1;
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

    /* tokens will not contains \n, add it manually */
    for (size_t i = 0; i < line_cnt; ++i)
    {
        lines[i].token[lines[i].size++] = '\n';
    }

    Request *request = create_request();

    if (!parse_request_line(lines[0].token, lines[0].size, request))
    {
        release_request(request);
        return NULL;
    }

    for (size_t i = 1; i < line_cnt; ++i)
    {
        if (!parse_header_line(lines[i].token, lines[i].size, request))
        {
            release_request(request);
            return NULL;
        }
    }

    return request;
}
