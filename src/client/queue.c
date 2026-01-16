/*
  +----------------------------------------------------------------------+
  | Signalforge HTTP Extension - Thread-Safe Queue Implementation        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2026 Signalforge                                       |
  +----------------------------------------------------------------------+
  | This source file is subject to MIT license.                          |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT

#include "php.h"
#include "client.h"
#include "queue.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

/**
 * Create a new thread-safe queue with specified capacity
 */
signalforge_client_queue_t *signalforge_client_queue_create(size_t capacity) {
    signalforge_client_queue_t *queue = malloc(sizeof(signalforge_client_queue_t));
    if (!queue) {
        return NULL;
    }

    queue->items = malloc(sizeof(void *) * capacity);
    if (!queue->items) {
        free(queue);
        return NULL;
    }

    queue->capacity = capacity;
    queue->size = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->shutdown = 0;

    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        free(queue->items);
        free(queue);
        return NULL;
    }

    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        free(queue->items);
        free(queue);
        return NULL;
    }

    if (pthread_cond_init(&queue->not_full, NULL) != 0) {
        pthread_cond_destroy(&queue->not_empty);
        pthread_mutex_destroy(&queue->mutex);
        free(queue->items);
        free(queue);
        return NULL;
    }

    return queue;
}

/**
 * Destroy the queue and free all resources
 */
void signalforge_client_queue_destroy(signalforge_client_queue_t *queue) {
    if (!queue) {
        return;
    }

    pthread_mutex_lock(&queue->mutex);
    queue->shutdown = 1;
    pthread_cond_broadcast(&queue->not_empty);
    pthread_cond_broadcast(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);

    pthread_cond_destroy(&queue->not_full);
    pthread_cond_destroy(&queue->not_empty);
    pthread_mutex_destroy(&queue->mutex);

    free(queue->items);
    free(queue);
}

/**
 * Push an item onto the queue (blocking if full)
 * Returns 0 on success, -1 on error or shutdown
 */
int signalforge_client_queue_push(signalforge_client_queue_t *queue, void *item) {
    if (!queue || !item) {
        return -1;
    }

    pthread_mutex_lock(&queue->mutex);

    /* Wait while queue is full and not shutting down */
    while (queue->size >= queue->capacity && !queue->shutdown) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }

    if (queue->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }

    /* Add item to queue */
    queue->items[queue->tail] = item;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->size++;

    /* Signal that queue is not empty */
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);

    return 0;
}

/**
 * Pop an item from the queue (blocking with optional timeout)
 * timeout_ms: -1 for infinite wait, 0 for non-blocking, >0 for timeout in ms
 * Returns item pointer on success, NULL on timeout/error/shutdown
 */
void *signalforge_client_queue_pop(signalforge_client_queue_t *queue, int timeout_ms) {
    if (!queue) {
        return NULL;
    }

    pthread_mutex_lock(&queue->mutex);

    if (timeout_ms == 0) {
        /* Non-blocking */
        if (queue->size == 0 || queue->shutdown) {
            pthread_mutex_unlock(&queue->mutex);
            return NULL;
        }
    } else if (timeout_ms < 0) {
        /* Infinite wait */
        while (queue->size == 0 && !queue->shutdown) {
            pthread_cond_wait(&queue->not_empty, &queue->mutex);
        }

        if (queue->shutdown && queue->size == 0) {
            pthread_mutex_unlock(&queue->mutex);
            return NULL;
        }
    } else {
        /* Timed wait */
        struct timespec ts;
        struct timeval tv;
        gettimeofday(&tv, NULL);

        ts.tv_sec = tv.tv_sec + (timeout_ms / 1000);
        ts.tv_nsec = (tv.tv_usec * 1000) + ((timeout_ms % 1000) * 1000000);

        /* Handle nanosecond overflow */
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }

        while (queue->size == 0 && !queue->shutdown) {
            int result = pthread_cond_timedwait(&queue->not_empty, &queue->mutex, &ts);
            if (result == ETIMEDOUT) {
                pthread_mutex_unlock(&queue->mutex);
                return NULL;
            }
        }

        if (queue->shutdown && queue->size == 0) {
            pthread_mutex_unlock(&queue->mutex);
            return NULL;
        }
    }

    /* Get item from queue */
    void *item = queue->items[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->size--;

    /* Signal that queue is not full */
    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);

    return item;
}

/**
 * Get current queue size
 */
size_t signalforge_client_queue_size(signalforge_client_queue_t *queue) {
    if (!queue) {
        return 0;
    }

    pthread_mutex_lock(&queue->mutex);
    size_t size = queue->size;
    pthread_mutex_unlock(&queue->mutex);

    return size;
}

#endif /* HAVE_SIGNALFORGE_HTTP_CLIENT */
