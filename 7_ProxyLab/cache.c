#include "csapp.h"
#include "cache.h"

static char *build_cache_obj(const cache_t *cache)
{
    char buffer[MAXLINE + MAXLINE + MAXLINE];
    size_t body_size = strlen(cache->content);

    sprintf(buffer, "HTTP/1.0 200 OK\r\nServer: %sContent-length: %ld\r\n%s\r\n", cache->host, cache->size, cache->content_type);
    size_t header_size = strlen(buffer);

    char *obj = (char *)Malloc(header_size + body_size + 1);
    memcpy(obj, buffer, header_size);
    memcpy(obj + header_size, cache->content, body_size);
    obj[header_size + body_size] = '\0';
    return obj;
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
char *read_cache(cache_pool_t *pool, const char *host, const char *port, const char *uri)
{
    if (!pool || !host || !port || !uri)
    {
        return NULL;
    }

    P(&pool->mutex);
    ++pool->reader_cnt;

    if (pool->reader_cnt > 0)
    {
        P(&pool->write_mutex);
    }
    V(&pool->mutex);

    /* search cache */
    char *obj = NULL;

    for (cache_t *p = pool->dummy->next; p != pool->dummy; p = p->next)
    {
        if (strcmp(host, p->host) != 0)
        {
            continue;
        }

        if (strcmp(port, p->port) != 0)
        {
            continue;
        }

        if (strcmp(uri, p->uri) != 0)
        {
            continue;
        }

        obj = build_cache_obj(p);
        break;
    }

    P(&pool->mutex);
    --pool->reader_cnt;

    if (pool->reader_cnt == 0)
    {
        V(&pool->write_mutex);
    }
    V(&pool->mutex);

    return obj;
}

void write_cache(cache_pool_t *pool, const char *host, const char *port, const char *uri, const char *content_type, const char *content)
{
    if (!pool || !host || !port || !uri || !content)
    {
        return;
    }

    size_t size = strlen(content);

    if (size > pool->max_obj)
    {
        return;
    }

    char *new_content = Malloc(size + 1);
    strncpy(new_content, content, size + 1);

    cache_t *new_cache = Malloc(sizeof(cache_t));
    strncpy(new_cache->host, host, MAXLINE);
    strncpy(new_cache->port, port, MAXLINE);
    strncpy(new_cache->uri, uri, MAXLINE);
    strncpy(new_cache->content_type, content_type, MAXLINE);
    new_cache->content = new_content;
    new_cache->size = size;

    P(&pool->write_mutex);

    /* delete old cache if exists */
    for (cache_t *p = pool->dummy->next; p != pool->dummy; p = p->next)
    {
        if (strcmp(p->host, host) != 0)
        {
            continue;
        }
        if (strcmp(p->port, port) != 0)
        {
            continue;
        }
        if (strcmp(p->uri, uri) != 0)
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
    while (size + pool->total_size > pool->max_size)
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

    printf("Wrote to cache.\n");

    V(&pool->write_mutex);
}
