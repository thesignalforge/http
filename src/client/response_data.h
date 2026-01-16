/*
  +----------------------------------------------------------------------+
  | Signalforge HTTP Extension - Response Data Structure                 |
  +----------------------------------------------------------------------+
  | Copyright (c) 2026 Signalforge                                       |
  +----------------------------------------------------------------------+
  | This source file is subject to MIT license.                          |
  +----------------------------------------------------------------------+
*/

#ifndef SIGNALFORGE_CLIENT_RESPONSE_DATA_H
#define SIGNALFORGE_CLIENT_RESPONSE_DATA_H

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT

#include "request_data.h"
#include <curl/curl.h>
#include <stddef.h>

/* Response data structure (thread-safe, fully copied) */
struct _signalforge_client_response {
    int request_id;
    long http_code;
    char *body;
    size_t body_len;
    signalforge_client_header_t *headers;
    size_t header_count;
    double total_time;
    double namelookup_time;
    double connect_time;
    double pretransfer_time;
    double starttransfer_time;
    CURLcode curl_code;
    char *error_message;
    int is_error;
    void *user_data;  /* User data for tracking (e.g., request index) */
};

/* Function declarations */
signalforge_client_response_t *signalforge_client_response_create(void);
void signalforge_client_response_destroy(signalforge_client_response_t *response);
zval signalforge_client_create_psr7_response(signalforge_client_response_t *response);

#endif /* HAVE_SIGNALFORGE_HTTP_CLIENT */

#endif /* SIGNALFORGE_CLIENT_RESPONSE_DATA_H */
