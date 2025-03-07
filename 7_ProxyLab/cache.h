#ifndef __CACHE_H__
#define __CACHE_H__

#include "csapp.h"
#include "requests.h"

typedef struct _cache
{
    char host[MAXLINE];
    char port[MAXLINE];
    char uri[MAXLINE];
    char content_type[MAXLINE];
    char status[MAXLINE];
    int status_code;
    char *content;
    size_t content_length;
    struct _cache *prev;
    struct _cache *next;
} cache_t;

typedef struct
{
    cache_t *dummy;
    size_t total_size;
    size_t max_size;
    size_t max_obj;
    size_t reader_cnt;
    sem_t mutex;
    sem_t write_mutex;
} cache_pool_t;

cache_pool_t *create_cache_pool(size_t max_size, size_t max_obj);
void release_cache_pool(cache_pool_t *pool);
int read_cache(cache_pool_t *pool, Request *request, char **buf);
void write_cache(cache_pool_t *pool, Request *request, Response *response);

#endif