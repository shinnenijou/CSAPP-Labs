#include <stdio.h>
#include <bits/types/struct_timeval.h>

#include "csapp.h"
#include "requests.h"
#include "queue.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* Work threads number */
#define WORK_THREADS_NUM 4
#define QUEUE_MAX_LEN 1024

/* terminate flag. server should wait for service completing after recerive signal to terminate */
volatile int terminate = 0;

static void *thread(void *arg);
static void serve(int fd);
static int do_request(Request *request, char *response, size_t *len);
static int do_get(Request *request, char *response, size_t *len);
static void clienterror(int fd, char *cause, int errnum, char *longmsg);
static void sig_terminate(int sig);

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
    Signal(SIGINT, sig_terminate);  /* ctrl-c */
    Signal(SIGTSTP, sig_terminate); /* ctrl-z */

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

    return 0;
}

static void *thread(void *arg)
{
    queue_t *queue = (queue_t *)arg;
    int fd;

    while ((fd = queue_get(queue)) > 0)
    {
        printf("serve %d by thread %ld\n", fd, Pthread_self());
        serve(fd);
        Close(fd);
    }

    return NULL;
}

/* TODO deal with timeout */
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
        clienterror(fd, buffer, BAD_REQUEST, "Cannot parse this request.");
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
            clienterror(fd, buffer, BAD_REQUEST, "Illegal header");
            release_request(request);
            return;
        }
    }

    char *response = Malloc(MAX_OBJECT_SIZE);
    size_t size = 0;
    int status = do_request(request, response, &size);

    if (status != OK)
    {
        clienterror(fd, "", status, "Proxy met something wrong.");
    }
    else
    {
        Rio_writen(fd, response, size);
    }

    Free(response);
    release_request(request);
}

/* do_request - do the proxy request then return the response from end server */
static int do_request(Request *request, char *response, size_t *len)
{
    if (strcmp(request->method, "GET") == 0)
    {
        return do_get(request, response, len);
    }

    return NOT_IMPLEMENTED;
}

static int do_get(Request *request, char *response, size_t *len)
{
    char *buf = (char *)Malloc(MAX_OBJECT_SIZE);
    size_t n;

    if ((n = make_request_string(request, buf)) == 0)
    {
        Free(buf);
        return INTERNAL_SERVER_ERROR;
    }

    int fd;
    if ((fd = open_clientfd(request->host, request->port)) < 0)
    {
        return BAD_GATEWAY;
    }

    Rio_writen(fd, buf, n);

    rio_t rio;
    Rio_readinitb(&rio, fd);

    size_t size = 0;

    while ((n = Rio_readnb(&rio, response, MAX_OBJECT_SIZE)))
    {
        size += n;
    }

    *len = size;
    Close(fd);
    Free(buf);

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
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Proxy Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=ffffff>\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%d: %s\r\n", errnum, get_status_str(errnum));
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Proxy</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}

static void sig_terminate(int sig)
{
    sigset_t set, old_set;
    Sigfillset(&set);
    Sigprocmask(SIG_BLOCK, &set, &old_set);

    int old_errno = errno;
    char buffer[MAXLINE];
    sprintf(buffer, "Received signal %d(%s), server will exit...\n", sig, strsignal(sig));
    write(STDOUT_FILENO, buffer, strlen(buffer));
    terminate = 1;
    errno = old_errno;

    Sigprocmask(SIG_SETMASK, &old_set, NULL);
}
