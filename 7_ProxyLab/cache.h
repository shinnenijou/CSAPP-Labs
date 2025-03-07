#ifndef __CACHE_H__
#define __CACHE_H__

#include "csapp.h"

typedef struct _cache
{
    char host[MAXLINE];
    char port[MAXLINE];
    char uri[MAXLINE];
    char content_type[MAXLINE];
    char *content;
    size_t size;
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
char *read_cache(cache_pool_t *pool, const char *host, const char *port, const char *uri);
void write_cache(cache_pool_t *pool, const char *host, const char *port, const char *uri, const char *content_type, const char *content);

#endif