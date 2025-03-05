#include "requests.h"
#include "csapp.h"

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_conn_hdr = "Proxy-Connection: close\r\n";

int read_request_line(rio_t *rp, void *usrbuf, size_t maxlen)
{
    int total = 0;
    int n = 0;
    char *buf = (char *)usrbuf;

    while ((n = rio_readlineb(rp, buf, maxlen)))
    {
        if (n < 0)
        {
            unix_error("read_request_line error");
        }

        total += n;
        buf += n;

        if (total > 1 && strcmp(buf - 2, "\r\n") == 0)
        {
            break;
        }
    }

    return total;
}

int end_of_request(const char *line)
{
    return strcmp(line, "\r\n") == 0;
}

Request *create_request()
{
    Request *request = Malloc(sizeof(Request));

    request->method[0] = '\0';
    request->host[0] = '\0';
    request->port = 0;
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
    printf("Port: %d\n", request->port);
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
    for (size_t i = 0; i < len; i++)
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

        request->port = port;

        if (slash)
        {
            *slash = '/';
        }
    }
    else
    {
        request->port = HTTP_DEFAULT_PORT;
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