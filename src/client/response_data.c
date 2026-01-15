/*
  +----------------------------------------------------------------------+
  | Signalforge HTTP Extension - Response Data Implementation            |
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
#include "response_data.h"
#include "../response.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* External class entry from signalforge_http */
extern zend_class_entry *signalforge_response_ce;

/**
 * Create a new response data structure
 */
signalforge_client_response_t *signalforge_client_response_create(void) {
    return calloc(1, sizeof(signalforge_client_response_t));
}

/**
 * Destroy response data structure
 */
void signalforge_client_response_destroy(signalforge_client_response_t *response) {
    if (!response) {
        return;
    }

    free(response->body);
    free(response->error_message);

    if (response->headers) {
        for (size_t i = 0; i < response->header_count; i++) {
            free(response->headers[i].name);
            free(response->headers[i].value);
        }
        free(response->headers);
    }

    free(response);
}

/**
 * Create a PSR-7 ResponseInterface from response data
 *
 * Directly initializes the Response internal structure instead of
 * calling a constructor (Response class has no PHP constructor).
 */
zval signalforge_client_create_psr7_response(signalforge_client_response_t *response) {
    zval response_obj;

    if (!response) {
        ZVAL_NULL(&response_obj);
        return response_obj;
    }

    /* Check if this is an error response */
    if (response->is_error) {
        /* Return array for error - caller will throw exception */
        array_init(&response_obj);
        add_assoc_bool(&response_obj, "error", 1);
        add_assoc_long(&response_obj, "curl_code", response->curl_code);

        if (response->error_message) {
            add_assoc_string(&response_obj, "error_message", response->error_message);
        }

        return response_obj;
    }

    /* Create Response object */
    if (object_init_ex(&response_obj, signalforge_response_ce) != SUCCESS) {
        /* Fallback to array */
        array_init(&response_obj);
        add_assoc_long(&response_obj, "status_code", response->http_code);

        if (response->body && response->body_len > 0) {
            add_assoc_stringl(&response_obj, "body", response->body, response->body_len);
        }

        return response_obj;
    }

    /* Get internal structure and initialize directly */
    signalforge_response_object *intern = signalforge_response_from_obj(Z_OBJ(response_obj));

    /* Set status code */
    intern->status_code = response->http_code;

    /* Set reason phrase if not default */
    if (intern->reason_phrase) {
        zend_string_release(intern->reason_phrase);
        intern->reason_phrase = NULL;
    }

    /* Initialize headers hash table */
    if (!intern->ht_headers) {
        ALLOC_HASHTABLE(intern->ht_headers);
        zend_hash_init(intern->ht_headers, response->header_count * 2, NULL, ZVAL_PTR_DTOR, 0);
    }

    /* Add headers - PSR-7 requires headers to be arrays of values */
    for (size_t i = 0; i < response->header_count; i++) {
        if (!response->headers[i].name || !response->headers[i].value) {
            continue;
        }

        /* Normalize header name to lowercase */
        char *lower_name = strdup(response->headers[i].name);
        if (!lower_name) continue;
        for (char *p = lower_name; *p; p++) {
            *p = tolower((unsigned char)*p);
        }

        /* Check if header already exists */
        zval *existing = zend_hash_str_find(intern->ht_headers, lower_name, strlen(lower_name));
        if (existing && Z_TYPE_P(existing) == IS_ARRAY) {
            /* Add value to existing array */
            add_next_index_string(existing, response->headers[i].value);
        } else {
            /* Create new array with this value */
            zval header_array;
            array_init(&header_array);
            add_next_index_string(&header_array, response->headers[i].value);
            zend_hash_str_add(intern->ht_headers, lower_name, strlen(lower_name), &header_array);
        }

        free(lower_name);
    }

    /* Set body as string (will be wrapped in Stream when getBody() is called) */
    if (response->body && response->body_len > 0) {
        ZVAL_STRINGL(&intern->zv_body, response->body, response->body_len);
    } else {
        ZVAL_EMPTY_STRING(&intern->zv_body);
    }
    intern->body_is_stream = 0;

    /* Set protocol version */
    if (!intern->protocol_version) {
        intern->protocol_version = zend_string_init("1.1", 3, 0);
    }

    return response_obj;
}

/**
 * Convert response to PSR-7 array representation (for debugging/fallback)
 */
zval signalforge_client_response_to_psr7(signalforge_client_response_t *response) {
    zval psr7_response;

    array_init(&psr7_response);

    if (response->is_error) {
        add_assoc_bool(&psr7_response, "error", 1);
        add_assoc_long(&psr7_response, "curl_code", response->curl_code);
        if (response->error_message) {
            add_assoc_string(&psr7_response, "error_message", response->error_message);
        }
    } else {
        add_assoc_bool(&psr7_response, "error", 0);
        add_assoc_long(&psr7_response, "status_code", response->http_code);

        if (response->body && response->body_len > 0) {
            add_assoc_stringl(&psr7_response, "body", response->body, response->body_len);
        } else {
            add_assoc_string(&psr7_response, "body", "");
        }

        /* Add headers */
        zval headers_array;
        array_init(&headers_array);

        for (size_t i = 0; i < response->header_count; i++) {
            add_assoc_string(&headers_array, response->headers[i].name, response->headers[i].value);
        }

        add_assoc_zval(&psr7_response, "headers", &headers_array);

        /* Add timing information */
        add_assoc_double(&psr7_response, "total_time", response->total_time);
        add_assoc_double(&psr7_response, "namelookup_time", response->namelookup_time);
        add_assoc_double(&psr7_response, "connect_time", response->connect_time);
        add_assoc_double(&psr7_response, "pretransfer_time", response->pretransfer_time);
        add_assoc_double(&psr7_response, "starttransfer_time", response->starttransfer_time);
    }

    add_assoc_long(&psr7_response, "request_id", response->request_id);

    return psr7_response;
}

#endif /* HAVE_SIGNALFORGE_HTTP_CLIENT */
