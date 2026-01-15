/*
  +----------------------------------------------------------------------+
  | Signalforge HTTP Extension - Request Data Structure                  |
  +----------------------------------------------------------------------+
  | Copyright (c) 2026 Signalforge                                       |
  +----------------------------------------------------------------------+
  | This source file is subject to MIT license.                          |
  +----------------------------------------------------------------------+
*/

#ifndef SIGNALFORGE_CLIENT_REQUEST_DATA_H
#define SIGNALFORGE_CLIENT_REQUEST_DATA_H

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT

#include <stddef.h>
#include "client.h"

/* Request data structure (thread-safe, fully copied) */
struct _signalforge_client_request {
    char *method;
    char *url;
    signalforge_client_header_t *headers;
    size_t header_count;
    char *body;
    size_t body_len;
    void *config;  /* Points to config, not owned */
    void *user_data;  /* Optional user data for callbacks */
    int request_id;   /* Unique request ID for tracking */
};

/* Function declarations */
signalforge_client_request_t *signalforge_client_request_create(
    const char *method,
    const char *url,
    zval *headers,
    const char *body,
    size_t body_len,
    signalforge_client_config_t *config
);
void signalforge_client_request_destroy(signalforge_client_request_t *request);

#endif /* HAVE_SIGNALFORGE_HTTP_CLIENT */

#endif /* SIGNALFORGE_CLIENT_REQUEST_DATA_H */
