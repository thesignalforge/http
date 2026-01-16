/*
  +----------------------------------------------------------------------+
  | Signalforge HTTP Extension - PSR-18 HTTP Client                      |
  +----------------------------------------------------------------------+
  | Copyright (c) 2026 Signalforge                                       |
  +----------------------------------------------------------------------+
  | This source file is subject to MIT license.                          |
  +----------------------------------------------------------------------+
*/

#ifndef SIGNALFORGE_HTTP_CLIENT_H
#define SIGNALFORGE_HTTP_CLIENT_H

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT

#include "php.h"
#include <curl/curl.h>

/* Thread pool configuration defaults */
#define SIGNALFORGE_CLIENT_DEFAULT_POOL_SIZE 8
#define SIGNALFORGE_CLIENT_DEFAULT_CONNECT_TIMEOUT 10
#define SIGNALFORGE_CLIENT_DEFAULT_TIMEOUT 30
#define SIGNALFORGE_CLIENT_DEFAULT_MAX_REDIRECTS 5
#define SIGNALFORGE_CLIENT_DEFAULT_MAX_RETRIES 0
#define SIGNALFORGE_CLIENT_DEFAULT_RETRY_DELAY_MS 1000

/* Queue configuration */
#define SIGNALFORGE_CLIENT_DEFAULT_QUEUE_SIZE 1024
#define SIGNALFORGE_CLIENT_QUEUE_BATCH_SIZE 16

/* HTTP version constants */
#define SIGNALFORGE_HTTP_VERSION_AUTO 0
#define SIGNALFORGE_HTTP_VERSION_1_0 1
#define SIGNALFORGE_HTTP_VERSION_1_1 2
#define SIGNALFORGE_HTTP_VERSION_2_0 3
#define SIGNALFORGE_HTTP_VERSION_3_0 4

/* Forward declarations */
typedef struct _signalforge_client_share signalforge_client_share_t;
typedef struct _signalforge_client_request signalforge_client_request_t;
typedef struct _signalforge_client_response signalforge_client_response_t;
typedef struct _signalforge_client_retry_config signalforge_client_retry_config_t;

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT_THREADS
/* Thread pool types - only available in ZTS builds with threading enabled */
typedef struct _signalforge_client_pool signalforge_client_pool_t;
typedef struct _signalforge_client_worker signalforge_client_worker_t;
typedef struct _signalforge_client_queue signalforge_client_queue_t;
#endif

/* Header structure */
typedef struct _signalforge_client_header {
    char *name;
    char *value;
} signalforge_client_header_t;

/* Retry configuration structure */
struct _signalforge_client_retry_config {
    int max_retries;
    int delay_ms;
    int max_delay_ms;
    double backoff_multiplier;
    int retry_on_timeout;
    int retry_on_connect_error;
    int retry_on_5xx;
    int retry_on_429;
};

/* Client configuration structure */
typedef struct _signalforge_client_config {
    int pool_size;
    int connect_timeout;
    int timeout;
    int http_version;
    int max_redirects;
    int follow_redirects;
    int verify_peer;
    int verify_host;
    char *proxy;
    char *user_agent;
    char *ca_cert;
    signalforge_client_retry_config_t *retry_config;
    int debug;
} signalforge_client_config_t;

/* Client object structure */
typedef struct _signalforge_client_object {
    signalforge_client_share_t *share;   /* Shared connection cache */
    signalforge_client_config_t *config;
#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT_THREADS
    signalforge_client_pool_t *thread_pool;  /* Optional thread pool for ZTS */
    int use_threads;                          /* Whether to use thread pool */
#endif
    zend_object std;
} signalforge_client_object_t;

/* Request pool object structure */
typedef struct _signalforge_http_request_pool_object {
    signalforge_client_object_t *client;
    int concurrency;
    HashTable *pending_requests;
    HashTable *success_callbacks;
    HashTable *error_callbacks;
    int request_count;
    int cancelled;
#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT_THREADS
    int use_threads;  /* Whether to use thread pool for this pool */
#endif
    zend_object std;
} signalforge_http_request_pool_object_t;

/* Class entries */
extern zend_class_entry *signalforge_client_ce;
extern zend_class_entry *signalforge_http_request_pool_ce;

/* Exception classes */
extern zend_class_entry *signalforge_http_exception_ce;
extern zend_class_entry *signalforge_network_exception_ce;
extern zend_class_entry *signalforge_request_exception_ce;

/* Object handlers */
extern zend_object_handlers signalforge_client_object_handlers;
extern zend_object_handlers signalforge_http_request_pool_object_handlers;

/* Utility macros */
#define SIGNALFORGE_CLIENT_FROM_OBJECT(type, object) \
    ((type *)((char *)(object) - XtOffsetOf(type, std)))

#define SIGNALFORGE_CLIENT_FROM_ZOBJ(zobj) \
    SIGNALFORGE_CLIENT_FROM_OBJECT(signalforge_client_object_t, zobj)

#define SIGNALFORGE_HTTP_REQUEST_POOL_FROM_ZOBJ(zobj) \
    SIGNALFORGE_CLIENT_FROM_OBJECT(signalforge_http_request_pool_object_t, zobj)

/* Debug logging */
#ifdef DEBUG_SIGNALFORGE_HTTP_CLIENT
#define SIGNALFORGE_CLIENT_DEBUG(fmt, ...) \
    php_error_docref(NULL, E_NOTICE, "[Signalforge\\NativeHttp\\Client] " fmt, ##__VA_ARGS__)
#else
#define SIGNALFORGE_CLIENT_DEBUG(fmt, ...) ((void)0)
#endif

/* Error handling */
#define SIGNALFORGE_CLIENT_ERROR(fmt, ...) \
    php_error_docref(NULL, E_WARNING, "[Signalforge\\NativeHttp\\Client] " fmt, ##__VA_ARGS__)

/* Registration function */
void signalforge_client_register_class(void);
void signalforge_http_request_pool_register_class(void);
void signalforge_client_exception_register_classes(void);

#endif /* HAVE_SIGNALFORGE_HTTP_CLIENT */

#endif /* SIGNALFORGE_HTTP_CLIENT_H */
