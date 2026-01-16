/*
  +----------------------------------------------------------------------+
  | Signalforge HTTP Extension - CURL Worker Thread                      |
  +----------------------------------------------------------------------+
  | Copyright (c) 2026 Signalforge                                       |
  +----------------------------------------------------------------------+
  | This source file is subject to MIT license.                          |
  +----------------------------------------------------------------------+
*/

#ifndef SIGNALFORGE_CLIENT_CURL_WORKER_H
#define SIGNALFORGE_CLIENT_CURL_WORKER_H

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT

#include "client.h"

/* CURL Handle Pool Configuration */
#define SIGNALFORGE_CURL_HANDLE_POOL_SIZE 10

/* CURL Handle Pool Structure */
typedef struct _signalforge_curl_handle_pool {
    CURL *handles[SIGNALFORGE_CURL_HANDLE_POOL_SIZE];
    int available_count;
    int total_created;
} signalforge_curl_handle_pool_t;

/* Request Body Read Context (Zero-Copy Optimization) */
typedef struct _signalforge_body_read_context {
    const char *data;      /* BORROWED: pointer to request body data */
    size_t data_len;       /* Total length of body data */
    size_t bytes_read;     /* Current read position (bytes already sent) */
} signalforge_body_read_context_t;

/* Active Request Tracking Structure */
typedef struct _signalforge_active_request {
    /* CURL handle management */
    CURL *curl_handle;
    int handle_from_pool;

    /* Request/Response objects */
    signalforge_client_request_t *request;
    signalforge_client_response_t *response;

    /* CURL header list */
    struct curl_slist *header_list;

    /* Response body accumulator */
    char *response_body;
    size_t response_body_len;
    size_t response_body_capacity;

    /* Response headers accumulator */
    char *response_headers;
    size_t response_headers_len;
    size_t response_headers_capacity;

    /* Request body read context (zero-copy optimization) */
    signalforge_body_read_context_t body_read_ctx;

    /* Retry tracking */
    int retry_count;
} signalforge_active_request_t;

/* Handle Pool API */
signalforge_curl_handle_pool_t *signalforge_curl_handle_pool_create(void);
CURL *signalforge_curl_handle_pool_acquire(signalforge_curl_handle_pool_t *pool, int *from_pool);
void signalforge_curl_handle_pool_release(signalforge_curl_handle_pool_t *pool, CURL *handle);
void signalforge_curl_handle_pool_discard(signalforge_curl_handle_pool_t *pool, CURL *handle);
void signalforge_curl_handle_pool_destroy(signalforge_curl_handle_pool_t *pool);

/* Worker thread main function */
void *signalforge_client_worker_thread(void *arg);

#endif /* HAVE_SIGNALFORGE_HTTP_CLIENT */

#endif /* SIGNALFORGE_CLIENT_CURL_WORKER_H */
