/*
  +----------------------------------------------------------------------+
  | Signalforge HTTP Extension - Request Data Implementation             |
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
#include "request_data.h"
#include <stdlib.h>
#include <string.h>

static int request_id_counter = 0;

/**
 * Create a new request data structure (deep copy for thread safety)
 */
signalforge_client_request_t *signalforge_client_request_create(
    const char *method,
    const char *url,
    zval *headers,
    const char *body,
    size_t body_len,
    signalforge_client_config_t *config
) {
    /* Use calloc for zero-initialization */
    signalforge_client_request_t *request = calloc(1, sizeof(signalforge_client_request_t));
    if (!request) {
        return NULL;
    }

    /* Assign unique request ID */
    request->request_id = __sync_fetch_and_add(&request_id_counter, 1);

    /* Copy method */
    if (method) {
        request->method = strdup(method);
        if (!request->method) {
            free(request);
            return NULL;
        }
    }

    /* Copy URL */
    if (url) {
        request->url = strdup(url);
        if (!request->url) {
            free(request->method);
            free(request);
            return NULL;
        }
    }

    /* Copy headers - PSR-7 getHeaders() returns array<string, array<string>> */
    if (headers && Z_TYPE_P(headers) == IS_ARRAY) {
        HashTable *ht = Z_ARRVAL_P(headers);
        request->header_count = zend_hash_num_elements(ht);

        if (request->header_count > 0) {
            request->headers = malloc(sizeof(signalforge_client_header_t) * request->header_count);
            if (!request->headers) {
                free(request->url);
                free(request->method);
                free(request);
                return NULL;
            }

            size_t idx = 0;
            zend_string *key;
            zval *val;

            ZEND_HASH_FOREACH_STR_KEY_VAL(ht, key, val) {
                if (key) {
                    char *header_value = NULL;

                    if (Z_TYPE_P(val) == IS_STRING) {
                        /* Simple string value */
                        header_value = strdup(Z_STRVAL_P(val));
                    } else if (Z_TYPE_P(val) == IS_ARRAY) {
                        /* PSR-7 format: array of values - join with ", " */
                        HashTable *vals = Z_ARRVAL_P(val);
                        size_t total_len = 0;

                        /* First pass: calculate total length */
                        zval *entry;
                        ZEND_HASH_FOREACH_VAL(vals, entry) {
                            if (Z_TYPE_P(entry) == IS_STRING) {
                                total_len += Z_STRLEN_P(entry);
                                if (total_len > 0) total_len += 2; /* ", " separator */
                            }
                        } ZEND_HASH_FOREACH_END();

                        if (total_len > 0) {
                            header_value = malloc(total_len + 1);
                            if (header_value) {
                                header_value[0] = '\0';
                                size_t pos = 0;
                                size_t entry_idx = 0;

                                ZEND_HASH_FOREACH_VAL(vals, entry) {
                                    if (Z_TYPE_P(entry) == IS_STRING) {
                                        if (entry_idx > 0 && pos > 0) {
                                            memcpy(header_value + pos, ", ", 2);
                                            pos += 2;
                                        }
                                        memcpy(header_value + pos, Z_STRVAL_P(entry), Z_STRLEN_P(entry));
                                        pos += Z_STRLEN_P(entry);
                                        entry_idx++;
                                    }
                                } ZEND_HASH_FOREACH_END();

                                header_value[pos] = '\0';
                            }
                        }
                    }

                    if (header_value) {
                        request->headers[idx].name = strdup(ZSTR_VAL(key));
                        request->headers[idx].value = header_value;

                        if (!request->headers[idx].name) {
                            free(header_value);
                            /* Cleanup on error */
                            for (size_t i = 0; i < idx; i++) {
                                free(request->headers[i].name);
                                free(request->headers[i].value);
                            }
                            free(request->headers);
                            free(request->url);
                            free(request->method);
                            free(request);
                            return NULL;
                        }
                        idx++;
                    }
                }
            } ZEND_HASH_FOREACH_END();

            request->header_count = idx;
        }
    }

    /* Copy body */
    if (body && body_len > 0) {
        request->body = malloc(body_len);
        if (!request->body) {
            for (size_t i = 0; i < request->header_count; i++) {
                free(request->headers[i].name);
                free(request->headers[i].value);
            }
            free(request->headers);
            free(request->url);
            free(request->method);
            free(request);
            return NULL;
        }
        memcpy(request->body, body, body_len);
        request->body_len = body_len;
    }

    /* Store config reference (not owned) */
    request->config = config;

    return request;
}

/**
 * Destroy request data structure
 */
void signalforge_client_request_destroy(signalforge_client_request_t *request) {
    if (!request) {
        return;
    }

    free(request->method);
    free(request->url);

    if (request->headers) {
        for (size_t i = 0; i < request->header_count; i++) {
            free(request->headers[i].name);
            free(request->headers[i].value);
        }
        free(request->headers);
    }

    free(request->body);
    free(request);
}

#endif /* HAVE_SIGNALFORGE_HTTP_CLIENT */
