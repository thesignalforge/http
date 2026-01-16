/*
  +----------------------------------------------------------------------+
  | Signalforge HTTP Extension - Thread-Safe Queue                       |
  +----------------------------------------------------------------------+
  | Copyright (c) 2026 Signalforge                                       |
  +----------------------------------------------------------------------+
  | This source file is subject to MIT license.                          |
  +----------------------------------------------------------------------+
*/

#ifndef SIGNALFORGE_CLIENT_QUEUE_H
#define SIGNALFORGE_CLIENT_QUEUE_H

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT

#include <pthread.h>
#include <stddef.h>

/* Thread-safe queue structure */
struct _signalforge_client_queue {
    void **items;
    size_t capacity;
    size_t size;
    size_t head;
    size_t tail;
    int shutdown;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
};

/* Function declarations */
signalforge_client_queue_t *signalforge_client_queue_create(size_t capacity);
void signalforge_client_queue_destroy(signalforge_client_queue_t *queue);
int signalforge_client_queue_push(signalforge_client_queue_t *queue, void *item);
void *signalforge_client_queue_pop(signalforge_client_queue_t *queue, int timeout_ms);
size_t signalforge_client_queue_size(signalforge_client_queue_t *queue);

#endif /* HAVE_SIGNALFORGE_HTTP_CLIENT */

#endif /* SIGNALFORGE_CLIENT_QUEUE_H */
