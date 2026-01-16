/*
  +----------------------------------------------------------------------+
  | Signalforge HTTP Extension - Thread Pool Implementation              |
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
#include "thread_pool.h"
#include "curl_worker.h"
#include "queue.h"
#include "share.h"
#include <stdlib.h>
#include <string.h>

/**
 * Create a new thread pool
 */
signalforge_client_pool_t *signalforge_client_pool_create(signalforge_client_config_t *config) {
    if (!config || config->pool_size <= 0) {
        return NULL;
    }

    /* Use calloc for zero-initialization */
    signalforge_client_pool_t *pool = calloc(1, sizeof(signalforge_client_pool_t));
    if (!pool) {
        return NULL;
    }

    pool->config = config;
    pool->worker_count = config->pool_size;
    pool->next_worker = 0;
    pool->shutdown = 0;

    /* Initialize dispatch mutex */
    if (pthread_mutex_init(&pool->dispatch_mutex, NULL) != 0) {
        free(pool);
        return NULL;
    }

    /* Create shared curl handle for connection pooling */
    pool->share = signalforge_client_share_create();
    if (!pool->share) {
        pthread_mutex_destroy(&pool->dispatch_mutex);
        free(pool);
        return NULL;
    }

    /* Create response queue */
    pool->response_queue = signalforge_client_queue_create(SIGNALFORGE_CLIENT_DEFAULT_QUEUE_SIZE);
    if (!pool->response_queue) {
        signalforge_client_share_destroy(pool->share);
        pthread_mutex_destroy(&pool->dispatch_mutex);
        free(pool);
        return NULL;
    }

    /* Allocate worker array using calloc for zero-initialization */
    pool->workers = calloc(pool->worker_count, sizeof(signalforge_client_worker_t));
    if (!pool->workers) {
        signalforge_client_queue_destroy(pool->response_queue);
        signalforge_client_share_destroy(pool->share);
        pthread_mutex_destroy(&pool->dispatch_mutex);
        free(pool);
        return NULL;
    }

    /* Initialize and start worker threads */
    for (int i = 0; i < pool->worker_count; i++) {
        signalforge_client_worker_t *worker = &pool->workers[i];
        worker->worker_id = i;
        worker->pool = pool;
        worker->running = 0;
        worker->shutdown = 0;

        /* Create request queue for this worker */
        worker->request_queue = signalforge_client_queue_create(SIGNALFORGE_CLIENT_DEFAULT_QUEUE_SIZE / pool->worker_count);
        if (!worker->request_queue) {
            /* Cleanup already created workers */
            for (int j = 0; j < i; j++) {
                pool->workers[j].shutdown = 1;
                signalforge_client_queue_destroy(pool->workers[j].request_queue);
            }
            free(pool->workers);
            signalforge_client_queue_destroy(pool->response_queue);
            signalforge_client_share_destroy(pool->share);
            pthread_mutex_destroy(&pool->dispatch_mutex);
            free(pool);
            return NULL;
        }

        /* Start worker thread */
        if (pthread_create(&worker->thread, NULL, signalforge_client_worker_thread, worker) != 0) {
            /* Cleanup */
            worker->shutdown = 1;
            signalforge_client_queue_destroy(worker->request_queue);

            for (int j = 0; j < i; j++) {
                pool->workers[j].shutdown = 1;
                pthread_join(pool->workers[j].thread, NULL);
                signalforge_client_queue_destroy(pool->workers[j].request_queue);
            }

            free(pool->workers);
            signalforge_client_queue_destroy(pool->response_queue);
            signalforge_client_share_destroy(pool->share);
            pthread_mutex_destroy(&pool->dispatch_mutex);
            free(pool);
            return NULL;
        }

        worker->running = 1;
    }

    return pool;
}

/**
 * Submit a request to the thread pool (round-robin dispatch)
 */
int signalforge_client_pool_submit(signalforge_client_pool_t *pool, signalforge_client_request_t *request) {
    if (!pool || !request || pool->shutdown) {
        return -1;
    }

    pthread_mutex_lock(&pool->dispatch_mutex);

    /* Round-robin worker selection */
    int worker_idx = pool->next_worker;
    pool->next_worker = (pool->next_worker + 1) % pool->worker_count;

    pthread_mutex_unlock(&pool->dispatch_mutex);

    /* Submit to worker's request queue */
    return signalforge_client_queue_push(pool->workers[worker_idx].request_queue, request);
}

/**
 * Wait for a response from any worker
 */
signalforge_client_response_t *signalforge_client_pool_wait_response(
    signalforge_client_pool_t *pool,
    int timeout_ms
) {
    if (!pool) {
        return NULL;
    }

    return (signalforge_client_response_t *)signalforge_client_queue_pop(pool->response_queue, timeout_ms);
}

/**
 * Shutdown the thread pool gracefully
 */
void signalforge_client_pool_shutdown(signalforge_client_pool_t *pool) {
    if (!pool || pool->shutdown) {
        return;
    }

    pool->shutdown = 1;

    /* Signal all workers to shutdown */
    for (int i = 0; i < pool->worker_count; i++) {
        pool->workers[i].shutdown = 1;
    }

    /* Wait for all workers to finish */
    for (int i = 0; i < pool->worker_count; i++) {
        if (pool->workers[i].running) {
            pthread_join(pool->workers[i].thread, NULL);
            pool->workers[i].running = 0;
        }
    }
}

/**
 * Destroy the thread pool and free all resources
 */
void signalforge_client_pool_destroy(signalforge_client_pool_t *pool) {
    if (!pool) {
        return;
    }

    /* Ensure shutdown */
    signalforge_client_pool_shutdown(pool);

    /* Destroy worker resources */
    if (pool->workers) {
        for (int i = 0; i < pool->worker_count; i++) {
            if (pool->workers[i].request_queue) {
                signalforge_client_queue_destroy(pool->workers[i].request_queue);
            }
        }
        free(pool->workers);
    }

    /* Destroy shared resources */
    if (pool->response_queue) {
        signalforge_client_queue_destroy(pool->response_queue);
    }

    if (pool->share) {
        signalforge_client_share_destroy(pool->share);
    }

    pthread_mutex_destroy(&pool->dispatch_mutex);

    free(pool);
}

#endif /* HAVE_SIGNALFORGE_HTTP_CLIENT */
