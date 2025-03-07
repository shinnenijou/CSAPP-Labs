#include <stdio.h>
#include <bits/types/struct_timeval.h>

#include "csapp.h"
#include "requests.h"
#include "queue.h"
#include "cache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* Work threads number */
#define WORK_THREADS_NUM 4
#define QUEUE_MAX_LEN 1024

/* global states */
cache_pool_t *cache_pool = NULL;

/* terminate flag. server should wait for service completing after recerive signal to terminate */
volatile int terminate = 0;

static void *thread(void *arg);
static void serve(int fd);
static int do_request(Request *request, int clientfd);
static int do_get(Request *request, int clientfd);
static void clienterror(int fd, char *cause, int errnum, char *longmsg);

static void sigint_handler(int sig);
static void sigpipe_handler(int sig);

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

    /* set signal handlers */
    Signal(SIGINT, sigint_handler);  /* ctrl-c */
    Signal(SIGTSTP, sigint_handler); /* ctrl-z */
    Signal(SIGPIPE, sigpipe_handler);

    /* init cache pool */
    cache_pool = create_cache_pool(MAX_CACHE_SIZE, MAX_OBJECT_SIZE);

    /* init task queue */
    queue_t *queue = queue_create(QUEUE_MAX_LEN);

    /* create thread pool */
    pthread_t threads[WORK_THREADS_NUM];

    for (size_t i = 0; i < WORK_THREADS_NUM; ++i)
    {
        Pthread_create(threads + i, NULL, thread, (void *)queue);
    }

    listenfd = Open_listenfd(argv[1]);

    fd_set ready_set;
    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(listenfd, &read_set);

    /*
     * instead of accept system call not interrupted by signal, this function
     * use select system call to check readable fd then could safely terminate by signal
     */
    while (!terminate)
    {
        ready_set = read_set;
        int old_errno = errno;
        int nready = select(listenfd + 1, &ready_set, NULL, NULL, NULL);

        if (nready > 0 && FD_ISSET(listenfd, &ready_set))
        {
            clientlen = sizeof(clientaddr);
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // line:netp:tiny:accept
            Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
            printf("Accepted connection from (%s, %s)\n", hostname, port);
            queue_put(queue, connfd); /* connection will be closed in thread */
        }
        else if (errno == EINTR)
        {
            errno = old_errno;
        }
        else
        {
            unix_error("select error");
        }
    }

    /* trick: send minus fd as a stop signal to threads to stop threads */
    for (size_t i = 0; i < WORK_THREADS_NUM; ++i)
    {
        queue_put(queue, -1);
    }

    for (size_t i = 0; i < WORK_THREADS_NUM; ++i)
    {
        Pthread_join(threads[i], NULL);
    }

    queue_free(queue);
    release_cache_pool(cache_pool);

    return 0;
}

static void *thread(void *arg)
{
    queue_t *queue = (queue_t *)arg;
    int fd;

    while ((fd = queue_get(queue)) > 0)
    {
        serve(fd);
        Close(fd);
    }

    return NULL;
}

static void serve(int fd)
{
    char *buffer = NULL;

    if (read_headers(fd, &buffer) > 0)
    {
        Request *request = parse_request(buffer);

        if (request)
        {
            int status = do_request(request, fd);

            if (status != OK)
            {
                clienterror(fd, "", status, "Proxy met something wrong");
            }
        }
        else
        {
            clienterror(fd, "", BAD_REQUEST, "Cannot parse request");
        }

        release_request(request);
    }
    else
    {
        clienterror(fd, "", BAD_GATEWAY, "");
    }

    Free(buffer);
}

/* do_request - do the proxy request then return the response from end server */
static int do_request(Request *request, int clientfd)
{
    if (strcmp(request->method, "GET") == 0)
    {
        return do_get(request, clientfd);
    }

    return NOT_IMPLEMENTED;
}

static int do_get(Request *request, int clientfd)
{
    /* try to get content from cache */
    char *cache = NULL;
    int size = read_cache(cache_pool, request, &cache);

    if (size > 0)
    {
        rio_writen(clientfd, cache, size);
        Free(cache);
        return OK;
    }

    int fd = 0;

    if ((fd = open_clientfd(request->host, request->port)) < 0)
    {
        return BAD_GATEWAY;
    }

    if (write_request(fd, request) < 0)
    {
        return BAD_GATEWAY;
    }

    char *resp_headers = NULL;

    if (read_headers(fd, &resp_headers) < 0)
    {
        return BAD_GATEWAY;
    }

    Response *resp = parse_response(resp_headers);

    if (resp == NULL)
    {
        return BAD_GATEWAY;
    }

    if (rio_readn(fd, resp->content, resp->content_length) < 0)
    {
        Free(resp);
        return BAD_GATEWAY;
    }

    /* close connection with content server */
    Close(fd);

    /* write to cahce */
    if (resp->content)
    {
        write_cache(cache_pool, request, resp);
    }

    /* write content to proxy client */
    rio_writen(clientfd, resp_headers, strlen(resp_headers));

    if (resp->content)
    {
        rio_writen(clientfd, resp->content, resp->content_length);
    }

    return OK;
}

/*
 * clienterror - returns an error message to the client
 */
static void clienterror(int fd, char *cause, int errnum, char *longmsg)
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %d %s\r\n", errnum, get_status_str(errnum));
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Proxy Error</title>");
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=ffffff>\r\n");
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%d: %s\r\n", errnum, get_status_str(errnum));
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Proxy</em>\r\n");
    rio_writen(fd, buf, strlen(buf));
}

static void sigint_handler(int sig)
{
    sigset_t set, old_set;
    Sigfillset(&set);
    Sigprocmask(SIG_BLOCK, &set, &old_set);

    int old_errno = errno;
    char buffer[MAXLINE];
    sprintf(buffer, "\nReceived signal %d(%s), server will exit...\n", sig, strsignal(sig));
    write(STDOUT_FILENO, buffer, strlen(buffer));
    terminate = 1;
    errno = old_errno;

    Sigprocmask(SIG_SETMASK, &old_set, NULL);
}

/* actual ignore SIGPIPE but will interrupt system call */
static void sigpipe_handler(int sig)
{
}