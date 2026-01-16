/*
  +----------------------------------------------------------------------+
  | Signalforge HTTP Extension - Event-Driven Concurrent CURL Execution  |
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
#include "curl_multi_pool.h"
#include "request_data.h"
#include "response_data.h"
#include "retry.h"
#include "share.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Default poll timeout in milliseconds */
#define MULTI_POLL_TIMEOUT_MS 1000

/* Initial response array capacity */
#define RESPONSE_ARRAY_INITIAL_CAPACITY 16

/**
 * Create a new curl_multi pool
 */
signalforge_curl_multi_pool_t *signalforge_curl_multi_pool_create(
    signalforge_client_share_t *share,
    signalforge_client_config_t *config,
    int max_concurrent
) {
    signalforge_curl_multi_pool_t *pool = ecalloc(1, sizeof(signalforge_curl_multi_pool_t));
    if (!pool) {
        return NULL;
    }

    pool->multi_handle = curl_multi_init();
    if (!pool->multi_handle) {
        efree(pool);
        return NULL;
    }

    /* Configure multi handle for optimal concurrent performance */
    curl_multi_setopt(pool->multi_handle, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);
    curl_multi_setopt(pool->multi_handle, CURLMOPT_MAX_HOST_CONNECTIONS, 6L);
    curl_multi_setopt(pool->multi_handle, CURLMOPT_MAXCONNECTS, 100L);

    /* Store shared handle for connection pooling */
    if (share) {
        pool->share_handle = signalforge_client_share_get_handle(share);
    }

    pool->config = config;
    pool->active_head = NULL;
    pool->active_count = 0;

    /* Initialize response array */
    pool->response_capacity = RESPONSE_ARRAY_INITIAL_CAPACITY;
    pool->responses = ecalloc(pool->response_capacity, sizeof(signalforge_client_response_t *));
    if (!pool->responses) {
        curl_multi_cleanup(pool->multi_handle);
        efree(pool);
        return NULL;
    }
    pool->response_count = 0;

    /* Set concurrency limit */
    pool->max_concurrent = max_concurrent > 0 ? max_concurrent : SIGNALFORGE_CURL_MULTI_MAX_CONNECTIONS;
    if (pool->max_concurrent > SIGNALFORGE_CURL_MULTI_MAX_CONNECTIONS) {
        pool->max_concurrent = SIGNALFORGE_CURL_MULTI_MAX_CONNECTIONS;
    }

    return pool;
}

/**
 * Find active request by curl handle
 */
static signalforge_multi_active_request_t *find_active_request(
    signalforge_curl_multi_pool_t *pool,
    CURL *curl_handle
) {
    signalforge_multi_active_request_t *current = pool->active_head;
    while (current) {
        if (current->curl_handle == curl_handle) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/**
 * Remove active request from linked list
 */
static void remove_active_request(
    signalforge_curl_multi_pool_t *pool,
    signalforge_multi_active_request_t *req
) {
    signalforge_multi_active_request_t **current = &pool->active_head;
    while (*current) {
        if (*current == req) {
            *current = req->next;
            pool->active_count--;
            return;
        }
        current = &(*current)->next;
    }
}

/**
 * Add completed response to response array
 */
static int add_response(
    signalforge_curl_multi_pool_t *pool,
    signalforge_client_response_t *response
) {
    if (pool->response_count >= pool->response_capacity) {
        size_t new_capacity = pool->response_capacity * 2;
        signalforge_client_response_t **new_responses = erealloc(
            pool->responses,
            new_capacity * sizeof(signalforge_client_response_t *)
        );
        if (!new_responses) {
            return -1;
        }
        pool->responses = new_responses;
        pool->response_capacity = new_capacity;
    }

    pool->responses[pool->response_count++] = response;
    return 0;
}

/**
 * Add a request to the pool
 */
int signalforge_curl_multi_pool_add(
    signalforge_curl_multi_pool_t *pool,
    signalforge_client_request_t *request
) {
    if (!pool || !request) {
        return -1;
    }

    /* Create active request tracking structure */
    signalforge_multi_active_request_t *active_req = ecalloc(1, sizeof(signalforge_multi_active_request_t));
    if (!active_req) {
        return -1;
    }

    /* Initialize curl context */
    memset(&active_req->ctx, 0, sizeof(signalforge_curl_context_t));
    active_req->ctx.request = request;
    active_req->ctx.response = signalforge_client_response_create();
    if (!active_req->ctx.response) {
        efree(active_req);
        return -1;
    }

    /* Allocate response buffers */
    active_req->ctx.response_body_capacity = 4096;
    active_req->ctx.response_body = emalloc(active_req->ctx.response_body_capacity);
    active_req->ctx.response_headers_capacity = 1024;
    active_req->ctx.response_headers = emalloc(active_req->ctx.response_headers_capacity);

    if (!active_req->ctx.response_body || !active_req->ctx.response_headers) {
        if (active_req->ctx.response_body) efree(active_req->ctx.response_body);
        if (active_req->ctx.response_headers) efree(active_req->ctx.response_headers);
        signalforge_client_response_destroy(active_req->ctx.response);
        efree(active_req);
        return -1;
    }

    /* Create curl handle */
    CURL *curl = curl_easy_init();
    if (!curl) {
        efree(active_req->ctx.response_body);
        efree(active_req->ctx.response_headers);
        signalforge_client_response_destroy(active_req->ctx.response);
        efree(active_req);
        return -1;
    }

    active_req->curl_handle = curl;

    /* Setup curl handle */
    if (signalforge_curl_setup(curl, &active_req->ctx, pool->share_handle, pool->config) != 0) {
        curl_easy_cleanup(curl);
        efree(active_req->ctx.response_body);
        efree(active_req->ctx.response_headers);
        signalforge_client_response_destroy(active_req->ctx.response);
        efree(active_req);
        return -1;
    }

    /* Add to multi handle */
    CURLMcode mc = curl_multi_add_handle(pool->multi_handle, curl);
    if (mc != CURLM_OK) {
        signalforge_curl_cleanup_context(&active_req->ctx);
        curl_easy_cleanup(curl);
        signalforge_client_response_destroy(active_req->ctx.response);
        efree(active_req);
        return -1;
    }

    /* Add to active request linked list */
    active_req->next = pool->active_head;
    pool->active_head = active_req;
    pool->active_count++;

    return 0;
}

/**
 * Process a completed request
 */
static void process_completed(
    signalforge_curl_multi_pool_t *pool,
    CURL *curl_handle,
    CURLcode result
) {
    signalforge_multi_active_request_t *active_req = find_active_request(pool, curl_handle);
    if (!active_req) {
        return;
    }

    signalforge_curl_context_t *ctx = &active_req->ctx;

    if (result == CURLE_OK) {
        /* Success - process response */
        signalforge_curl_process_response(curl_handle, ctx);
    } else {
        /* Error occurred */
        ctx->response->is_error = 1;
        ctx->response->curl_code = result;
        ctx->response->error_message = estrdup(curl_easy_strerror(result));

        /* Still try to get any partial response data */
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &ctx->response->http_code);
    }

    /* Remove from multi handle */
    curl_multi_remove_handle(pool->multi_handle, curl_handle);

    /* Cleanup curl handle */
    curl_easy_cleanup(curl_handle);
    active_req->curl_handle = NULL;

    /* Free header list if present */
    if (ctx->header_list) {
        curl_slist_free_all(ctx->header_list);
        ctx->header_list = NULL;
    }

    /* Free temporary buffers (body/headers already transferred or error) */
    if (ctx->response_body) {
        efree(ctx->response_body);
        ctx->response_body = NULL;
    }
    if (ctx->response_headers) {
        efree(ctx->response_headers);
        ctx->response_headers = NULL;
    }

    /* Add response to results */
    add_response(pool, ctx->response);
    ctx->response = NULL; /* Pool now owns response */

    /* Free request data (pool took ownership) */
    if (ctx->request) {
        signalforge_client_request_destroy(ctx->request);
        ctx->request = NULL;
    }

    /* Remove from active list and free */
    remove_active_request(pool, active_req);
    efree(active_req);
}

/**
 * Execute all requests and collect responses
 */
int signalforge_curl_multi_pool_execute(
    signalforge_curl_multi_pool_t *pool,
    int timeout_ms
) {
    if (!pool) {
        return -1;
    }

    if (pool->active_count == 0) {
        return 0;
    }

    int still_running = 1;
    int poll_timeout = timeout_ms > 0 ? timeout_ms : MULTI_POLL_TIMEOUT_MS;

    /* Main event loop */
    while (still_running > 0) {
        CURLMcode mc;

        /* Perform transfers */
        do {
            mc = curl_multi_perform(pool->multi_handle, &still_running);
        } while (mc == CURLM_CALL_MULTI_PERFORM);

        if (mc != CURLM_OK) {
            /* Multi error - mark remaining requests as failed */
            return -1;
        }

        /* Check for completed transfers */
        int msgs_in_queue;
        CURLMsg *msg;

        while ((msg = curl_multi_info_read(pool->multi_handle, &msgs_in_queue))) {
            if (msg->msg == CURLMSG_DONE) {
                process_completed(pool, msg->easy_handle, msg->data.result);
            }
        }

        /* Wait for I/O if still running */
        if (still_running > 0) {
#ifdef HAVE_CURL_MULTI_POLL
            mc = curl_multi_poll(pool->multi_handle, NULL, 0, poll_timeout, NULL);
#else
            mc = curl_multi_wait(pool->multi_handle, NULL, 0, poll_timeout, NULL);
#endif
            if (mc != CURLM_OK) {
                return -1;
            }
        }
    }

    return (int)pool->response_count;
}

/**
 * Get completed responses
 */
signalforge_client_response_t **signalforge_curl_multi_pool_get_responses(
    signalforge_curl_multi_pool_t *pool,
    size_t *count
) {
    if (!pool || !count) {
        if (count) *count = 0;
        return NULL;
    }

    *count = pool->response_count;
    return pool->responses;
}

/**
 * Destroy the pool and free all resources
 *
 * Note: This does NOT destroy the responses - caller must handle them
 */
void signalforge_curl_multi_pool_destroy(signalforge_curl_multi_pool_t *pool) {
    if (!pool) {
        return;
    }

    /* Cleanup any remaining active requests */
    signalforge_multi_active_request_t *current = pool->active_head;
    while (current) {
        signalforge_multi_active_request_t *next = current->next;

        /* Remove from multi and cleanup */
        if (current->curl_handle) {
            curl_multi_remove_handle(pool->multi_handle, current->curl_handle);
            curl_easy_cleanup(current->curl_handle);
        }

        /* Cleanup context */
        signalforge_curl_cleanup_context(&current->ctx);

        /* Destroy response if not transferred */
        if (current->ctx.response) {
            signalforge_client_response_destroy(current->ctx.response);
        }

        /* Destroy request */
        if (current->ctx.request) {
            signalforge_client_request_destroy(current->ctx.request);
        }

        efree(current);
        current = next;
    }

    /* Cleanup multi handle */
    if (pool->multi_handle) {
        curl_multi_cleanup(pool->multi_handle);
    }

    /* Free response array (but not the responses themselves) */
    efree(pool->responses);

    efree(pool);
}

#endif /* HAVE_SIGNALFORGE_HTTP_CLIENT */
