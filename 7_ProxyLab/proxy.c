#include <stdio.h>

#include "csapp.h"
#include "requests.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

static void serve(int fd);
static size_t do_request(Request *request, char *response);
static size_t do_get(Request *request, char *response);
static void clienterror(int fd, char *cause, int errnum, char *shortmsg, char *longmsg);

int main(int argc, char **argv)
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1)
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // line:netp:tiny:accept
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        serve(connfd);
        Close(connfd); // line:netp:tiny:close
    }

    return 0;
}

static void serve(int fd)
{
    rio_t rio;
    Rio_readinitb(&rio, fd);
    char buffer[MAXLINE];

    // parse method & URI
    if (!read_request_line(&rio, buffer, MAXLINE))
    {
        return;
    }

    Request *request = create_request();

    if (!parse_request_line(buffer, request))
    {
        clienterror(fd, buffer, BAD_REQUEST, "Bad Request", "Cannot parse this request.");
        release_request(request);
        return;
    }

    if (strcmp(request->method, "GET") != 0 && strcmp(request->method, "POST") != 0)
    {
        clienterror(fd, request->method, NOT_IMPLEMENTED, "Not Implmented", "Proxy does not implemented this method.");
        release_request(request);
        return;
    }

    while (read_request_line(&rio, buffer, MAXLINE))
    {
        if (end_of_request(buffer))
        {
            break;
        }

        if (!parse_header_line(buffer, request))
        {
            clienterror(fd, buffer, BAD_REQUEST, "Bad Request", "Illegal header");
            release_request(request);
            return;
        }
    }

    char *response = Malloc(MAX_OBJECT_SIZE);
    size_t response_size = do_request(request, response);

    if (response_size == 0)
    {
        clienterror(fd, "", INTERNAL_SERVER_ERROR, "Internal Server Error", "Proxy met something wrong.");
    }
    else
    {
        Rio_writen(fd, response, response_size);
    }

    Free(response);
    release_request(request);
}

/* do_request - do the proxy request then return the response from end server */
static size_t do_request(Request *request, char *response)
{
    if (strcmp(request->method, "GET"))
    {
        return do_get(request, response);
    }

    return 0;
}

static size_t do_get(Request *request, char *response)
{
    return 0;
}

/*
 * clienterror - returns an error message to the client
 */
static void clienterror(int fd, char *cause, int errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %d %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Proxy Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=ffffff>\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%d: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Proxy</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}