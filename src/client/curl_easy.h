/*
  +----------------------------------------------------------------------+
  | Signalforge HTTP Extension - Synchronous CURL Execution              |
  +----------------------------------------------------------------------+
  | Copyright (c) 2026 Signalforge                                       |
  +----------------------------------------------------------------------+
  | This source file is subject to MIT license.                          |
  +----------------------------------------------------------------------+
*/

#ifndef SIGNALFORGE_CLIENT_CURL_EASY_H
#define SIGNALFORGE_CLIENT_CURL_EASY_H

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT

#include "client.h"
#include <curl/curl.h>

/* Request Body Read Context (Zero-Copy Optimization) */
typedef struct _signalforge_body_read_context {
    const char *data;      /* BORROWED: pointer to request body data */
    size_t data_len;       /* Total length of body data */
    size_t bytes_read;     /* Current read position (bytes already sent) */
} signalforge_body_read_context_t;

/* Execution Context Structure */
typedef struct _signalforge_curl_context {
    /* CURL handle */
    CURL *curl_handle;

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

} signalforge_curl_context_t;

/**
 * Setup curl handle with request data
 *
 * @param curl CURL handle to configure
 * @param ctx Execution context
 * @param share CURLSH shared handle (can be NULL)
 * @param config Client configuration
 * @return 0 on success, -1 on failure
 */
int signalforge_curl_setup(
    CURL *curl,
    signalforge_curl_context_t *ctx,
    CURLSH *share,
    signalforge_client_config_t *config
);

/**
 * Process curl response after execution
 *
 * @param curl CURL handle that completed
 * @param ctx Execution context
 */
void signalforge_curl_process_response(CURL *curl, signalforge_curl_context_t *ctx);

/**
 * Parse response headers into response structure
 *
 * @param ctx Execution context with accumulated headers
 */
void signalforge_curl_parse_headers(signalforge_curl_context_t *ctx);

/**
 * Cleanup execution context resources
 *
 * @param ctx Execution context to cleanup
 */
void signalforge_curl_cleanup_context(signalforge_curl_context_t *ctx);

/**
 * Reset context for retry (preserves request data, clears response accumulator)
 *
 * @param ctx Execution context to reset
 */
void signalforge_curl_reset_context(signalforge_curl_context_t *ctx);

/**
 * Execute HTTP request synchronously
 *
 * @param request Request data to execute
 * @param share CURLSH shared handle for connection pooling (can be NULL)
 * @param config Client configuration
 * @return Response data (caller owns), or NULL on failure
 */
signalforge_client_response_t *signalforge_curl_easy_execute(
    signalforge_client_request_t *request,
    signalforge_client_share_t *share,
    signalforge_client_config_t *config
);

#endif /* HAVE_SIGNALFORGE_HTTP_CLIENT */

#endif /* SIGNALFORGE_CLIENT_CURL_EASY_H */
