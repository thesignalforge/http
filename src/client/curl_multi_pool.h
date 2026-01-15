/*
  +----------------------------------------------------------------------+
  | Signalforge HTTP Extension - Event-Driven Concurrent CURL Execution  |
  +----------------------------------------------------------------------+
  | Copyright (c) 2026 Signalforge                                       |
  +----------------------------------------------------------------------+
  | This source file is subject to MIT license.                          |
  +----------------------------------------------------------------------+
*/

#ifndef SIGNALFORGE_CLIENT_CURL_MULTI_POOL_H
#define SIGNALFORGE_CLIENT_CURL_MULTI_POOL_H

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT

#include "client.h"
#include "curl_easy.h"
#include <curl/curl.h>

/* Maximum concurrent connections in a single pool */
#define SIGNALFORGE_CURL_MULTI_MAX_CONNECTIONS 100

/* Tracking structure for active requests in curl_multi */
typedef struct _signalforge_multi_active_request {
    CURL *curl_handle;
    signalforge_curl_context_t ctx;
    struct _signalforge_multi_active_request *next;
} signalforge_multi_active_request_t;

/* curl_multi pool structure */
typedef struct _signalforge_curl_multi_pool {
    CURLM *multi_handle;
    CURLSH *share_handle;
    signalforge_client_config_t *config;

    /* Active request tracking via linked list */
    signalforge_multi_active_request_t *active_head;
    int active_count;

    /* Completed responses */
    signalforge_client_response_t **responses;
    size_t response_count;
    size_t response_capacity;

    /* Concurrency limit */
    int max_concurrent;

} signalforge_curl_multi_pool_t;

/**
 * Create a new curl_multi pool
 *
 * @param share CURLSH handle for connection sharing (can be NULL)
 * @param config Client configuration
 * @param max_concurrent Maximum concurrent connections (0 = unlimited)
 * @return New pool, or NULL on failure
 */
signalforge_curl_multi_pool_t *signalforge_curl_multi_pool_create(
    signalforge_client_share_t *share,
    signalforge_client_config_t *config,
    int max_concurrent
);

/**
 * Add a request to the pool
 *
 * @param pool The multi pool
 * @param request Request data (pool takes ownership)
 * @return 0 on success, -1 on failure
 */
int signalforge_curl_multi_pool_add(
    signalforge_curl_multi_pool_t *pool,
    signalforge_client_request_t *request
);

/**
 * Execute all requests and collect responses
 *
 * This function blocks until all added requests complete.
 * Uses event-driven I/O for efficient concurrent execution.
 *
 * @param pool The multi pool
 * @param timeout_ms Maximum time to wait (0 = use config timeout)
 * @return Number of completed responses, or -1 on error
 */
int signalforge_curl_multi_pool_execute(
    signalforge_curl_multi_pool_t *pool,
    int timeout_ms
);

/**
 * Get completed responses
 *
 * @param pool The multi pool
 * @param count Output: number of responses
 * @return Array of response pointers (pool retains ownership)
 */
signalforge_client_response_t **signalforge_curl_multi_pool_get_responses(
    signalforge_curl_multi_pool_t *pool,
    size_t *count
);

/**
 * Destroy the pool and free all resources
 *
 * Note: Does NOT destroy the responses - caller must handle them
 *
 * @param pool Pool to destroy
 */
void signalforge_curl_multi_pool_destroy(signalforge_curl_multi_pool_t *pool);

#endif /* HAVE_SIGNALFORGE_HTTP_CLIENT */

#endif /* SIGNALFORGE_CLIENT_CURL_MULTI_POOL_H */
