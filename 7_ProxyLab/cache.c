#include "csapp.h"
#include "cache.h"
#include "requests.h"

static int build_cache_obj(const cache_t *cache, char **buf)
{
    char buffer[MAXLINE + MAXLINE + MAXLINE];

    sprintf(buffer, "HTTP/1.0 200 OK\r\nServer: %s\r\nContent-length: %ld\r\n%s\r\n\r\n", cache->host, cache->size, cache->content_type);
    size_t header_size = strlen(buffer);

    char *obj = (char *)Malloc(header_size + cache->size);
    memcpy(obj, buffer, header_size);
    memcpy(obj + header_size, cache->content, cache->size);

    *buf = obj;
    return header_size + cache->size;
}

cache_pool_t *create_cache_pool(size_t max_size, size_t max_obj)
{
    cache_pool_t *pool = (cache_pool_t *)Malloc(sizeof(cache_pool_t));

    pool->max_size = max_size;
    pool->max_obj = max_obj;
    pool->total_size = 0;
    pool->reader_cnt = 0;

    Sem_init(&pool->mutex, 0, 1);
    Sem_init(&pool->write_mutex, 0, 1);
    pool->dummy = (cache_t *)Malloc(sizeof(cache_t));
    memset(pool->dummy, 0, sizeof(cache_t));
    pool->dummy->prev = pool->dummy;
    pool->dummy->next = pool->dummy;

    return pool;
}

void release_cache_pool(cache_pool_t *pool)
{
    if (pool == NULL)
    {
        return;
    }

    cache_t *p = pool->dummy->next;

    while (p != pool->dummy)
    {
        cache_t *next = p->next;
        Free(p->content);
        Free(p);
        p = next;
    }

    Free(pool->dummy);
    Free(pool);
}

/*
 * read_cache - use cached object content to build a complete HTTP response
 * user responsibility to free memory
 */
int read_cache(cache_pool_t *pool, Request *request, char **buf)
{
    if (!pool || !request || !buf)
    {
        return -1;
    }

    P(&pool->mutex);
    ++pool->reader_cnt;

    if (pool->reader_cnt > 0)
    {
        P(&pool->write_mutex);
    }
    V(&pool->mutex);

    /* search cache */
    int size = -1;

    for (cache_t *p = pool->dummy->next; p != pool->dummy; p = p->next)
    {
        if (strcmp(request->host, p->host) != 0)
        {
            continue;
        }

        if (strcmp(request->port, p->port) != 0)
        {
            continue;
        }

        if (strcmp(request->uri, p->uri) != 0)
        {
            continue;
        }

        size = build_cache_obj(p, buf);
        break;
    }

    P(&pool->mutex);
    --pool->reader_cnt;

    if (pool->reader_cnt == 0)
    {
        V(&pool->write_mutex);
    }
    V(&pool->mutex);

    return size;
}

void write_cache(cache_pool_t *pool, Request *request, Response *response)
{
    if (!pool || !request || !response)
    {
        return;
    }

    if (response->content_length > pool->max_obj)
    {
        return;
    }

    char *new_content = Malloc(response->content_length);
    memcpy(new_content, response->content, response->content_length);

    cache_t *new_cache = Malloc(sizeof(cache_t));
    strncpy(new_cache->host, request->host, MAXLINE);
    strncpy(new_cache->port, request->port, MAXLINE);
    strncpy(new_cache->uri, request->uri, MAXLINE);
    strncpy(new_cache->content_type, response->content_type, MAXLINE);
    new_cache->content = new_content;
    new_cache->size = response->content_length;

    P(&pool->write_mutex);

    /* delete old cache if exists */
    for (cache_t *p = pool->dummy->next; p != pool->dummy; p = p->next)
    {
        if (strcmp(p->host, request->host) != 0)
        {
            continue;
        }
        if (strcmp(p->port, request->port) != 0)
        {
            continue;
        }
        if (strcmp(p->uri, request->uri) != 0)
        {
            continue;
        }

        pool->total_size -= p->size;
        p->prev->next = p->next;
        p->next->prev = p->prev;
        Free(p->content);
        Free(p);
        break;
    }

    /* delete old cache if size exceeds limit */
    while (response->content_length + pool->total_size > pool->max_size)
    {
        cache_t *last = pool->dummy->prev;
        pool->total_size -= last->size;
        last->prev->next = last->next;
        last->next->prev = last->prev;
        Free(last->content);
        Free(last);
    }

    cache_t *head = pool->dummy->next;
    head->prev = new_cache;
    pool->dummy->next = new_cache;
    new_cache->next = head;
    new_cache->prev = pool->dummy;

    V(&pool->write_mutex);
}
