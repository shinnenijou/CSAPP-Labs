#include <time.h>
#include "requests.h"
#include "csapp.h"

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

/* read_headers - read whole header part of HTTP request until continous /r/n */
int read_headers(int fd, void *usrbuf, size_t maxlen)
{
    char *buf = usrbuf;

    fd_set ready_set;
    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(fd, &read_set);

    struct timeval timev;
    timev.tv_sec = DEFAULT_TIMEOUT;
    timev.tv_usec = 0;

    size_t rest_size = maxlen;

    while (rest_size > 0)
    {
        ready_set = read_set;
        int old_errno = errno;
        int nready = select(fd + 1, &ready_set, NULL, NULL, &timev);

        if (nready < 0)
        {
            if (errno != EINTR)
            {
                return -1;
            }

            errno = old_errno;
        }
        else if (nready == 0) /* time out */
        {
            return -1;
        }
        else
        {
            size_t rc = read(fd, buf, rest_size);

            if (rc < 0)
            {
                return -1;
            }
            else if (rc == 0)
            { /* EOF */
                break;
            }

            rest_size -= rc;

            /* read successfully. check continous /r/n */
            if (end_of_request(buf, rc))
            {
                break;
            }

            buf += rc;
        }
    }

    *((char *)usrbuf + maxlen - rest_size) = '\0';

    return maxlen - rest_size;
}

int request_writen(int fd, void *usrbuf, size_t maxlen)
{
    Rio_writen(fd, usrbuf, maxlen);
    return maxlen;
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

static char *strfind(char *str, size_t len, char delim)
{
    for (size_t i = 0; i < len && str[i] != '\0'; i++)
    {
        if (str[i] == delim)
        {
            return &str[i];
        }
    }

    return NULL;
}

int parse_request_line(char *usrbuf, Request *request)
{
    if (!usrbuf || !request)
    {
        return 0;
    }

    size_t size;
    char *sep;

    /* parse method */
    size = strlen(usrbuf);
    sep = strfind(usrbuf, size, ' ');

    if (sep == NULL)
    {
        return 0;
    }

    *sep = '\0';
    strcpy(request->method, usrbuf);
    *sep = ' ';

    /* parse uri */
    size -= sep - usrbuf;
    usrbuf = ++sep;
    sep = strfind(usrbuf, size, ' ');

    if (sep == NULL)
    {
        return 0;
    }

    char uri[MAXLINE];
    char *urip;

    *sep = '\0';
    strcpy(uri, usrbuf);
    *sep = ' ';

    /* The absoluteURI form is only allowed when the request is being made to a proxy.  */

    /* scheme */
    urip = uri;

    if ((sep = strfind(urip, MAXLINE, ':')) == NULL)
    {
        return 0;
    }

    *sep = '\0';
    strcpy(request->scheme, urip);
    *sep = ':';

    /* host and port */
    urip = sep + 3;
    sep = strfind(urip, MAXLINE - (urip - uri), ':');

    /* host */
    if (sep)
    {
        *sep = '\0';
        strcpy(request->host, urip);
        *sep = ':';
    }
    else
    {
        char *slash = strfind(urip, MAXLINE - (urip - uri), '/');

        if (slash)
        {
            *slash = '\0';
        }

        strcpy(request->host, urip);

        if (slash)
        {
            *slash = '/';
        }
    }

    /* port */
    if (sep)
    {
        char *slash = strfind(sep, MAXLINE - (sep - uri), '/');

        if (slash)
        {
            *slash = '\0';
        }

        int port = atoi(sep + 1);

        if (port == 0)
        {
            return 0;
        }

        strcpy(request->port, sep + 1);

        if (slash)
        {
            *slash = '/';
        }
    }
    else
    {
        strcpy(request->port, HTTP_DEFAULT_PORT);
    }

    /* relative uri */
    sep = strfind(urip, MAXLINE - (urip - uri), '/');
    strcpy(request->uri, sep ? sep : HTTP_DEFAULT_URI);

    return 1;
}

int parse_header_line(char *usrbuf, Request *request)
{
    if (!usrbuf || !request)
    {
        return 0;
    }

    char *sep = strfind(usrbuf, MAXLINE, ':');

    if (!sep)
    {
        return 0;
    }

    if (strncasecmp(usrbuf, user_agent_hdr, sep - usrbuf) == 0)
    {
        return 1;
    }

    if (strncasecmp(usrbuf, connection_hdr, sep - usrbuf) == 0)
    {
        return 1;
    }

    if (strncasecmp(usrbuf, proxy_conn_hdr, sep - usrbuf) == 0)
    {
        return 1;
    }

    if (strncasecmp(usrbuf, "Host:", sep - usrbuf) == 0)
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
    strcpy(request->headers[request->header_size], usrbuf);
    ++request->header_size;

    return 1;
}

int parse_request(void *usrbuf, Request *request)
{
    char buffer[MAXLINE];
    char *buf = usrbuf;

    int first_line = 1;

    /* split other headers */
    for (size_t i = 1; buf[i] != '\0'; ++i)
    {
        if (buf[i - 1] != '\r' || buf[i] != '\n')
        {
            continue;
        }

        if (++i >= MAXLINE)
        {
            return 0;
        }

        strncpy(buffer, buf, i);
        buffer[i] = '\0';

        /* end of request */
        if (strncmp(buffer, "\r\n", 2) == 0)
        {
            break;
        }

        if (first_line)
        {
            if (!parse_request_line(buffer, request))
            {
                return 0;
            }

            first_line = 0;
        }
        else
        {
            if (!parse_header_line(buffer, request))
            {
                return 0;
            }
        }

        buf += i;
        i = 1;
    }

    return 1;
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