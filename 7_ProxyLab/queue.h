#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "csapp.h"

typedef struct
{
    int *fd;
    int capacity;
    int start;
    int end;
    sem_t mutex;
    sem_t items;
    sem_t slots;
} queue_t;

queue_t *queue_create(int capacity);
void queue_free(queue_t *queue);
int queue_get(queue_t *queue);
void queue_put(queue_t *queue, int value);

#endif