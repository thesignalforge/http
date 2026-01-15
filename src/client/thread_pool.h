/*
  +----------------------------------------------------------------------+
  | Signalforge HTTP Extension - Thread Pool                             |
  +----------------------------------------------------------------------+
  | Copyright (c) 2026 Signalforge                                       |
  +----------------------------------------------------------------------+
  | This source file is subject to MIT license.                          |
  +----------------------------------------------------------------------+
*/

#ifndef SIGNALFORGE_CLIENT_THREAD_POOL_H
#define SIGNALFORGE_CLIENT_THREAD_POOL_H

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT

#include "client.h"
#include <pthread.h>

/* Worker thread structure */
struct _signalforge_client_worker {
    pthread_t thread;
    int worker_id;
    signalforge_client_pool_t *pool;
    signalforge_client_queue_t *request_queue;
    CURLM *multi_handle;
    int running;
    int shutdown;
};

/* Thread pool structure */
struct _signalforge_client_pool {
    signalforge_client_worker_t *workers;
    int worker_count;
    signalforge_client_queue_t *response_queue;
    signalforge_client_share_t *share;
    signalforge_client_config_t *config;
    int shutdown;
    int next_worker;
    pthread_mutex_t dispatch_mutex;
};

/* Function declarations */
signalforge_client_pool_t *signalforge_client_pool_create(signalforge_client_config_t *config);
void signalforge_client_pool_destroy(signalforge_client_pool_t *pool);
int signalforge_client_pool_submit(signalforge_client_pool_t *pool, signalforge_client_request_t *request);
signalforge_client_response_t *signalforge_client_pool_wait_response(signalforge_client_pool_t *pool, int timeout_ms);
void signalforge_client_pool_shutdown(signalforge_client_pool_t *pool);

#endif /* HAVE_SIGNALFORGE_HTTP_CLIENT */

#endif /* SIGNALFORGE_CLIENT_THREAD_POOL_H */
