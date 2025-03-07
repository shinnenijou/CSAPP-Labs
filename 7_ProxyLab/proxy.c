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
static int do_request(Request *request, char **response, size_t *len);
static int do_get(Request *request, char **response, size_t *len);
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

/* TODO deal with SIGPIPE/EPIPE */
static void serve(int fd)
{
    char *buffer = Malloc(MAX_OBJECT_SIZE);

    if (read_headers(fd, buffer, MAX_OBJECT_SIZE) > 0)
    {
        Request *request = parse_request(buffer);

        if (request)
        {
            debug_print_request(request);

            // char *response = NULL;

            // size_t size = 0;
            // int status = do_request(request, &response, &size);

            // if (status != OK)
            // {
            //     clienterror(fd, "", status, "Proxy met something wrong");
            // }
            // else
            // {
            //     request_writen(fd, response, size);
            // }

            // Free(response);
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
static int do_request(Request *request, char **response, size_t *len)
{
    if (strcmp(request->method, "GET") == 0)
    {
        return do_get(request, response, len);
    }

    return NOT_IMPLEMENTED;
}

static int do_get(Request *request, char **response, size_t *len)
{
    /* try to get content from cache */
    char *content = read_cache(cache_pool, request->host, request->port, request->uri);

    if (content)
    {
        *len = strlen(content);
        *response = content;
        return OK;
    }

    char *buf = (char *)Malloc(MAX_OBJECT_SIZE);
    size_t n;
    int status = OK;

    if ((n = make_request_string(request, buf)) > 0)
    {
        int fd = 0;

        if ((fd = open_clientfd(request->host, request->port)) > 0)
        {
            request_writen(fd, buf, n);

            /* for headers and body */
            *response = Malloc(MAX_OBJECT_SIZE + MAX_OBJECT_SIZE);
            char content_type[MAXLINE];

            int rc = read_headers(fd, *response, MAX_OBJECT_SIZE + MAX_OBJECT_SIZE);
            *len = rc;

            if (rc > 0)
            {
                int rest_size = parse_response(*response, content_type);

                if (rest_size > 0)
                {
                    rc = request_readn(fd, (char *)*response + rc, rest_size);
                    *len += rc;
                    rest_size -= rc;

                    if (rc < 0)
                    {
                        status = BAD_GATEWAY;
                    }
                }
                else if (rest_size < 0)
                {
                    status = BAD_GATEWAY;
                }

                if (rest_size == 0)
                {
                    write_cache(cache_pool, request->host, request->port, request->uri, content_type, *response);
                }
            }
            else
            {
                status = BAD_GATEWAY;
            }

            Close(fd);
        }
        else
        {
            status = BAD_GATEWAY;
        }
    }
    else
    {
        status = INTERNAL_SERVER_ERROR;
    }

    Free(buf);

    return status;
}

/*
 * clienterror - returns an error message to the client
 */
static void clienterror(int fd, char *cause, int errnum, char *longmsg)
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %d %s\r\n", errnum, get_status_str(errnum));
    request_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    request_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Proxy Error</title>");
    request_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=ffffff>\r\n");
    request_writen(fd, buf, strlen(buf));
    sprintf(buf, "%d: %s\r\n", errnum, get_status_str(errnum));
    request_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    request_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Proxy</em>\r\n");
    request_writen(fd, buf, strlen(buf));
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