#include <pthread.h>
#include <stdio.h>

#ifndef _MSG_QUEUE_H
#define _MSG_QUEUE_H

#define QUEUE_INITIALIZER(buffer) { buffer, sizeof(buffer) / sizeof(buffer[0]), 0, 0, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER }

typedef struct _msgqueue
{
        void **buffer;
        const int capacity;
        int size;
        int in;
        int out;
        pthread_mutex_t mutex;
        pthread_cond_t cond_full;
        pthread_cond_t cond_empty;
} fds_msg_queue_t;

void fds_msgq_enqueue(fds_msg_queue_t *queue, void *value)
{
        pthread_mutex_lock(&(queue->mutex));
        while (queue->size == queue->capacity)
                pthread_cond_wait(&(queue->cond_full), &(queue->mutex));
        printf("enqueue %d\n", *(int *)value);
        queue->buffer[queue->in] = value;
        ++ queue->size;
        ++ queue->in;
        queue->in %= queue->capacity;
        pthread_mutex_unlock(&(queue->mutex));
        pthread_cond_broadcast(&(queue->cond_empty));
}

void *fds_msgq_dequeue(fds_msg_queue_t *queue)
{
        pthread_mutex_lock(&(queue->mutex));
        while (queue->size == 0)
                pthread_cond_wait(&(queue->cond_empty), &(queue->mutex));
        void *value = queue->buffer[queue->out];
        printf("dequeue %d\n", *(int *)value);
        -- queue->size;
        ++ queue->out;
        queue->out %= queue->capacity;
        pthread_mutex_unlock(&(queue->mutex));
        pthread_cond_broadcast(&(queue->cond_full));
        return value;
}

int fds_msgq_get_size(fds_msg_queue_t *queue)
{
        pthread_mutex_lock(&(queue->mutex));
        int size = queue->size;
        pthread_mutex_unlock(&(queue->mutex));
        return size;
}

#endif
