#include "csapp.h"
#include "queue.h"

queue_t *queue_create(int capacity)
{
    queue_t *queue = Malloc(sizeof(queue_t));

    Sem_init(&queue->slots, 0, capacity);
    Sem_init(&queue->items, 0, 0);
    Sem_init(&queue->mutex, 0, 1);
    queue->start = 0;
    queue->end = 0;
    queue->capacity = capacity;
    queue->fd = Malloc(capacity * sizeof(int));

    return queue;
}

void queue_free(queue_t *queue)
{
}

int queue_get(queue_t *queue)
{
    int value;

    P(&queue->items);
    P(&queue->mutex);

    value = queue->fd[queue->start];
    queue->start = (queue->start + 1) % queue->capacity;

    V(&queue->mutex);
    V(&queue->slots);

    return value;
}

void queue_put(queue_t *queue, int value)
{
    P(&queue->slots);
    P(&queue->mutex);
    queue->fd[queue->end] = value;
    queue->end = (queue->end + 1) % queue->capacity;
    V(&queue->mutex);
    V(&queue->items);
}