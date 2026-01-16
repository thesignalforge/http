/*
  +----------------------------------------------------------------------+
  | Signalforge HTTP Extension - PSR-18 Client Implementation            |
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
#include "zend_exceptions.h"
#include "zend_interfaces.h"
#include "client.h"
#include "curl_easy.h"
#include "curl_multi_pool.h"
#include "share.h"
#include "request_data.h"
#include "response_data.h"
#include "retry.h"
#include "../response.h"
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT_THREADS
#include "thread_pool.h"
#include "queue.h"
#endif

/* Class entries */
zend_class_entry *signalforge_client_ce;
zend_class_entry *signalforge_http_request_pool_ce;

/* Exception classes */
zend_class_entry *signalforge_http_exception_ce;
zend_class_entry *signalforge_network_exception_ce;
zend_class_entry *signalforge_request_exception_ce;

/* Object handlers */
zend_object_handlers signalforge_client_object_handlers;
zend_object_handlers signalforge_http_request_pool_object_handlers;

/* External class entries */
extern zend_class_entry *signalforge_response_ce;

/* PSR-18 interface class entries (registered in psr7_interfaces.c) */
extern zend_class_entry *psr18_client_interface_ce;
extern zend_class_entry *psr18_network_exception_interface_ce;
extern zend_class_entry *psr18_request_exception_interface_ce;

/* Forward declarations for PSR-7 integration */
static signalforge_client_request_t *signalforge_client_psr7_extract_request(zval *request_zval, signalforge_client_config_t *config);

/* {{{ arginfo */
ZEND_BEGIN_ARG_INFO_EX(arginfo_signalforge_client_construct, 0, 0, 0)
    ZEND_ARG_TYPE_INFO(0, options, IS_ARRAY, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_signalforge_client_sendrequest, 0, 0, 1)
    ZEND_ARG_OBJ_INFO(0, request, Psr\\Http\\Message\\RequestInterface, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_signalforge_http_request_pool_construct, 0, 0, 1)
    ZEND_ARG_OBJ_INFO(0, client, Signalforge\\NativeHttp\\Client, 0)
    ZEND_ARG_TYPE_INFO(0, concurrency, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_signalforge_http_request_pool_add, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, request, IS_OBJECT, 0)
    ZEND_ARG_INFO(0, onSuccess)
    ZEND_ARG_INFO(0, onError)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_signalforge_http_request_pool_wait, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_signalforge_http_request_pool_cancel, 0, 0, 0)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ Client object handlers */
static zend_object *signalforge_client_create_object(zend_class_entry *ce) {
    signalforge_client_object_t *object = zend_object_alloc(sizeof(signalforge_client_object_t), ce);

    zend_object_std_init(&object->std, ce);
    object_properties_init(&object->std, ce);

    object->std.handlers = &signalforge_client_object_handlers;
    object->share = NULL;
    object->config = NULL;
#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT_THREADS
    object->thread_pool = NULL;
    object->use_threads = 0;
#endif

    return &object->std;
}

static void signalforge_client_free_object(zend_object *object) {
    signalforge_client_object_t *client = SIGNALFORGE_CLIENT_FROM_ZOBJ(object);

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT_THREADS
    if (client->thread_pool) {
        signalforge_client_pool_destroy(client->thread_pool);
        client->thread_pool = NULL;
    }
#endif

    if (client->share) {
        signalforge_client_share_destroy(client->share);
        client->share = NULL;
    }

    if (client->config) {
        if (client->config->proxy) {
            efree(client->config->proxy);
        }
        if (client->config->user_agent) {
            efree(client->config->user_agent);
        }
        if (client->config->ca_cert) {
            efree(client->config->ca_cert);
        }
        if (client->config->retry_config) {
            signalforge_client_retry_config_destroy(client->config->retry_config);
        }
        efree(client->config);
        client->config = NULL;
    }

    zend_object_std_dtor(&client->std);
}
/* }}} */

/* {{{ Request pool object handlers */
static zend_object *signalforge_http_request_pool_create_object(zend_class_entry *ce) {
    signalforge_http_request_pool_object_t *object = zend_object_alloc(sizeof(signalforge_http_request_pool_object_t), ce);

    zend_object_std_init(&object->std, ce);
    object_properties_init(&object->std, ce);

    object->std.handlers = &signalforge_http_request_pool_object_handlers;
    object->client = NULL;
    object->concurrency = 50;
    object->request_count = 0;
    object->cancelled = 0;
#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT_THREADS
    object->use_threads = 0;
#endif

    ALLOC_HASHTABLE(object->pending_requests);
    ALLOC_HASHTABLE(object->success_callbacks);
    ALLOC_HASHTABLE(object->error_callbacks);

    zend_hash_init(object->pending_requests, 8, NULL, ZVAL_PTR_DTOR, 0);
    zend_hash_init(object->success_callbacks, 8, NULL, ZVAL_PTR_DTOR, 0);
    zend_hash_init(object->error_callbacks, 8, NULL, ZVAL_PTR_DTOR, 0);

    return &object->std;
}

static void signalforge_http_request_pool_free_object(zend_object *object) {
    signalforge_http_request_pool_object_t *pool = SIGNALFORGE_HTTP_REQUEST_POOL_FROM_ZOBJ(object);

    if (pool->pending_requests) {
        zend_hash_destroy(pool->pending_requests);
        FREE_HASHTABLE(pool->pending_requests);
    }

    if (pool->success_callbacks) {
        zend_hash_destroy(pool->success_callbacks);
        FREE_HASHTABLE(pool->success_callbacks);
    }

    if (pool->error_callbacks) {
        zend_hash_destroy(pool->error_callbacks);
        FREE_HASHTABLE(pool->error_callbacks);
    }

    pool->client = NULL;

    zend_object_std_dtor(&pool->std);
}
/* }}} */

/* {{{ PSR-7 Integration helpers */
static char *extract_method(zval *request_obj) {
    zval method_result;

    zend_call_method_with_0_params(Z_OBJ_P(request_obj), Z_OBJCE_P(request_obj), NULL, "getmethod", &method_result);

    if (Z_TYPE(method_result) == IS_STRING && Z_STRLEN(method_result) > 0) {
        char *method = estrdup(Z_STRVAL(method_result));
        zval_ptr_dtor(&method_result);
        return method;
    }

    zval_ptr_dtor(&method_result);
    return estrdup("GET");
}

static char *extract_uri(zval *request_obj) {
    zval uri_result;

    zend_call_method_with_0_params(Z_OBJ_P(request_obj), Z_OBJCE_P(request_obj), NULL, "geturi", &uri_result);

    if (Z_TYPE(uri_result) == IS_OBJECT) {
        zval str_result;
        zend_call_method_with_0_params(Z_OBJ(uri_result), Z_OBJCE(uri_result), NULL, "__tostring", &str_result);

        if (Z_TYPE(str_result) == IS_STRING && Z_STRLEN(str_result) > 0) {
            char *uri = estrdup(Z_STRVAL(str_result));
            zval_ptr_dtor(&str_result);
            zval_ptr_dtor(&uri_result);
            return uri;
        }
        zval_ptr_dtor(&str_result);
    } else if (Z_TYPE(uri_result) == IS_STRING && Z_STRLEN(uri_result) > 0) {
        char *uri = estrdup(Z_STRVAL(uri_result));
        zval_ptr_dtor(&uri_result);
        return uri;
    }

    zval_ptr_dtor(&uri_result);
    return estrdup("http://localhost/");
}

static void extract_headers(zval *request_obj, zval *headers_array) {
    zval headers_result;

    zend_call_method_with_0_params(Z_OBJ_P(request_obj), Z_OBJCE_P(request_obj), NULL, "getheaders", &headers_result);

    if (Z_TYPE(headers_result) == IS_ARRAY) {
        ZVAL_COPY(headers_array, &headers_result);
    } else {
        array_init(headers_array);
    }

    zval_ptr_dtor(&headers_result);
}

static void extract_body(zval *request_obj, char **body, size_t *body_len) {
    zval body_result;

    zend_call_method_with_0_params(Z_OBJ_P(request_obj), Z_OBJCE_P(request_obj), NULL, "getbody", &body_result);

    if (Z_TYPE(body_result) == IS_OBJECT) {
        /* Use zval_get_string() which properly invokes __toString magic method
         * This is the correct way to convert an object to string in Zend Engine */
        zend_string *str = zval_get_string(&body_result);

        if (str && ZSTR_LEN(str) > 0) {
            *body = emalloc(ZSTR_LEN(str));
            if (*body) {
                memcpy(*body, ZSTR_VAL(str), ZSTR_LEN(str));
                *body_len = ZSTR_LEN(str);
            }
        }
        zend_string_release(str);
    } else if (Z_TYPE(body_result) == IS_STRING && Z_STRLEN(body_result) > 0) {
        *body = emalloc(Z_STRLEN(body_result));
        if (*body) {
            memcpy(*body, Z_STRVAL(body_result), Z_STRLEN(body_result));
            *body_len = Z_STRLEN(body_result);
        }
    }

    zval_ptr_dtor(&body_result);
}

static signalforge_client_request_t *signalforge_client_psr7_extract_request(
    zval *request_zval,
    signalforge_client_config_t *config
) {
    if (!request_zval || Z_TYPE_P(request_zval) != IS_OBJECT) {
        return NULL;
    }

    char *method = extract_method(request_zval);
    char *url = extract_uri(request_zval);

    zval headers;
    extract_headers(request_zval, &headers);

    char *body = NULL;
    size_t body_len = 0;
    extract_body(request_zval, &body, &body_len);

    signalforge_client_request_t *request = signalforge_client_request_create(
        method,
        url,
        &headers,
        body,
        body_len,
        config
    );

    efree(method);
    efree(url);
    if (body) {
        efree(body);
    }
    zval_ptr_dtor(&headers);

    return request;
}
/* }}} */

/* {{{ proto void Client::__construct(array $options = []) */
static PHP_METHOD(SignalforgeClient, __construct) {
    zval *options = NULL;
    signalforge_client_object_t *client_obj;
    signalforge_client_config_t *config;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY(options)
    ZEND_PARSE_PARAMETERS_END();

    client_obj = SIGNALFORGE_CLIENT_FROM_ZOBJ(Z_OBJ_P(ZEND_THIS));

    config = ecalloc(1, sizeof(signalforge_client_config_t));
    if (!config) {
        zend_throw_exception(signalforge_http_exception_ce, "Failed to allocate memory for configuration", 0);
        RETURN_THROWS();
    }

    /* Set defaults */
    config->pool_size = SIGNALFORGE_CLIENT_DEFAULT_POOL_SIZE;
    config->connect_timeout = SIGNALFORGE_CLIENT_DEFAULT_CONNECT_TIMEOUT;
    config->timeout = SIGNALFORGE_CLIENT_DEFAULT_TIMEOUT;
    config->http_version = SIGNALFORGE_HTTP_VERSION_AUTO;
    config->max_redirects = SIGNALFORGE_CLIENT_DEFAULT_MAX_REDIRECTS;
    config->follow_redirects = 1;
    config->verify_peer = 1;
    config->verify_host = 1;
    config->debug = 0;

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT_THREADS
    int use_threads = 0;
#endif

    /* Parse options */
    if (options && Z_TYPE_P(options) == IS_ARRAY) {
        zval *val;

        if ((val = zend_hash_str_find(Z_ARRVAL_P(options), "pool_size", sizeof("pool_size") - 1)) != NULL) {
            config->pool_size = (int)zval_get_long(val);
            if (config->pool_size <= 0) config->pool_size = SIGNALFORGE_CLIENT_DEFAULT_POOL_SIZE;
        }

        if ((val = zend_hash_str_find(Z_ARRVAL_P(options), "connect_timeout", sizeof("connect_timeout") - 1)) != NULL) {
            config->connect_timeout = (int)zval_get_long(val);
        }

        if ((val = zend_hash_str_find(Z_ARRVAL_P(options), "timeout", sizeof("timeout") - 1)) != NULL) {
            config->timeout = (int)zval_get_long(val);
        }

        if ((val = zend_hash_str_find(Z_ARRVAL_P(options), "http_version", sizeof("http_version") - 1)) != NULL) {
            if (Z_TYPE_P(val) == IS_STRING) {
                const char *version = Z_STRVAL_P(val);
                if (strcmp(version, "1.0") == 0) {
                    config->http_version = SIGNALFORGE_HTTP_VERSION_1_0;
                } else if (strcmp(version, "1.1") == 0) {
                    config->http_version = SIGNALFORGE_HTTP_VERSION_1_1;
                } else if (strcmp(version, "2") == 0 || strcmp(version, "2.0") == 0) {
                    config->http_version = SIGNALFORGE_HTTP_VERSION_2_0;
                } else if (strcmp(version, "3") == 0 || strcmp(version, "3.0") == 0) {
                    config->http_version = SIGNALFORGE_HTTP_VERSION_3_0;
                }
            } else {
                config->http_version = (int)zval_get_long(val);
            }
        }

        if ((val = zend_hash_str_find(Z_ARRVAL_P(options), "proxy", sizeof("proxy") - 1)) != NULL) {
            zend_string *proxy_str = zval_get_string(val);
            config->proxy = estrdup(ZSTR_VAL(proxy_str));
            zend_string_release(proxy_str);
        }

        if ((val = zend_hash_str_find(Z_ARRVAL_P(options), "user_agent", sizeof("user_agent") - 1)) != NULL) {
            zend_string *ua_str = zval_get_string(val);
            config->user_agent = estrdup(ZSTR_VAL(ua_str));
            zend_string_release(ua_str);
        }

        if ((val = zend_hash_str_find(Z_ARRVAL_P(options), "verify_peer", sizeof("verify_peer") - 1)) != NULL) {
            config->verify_peer = zend_is_true(val);
        }

        if ((val = zend_hash_str_find(Z_ARRVAL_P(options), "verify_host", sizeof("verify_host") - 1)) != NULL) {
            config->verify_host = zend_is_true(val);
        }

        if ((val = zend_hash_str_find(Z_ARRVAL_P(options), "ca_cert", sizeof("ca_cert") - 1)) != NULL) {
            zend_string *ca_str = zval_get_string(val);
            config->ca_cert = estrdup(ZSTR_VAL(ca_str));
            zend_string_release(ca_str);
        }

        if ((val = zend_hash_str_find(Z_ARRVAL_P(options), "follow_redirects", sizeof("follow_redirects") - 1)) != NULL) {
            config->follow_redirects = zend_is_true(val);
        }

        if ((val = zend_hash_str_find(Z_ARRVAL_P(options), "max_redirects", sizeof("max_redirects") - 1)) != NULL) {
            config->max_redirects = (int)zval_get_long(val);
        }

        /* Retry configuration */
        if ((val = zend_hash_str_find(Z_ARRVAL_P(options), "retry", sizeof("retry") - 1)) != NULL && Z_TYPE_P(val) == IS_ARRAY) {
            zval *retry_val;
            int max_retries = SIGNALFORGE_CLIENT_DEFAULT_MAX_RETRIES;
            int delay_ms = SIGNALFORGE_CLIENT_DEFAULT_RETRY_DELAY_MS;
            int max_delay_ms = 60000;
            double backoff = 2.0;

            if ((retry_val = zend_hash_str_find(Z_ARRVAL_P(val), "max", sizeof("max") - 1)) != NULL) {
                max_retries = (int)zval_get_long(retry_val);
            }

            if ((retry_val = zend_hash_str_find(Z_ARRVAL_P(val), "delay", sizeof("delay") - 1)) != NULL) {
                delay_ms = (int)zval_get_long(retry_val);
            }

            if ((retry_val = zend_hash_str_find(Z_ARRVAL_P(val), "max_delay", sizeof("max_delay") - 1)) != NULL) {
                max_delay_ms = (int)zval_get_long(retry_val);
            }

            if ((retry_val = zend_hash_str_find(Z_ARRVAL_P(val), "backoff", sizeof("backoff") - 1)) != NULL) {
                backoff = zval_get_double(retry_val);
            }

            config->retry_config = signalforge_client_retry_config_create(max_retries, delay_ms, max_delay_ms, backoff);
        }

        if ((val = zend_hash_str_find(Z_ARRVAL_P(options), "debug", sizeof("debug") - 1)) != NULL) {
            config->debug = zend_is_true(val);
        }

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT_THREADS
        /* Thread pool mode (optional, ZTS only) */
        if ((val = zend_hash_str_find(Z_ARRVAL_P(options), "use_threads", sizeof("use_threads") - 1)) != NULL) {
            use_threads = zend_is_true(val);
        }
#endif
    }

    client_obj->config = config;

    /* Create shared connection cache */
    client_obj->share = signalforge_client_share_create();
    if (!client_obj->share) {
        efree(config);
        client_obj->config = NULL;
        zend_throw_exception(signalforge_http_exception_ce, "Failed to create shared connection cache", 0);
        RETURN_THROWS();
    }

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT_THREADS
    client_obj->use_threads = use_threads;
    if (use_threads) {
        /* Create thread pool for parallel execution */
        client_obj->thread_pool = signalforge_client_pool_create(config);
        if (!client_obj->thread_pool) {
            signalforge_client_share_destroy(client_obj->share);
            client_obj->share = NULL;
            efree(config);
            client_obj->config = NULL;
            zend_throw_exception(signalforge_http_exception_ce, "Failed to create thread pool", 0);
            RETURN_THROWS();
        }
    }
#endif
}
/* }}} */

/* {{{ proto ResponseInterface Client::sendRequest(RequestInterface $request)
 * Execute HTTP request synchronously using curl_easy_perform
 * This is PSR-18 compliant - blocking until response is received
 */
static PHP_METHOD(SignalforgeClient, sendRequest) {
    zval *request_zval;
    signalforge_client_object_t *client_obj;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT(request_zval)
    ZEND_PARSE_PARAMETERS_END();

    client_obj = SIGNALFORGE_CLIENT_FROM_ZOBJ(Z_OBJ_P(ZEND_THIS));

    if (!client_obj->config) {
        zend_throw_exception(signalforge_http_exception_ce, "Client not initialized", 0);
        RETURN_THROWS();
    }

    /* Extract request data from PSR-7 object */
    signalforge_client_request_t *request = signalforge_client_psr7_extract_request(
        request_zval,
        client_obj->config
    );

    if (!request) {
        zend_throw_exception(signalforge_request_exception_ce,
            "Failed to extract request data from PSR-7 object", 0);
        RETURN_THROWS();
    }

    /* Execute request synchronously using curl_easy */
    signalforge_client_response_t *response = signalforge_curl_easy_execute(
        request,
        client_obj->share,
        client_obj->config
    );

    /* Request is consumed by execute (or must be freed on error) */
    signalforge_client_request_destroy(request);

    if (!response) {
        zend_throw_exception(signalforge_network_exception_ce,
            "Request failed - no response received", 0);
        RETURN_THROWS();
    }

    /* Check for errors */
    if (response->is_error) {
        /* Copy error details before destroying response to avoid use-after-free */
        CURLcode curl_code = response->curl_code;
        char *msg_copy = NULL;

        if (response->error_message) {
            msg_copy = estrdup(response->error_message);
        }

        signalforge_client_response_destroy(response);

        /* Build detailed error message including curl error code */
        char detailed_msg[512];
        if (msg_copy && curl_code != 0) {
            snprintf(detailed_msg, sizeof(detailed_msg),
                "Network error (curl code %d): %s", (int)curl_code, msg_copy);
        } else if (msg_copy) {
            snprintf(detailed_msg, sizeof(detailed_msg), "%s", msg_copy);
        } else if (curl_code != 0) {
            snprintf(detailed_msg, sizeof(detailed_msg),
                "Network error (curl code %d)", (int)curl_code);
        } else {
            snprintf(detailed_msg, sizeof(detailed_msg), "Network error occurred");
        }

        zend_throw_exception(signalforge_network_exception_ce, detailed_msg, (zend_long)curl_code);

        if (msg_copy) {
            efree(msg_copy);
        }
        RETURN_THROWS();
    }

    /* Convert response to PSR-7 object */
    zval response_zval = signalforge_client_create_psr7_response(response);

    /* Cleanup */
    signalforge_client_response_destroy(response);

    RETURN_ZVAL(&response_zval, 0, 1);
}
/* }}} */

/* {{{ proto void HttpRequestPool::__construct(Client $client, int $concurrency = 50) */
static PHP_METHOD(SignalforgeHttpRequestPool, __construct) {
    zval *client_zval;
    zend_long concurrency = 50;
    signalforge_http_request_pool_object_t *pool_obj;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_OBJECT_OF_CLASS(client_zval, signalforge_client_ce)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(concurrency)
    ZEND_PARSE_PARAMETERS_END();

    pool_obj = SIGNALFORGE_HTTP_REQUEST_POOL_FROM_ZOBJ(Z_OBJ_P(ZEND_THIS));
    pool_obj->client = SIGNALFORGE_CLIENT_FROM_ZOBJ(Z_OBJ_P(client_zval));
    pool_obj->concurrency = (int)concurrency;

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT_THREADS
    /* Inherit thread mode from client */
    pool_obj->use_threads = pool_obj->client->use_threads;
#endif
}
/* }}} */

/* {{{ proto void HttpRequestPool::add(RequestInterface $request, ?callable $onSuccess = null, ?callable $onError = null) */
static PHP_METHOD(SignalforgeHttpRequestPool, add) {
    zval *request_zval;
    zval *on_success = NULL;
    zval *on_error = NULL;
    signalforge_http_request_pool_object_t *pool_obj;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_OBJECT(request_zval)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL_OR_NULL(on_success)
        Z_PARAM_ZVAL_OR_NULL(on_error)
    ZEND_PARSE_PARAMETERS_END();

    pool_obj = SIGNALFORGE_HTTP_REQUEST_POOL_FROM_ZOBJ(Z_OBJ_P(ZEND_THIS));

    if (pool_obj->cancelled) {
        zend_throw_exception(signalforge_http_exception_ce, "Cannot add requests to a cancelled pool", 0);
        return;
    }

    zval request_copy;
    ZVAL_COPY(&request_copy, request_zval);
    zend_hash_next_index_insert(pool_obj->pending_requests, &request_copy);

    if (on_success && Z_TYPE_P(on_success) == IS_OBJECT && zend_is_callable(on_success, 0, NULL)) {
        zval callback_copy;
        ZVAL_COPY(&callback_copy, on_success);
        zend_hash_index_update(pool_obj->success_callbacks, pool_obj->request_count, &callback_copy);
    } else {
        zval null_val;
        ZVAL_NULL(&null_val);
        zend_hash_index_update(pool_obj->success_callbacks, pool_obj->request_count, &null_val);
    }

    if (on_error && Z_TYPE_P(on_error) == IS_OBJECT && zend_is_callable(on_error, 0, NULL)) {
        zval callback_copy;
        ZVAL_COPY(&callback_copy, on_error);
        zend_hash_index_update(pool_obj->error_callbacks, pool_obj->request_count, &callback_copy);
    } else {
        zval null_val;
        ZVAL_NULL(&null_val);
        zend_hash_index_update(pool_obj->error_callbacks, pool_obj->request_count, &null_val);
    }

    pool_obj->request_count++;
}
/* }}} */

/* {{{ Event-driven wait using curl_multi_pool (default mode) */
static void http_request_pool_wait_event_driven(
    signalforge_http_request_pool_object_t *pool_obj,
    zval *return_value
) {
    /* Create curl_multi pool */
    signalforge_curl_multi_pool_t *multi_pool = signalforge_curl_multi_pool_create(
        pool_obj->client->share,
        pool_obj->client->config,
        pool_obj->concurrency
    );

    if (!multi_pool) {
        zend_throw_exception(signalforge_http_exception_ce, "Failed to create request pool", 0);
        return;
    }

    /* Add all pending requests to the pool */
    zval *request_zval;
    zend_ulong idx;
    int added_count = 0;

    ZEND_HASH_FOREACH_NUM_KEY_VAL(pool_obj->pending_requests, idx, request_zval) {
        if (pool_obj->cancelled) {
            break;
        }

        signalforge_client_request_t *request = signalforge_client_psr7_extract_request(
            request_zval,
            pool_obj->client->config
        );

        if (!request) {
            continue;
        }

        /* Store index for callback lookup */
        request->user_data = (void *)(uintptr_t)idx;

        if (signalforge_curl_multi_pool_add(multi_pool, request) == 0) {
            added_count++;
        } else {
            signalforge_client_request_destroy(request);
        }
    } ZEND_HASH_FOREACH_END();

    /* Execute all requests concurrently */
    int timeout_ms = (pool_obj->client->config->timeout + pool_obj->client->config->connect_timeout) * 1000 + 5000;

    if (added_count > 0 && !pool_obj->cancelled) {
        signalforge_curl_multi_pool_execute(multi_pool, timeout_ms);
    }

    /* Collect responses */
    array_init(return_value);

    size_t response_count;
    signalforge_client_response_t **responses = signalforge_curl_multi_pool_get_responses(multi_pool, &response_count);

    for (size_t i = 0; i < response_count; i++) {
        signalforge_client_response_t *response = responses[i];
        if (!response) continue;

        zend_ulong request_idx = (zend_ulong)(uintptr_t)response->user_data;

        /* Convert to PSR-7 response */
        zval response_zval = signalforge_client_create_psr7_response(response);

        int is_error_response = response->is_error;

        /* Call appropriate callback */
        if (is_error_response) {
            zval *error_callback = zend_hash_index_find(pool_obj->error_callbacks, request_idx);
            if (error_callback && Z_TYPE_P(error_callback) != IS_NULL && zend_is_callable(error_callback, 0, NULL)) {
                zval retval;
                zval args[1];
                ZVAL_COPY(&args[0], &response_zval);

                call_user_function(NULL, NULL, error_callback, &retval, 1, args);

                zval_ptr_dtor(&retval);
                zval_ptr_dtor(&args[0]);
            }
        } else {
            zval *success_callback = zend_hash_index_find(pool_obj->success_callbacks, request_idx);
            if (success_callback && Z_TYPE_P(success_callback) != IS_NULL && zend_is_callable(success_callback, 0, NULL)) {
                zval retval;
                zval args[1];
                ZVAL_COPY(&args[0], &response_zval);

                call_user_function(NULL, NULL, success_callback, &retval, 1, args);

                zval_ptr_dtor(&retval);
                zval_ptr_dtor(&args[0]);
            }
        }

        zend_hash_index_update(Z_ARRVAL_P(return_value), request_idx, &response_zval);

        /* Cleanup response */
        signalforge_client_response_destroy(response);
    }

    /* Cleanup pool */
    signalforge_curl_multi_pool_destroy(multi_pool);
}
/* }}} */

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT_THREADS
/* {{{ Thread pool wait (ZTS mode with use_threads=true) */
static void http_request_pool_wait_threaded(
    signalforge_http_request_pool_object_t *pool_obj,
    zval *return_value
) {
    zval *request_zval;
    zend_ulong idx;
    int submitted_count = 0;
    int received_count = 0;

    ZEND_HASH_FOREACH_NUM_KEY_VAL(pool_obj->pending_requests, idx, request_zval) {
        if (pool_obj->cancelled) {
            break;
        }

        signalforge_client_request_t *request = signalforge_client_psr7_extract_request(
            request_zval,
            pool_obj->client->config
        );

        if (!request) {
            continue;
        }

        request->user_data = (void *)(uintptr_t)idx;

        if (signalforge_client_pool_submit(pool_obj->client->thread_pool, request) == 0) {
            submitted_count++;
        } else {
            signalforge_client_request_destroy(request);
        }
    } ZEND_HASH_FOREACH_END();

    array_init(return_value);

    int timeout_ms = (pool_obj->client->config->timeout + pool_obj->client->config->connect_timeout) * 1000 + 5000;

    while (received_count < submitted_count && !pool_obj->cancelled) {
        signalforge_client_response_t *response = signalforge_client_pool_wait_response(
            pool_obj->client->thread_pool,
            timeout_ms
        );

        if (!response) {
            break;
        }

        received_count++;

        zend_ulong request_idx = (zend_ulong)(uintptr_t)response->user_data;

        zval response_zval = signalforge_client_create_psr7_response(response);

        int is_error_response = 0;
        if (Z_TYPE(response_zval) == IS_ARRAY) {
            zval *is_error = zend_hash_str_find(Z_ARRVAL(response_zval), "error", sizeof("error") - 1);
            if (is_error && Z_TYPE_P(is_error) == IS_TRUE) {
                is_error_response = 1;
            }
        }

        if (is_error_response) {
            zval *error_callback = zend_hash_index_find(pool_obj->error_callbacks, request_idx);
            if (error_callback && Z_TYPE_P(error_callback) != IS_NULL && zend_is_callable(error_callback, 0, NULL)) {
                zval retval;
                zval args[1];
                ZVAL_COPY(&args[0], &response_zval);

                call_user_function(NULL, NULL, error_callback, &retval, 1, args);

                zval_ptr_dtor(&retval);
                zval_ptr_dtor(&args[0]);
            }
        } else {
            zval *success_callback = zend_hash_index_find(pool_obj->success_callbacks, request_idx);
            if (success_callback && Z_TYPE_P(success_callback) != IS_NULL && zend_is_callable(success_callback, 0, NULL)) {
                zval retval;
                zval args[1];
                ZVAL_COPY(&args[0], &response_zval);

                call_user_function(NULL, NULL, success_callback, &retval, 1, args);

                zval_ptr_dtor(&retval);
                zval_ptr_dtor(&args[0]);
            }
        }

        zend_hash_index_update(Z_ARRVAL_P(return_value), request_idx, &response_zval);

        signalforge_client_response_destroy(response);
    }
}
/* }}} */
#endif

/* {{{ proto array HttpRequestPool::wait() */
static PHP_METHOD(SignalforgeHttpRequestPool, wait) {
    signalforge_http_request_pool_object_t *pool_obj;

    ZEND_PARSE_PARAMETERS_NONE();

    pool_obj = SIGNALFORGE_HTTP_REQUEST_POOL_FROM_ZOBJ(Z_OBJ_P(ZEND_THIS));

    if (!pool_obj->client || !pool_obj->client->config) {
        zend_throw_exception(signalforge_http_exception_ce, "Client not initialized", 0);
        RETURN_THROWS();
    }

    if (pool_obj->cancelled) {
        array_init(return_value);
        goto cleanup;
    }

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT_THREADS
    if (pool_obj->use_threads && pool_obj->client->thread_pool) {
        http_request_pool_wait_threaded(pool_obj, return_value);
    } else {
        http_request_pool_wait_event_driven(pool_obj, return_value);
    }
#else
    http_request_pool_wait_event_driven(pool_obj, return_value);
#endif

cleanup:
    zend_hash_clean(pool_obj->pending_requests);
    zend_hash_clean(pool_obj->success_callbacks);
    zend_hash_clean(pool_obj->error_callbacks);
    pool_obj->request_count = 0;
}
/* }}} */

/* {{{ proto void HttpRequestPool::cancel() */
static PHP_METHOD(SignalforgeHttpRequestPool, cancel) {
    signalforge_http_request_pool_object_t *pool_obj;

    ZEND_PARSE_PARAMETERS_NONE();

    pool_obj = SIGNALFORGE_HTTP_REQUEST_POOL_FROM_ZOBJ(Z_OBJ_P(ZEND_THIS));

    pool_obj->cancelled = 1;

    zend_hash_clean(pool_obj->pending_requests);
    zend_hash_clean(pool_obj->success_callbacks);
    zend_hash_clean(pool_obj->error_callbacks);
    pool_obj->request_count = 0;
}
/* }}} */

/* {{{ Client methods */
static const zend_function_entry signalforge_client_methods[] = {
    PHP_ME(SignalforgeClient, __construct, arginfo_signalforge_client_construct, ZEND_ACC_PUBLIC)
    PHP_ME(SignalforgeClient, sendRequest, arginfo_signalforge_client_sendrequest, ZEND_ACC_PUBLIC)
    PHP_FE_END
};
/* }}} */

/* {{{ HttpRequestPool methods */
static const zend_function_entry signalforge_http_request_pool_methods[] = {
    PHP_ME(SignalforgeHttpRequestPool, __construct, arginfo_signalforge_http_request_pool_construct, ZEND_ACC_PUBLIC)
    PHP_ME(SignalforgeHttpRequestPool, add, arginfo_signalforge_http_request_pool_add, ZEND_ACC_PUBLIC)
    PHP_ME(SignalforgeHttpRequestPool, wait, arginfo_signalforge_http_request_pool_wait, ZEND_ACC_PUBLIC)
    PHP_ME(SignalforgeHttpRequestPool, cancel, arginfo_signalforge_http_request_pool_cancel, ZEND_ACC_PUBLIC)
    PHP_FE_END
};
/* }}} */

/* {{{ Register Client class */
void signalforge_client_register_class(void) {
    zend_class_entry ce;

    /* Register Client class */
    INIT_NS_CLASS_ENTRY(ce, "Signalforge\\NativeHttp", "Client", signalforge_client_methods);
    signalforge_client_ce = zend_register_internal_class(&ce);
    signalforge_client_ce->create_object = signalforge_client_create_object;

    memcpy(&signalforge_client_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    signalforge_client_object_handlers.offset = XtOffsetOf(signalforge_client_object_t, std);
    signalforge_client_object_handlers.free_obj = signalforge_client_free_object;

    /* Implement PSR-18 ClientInterface if available */
    if (psr18_client_interface_ce) {
        zend_class_implements(signalforge_client_ce, 1, psr18_client_interface_ce);
    }
}
/* }}} */

/* {{{ Register HttpRequestPool class */
void signalforge_http_request_pool_register_class(void) {
    zend_class_entry ce;

    INIT_NS_CLASS_ENTRY(ce, "Signalforge\\NativeHttp", "HttpRequestPool", signalforge_http_request_pool_methods);
    signalforge_http_request_pool_ce = zend_register_internal_class(&ce);
    signalforge_http_request_pool_ce->create_object = signalforge_http_request_pool_create_object;

    memcpy(&signalforge_http_request_pool_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    signalforge_http_request_pool_object_handlers.offset = XtOffsetOf(signalforge_http_request_pool_object_t, std);
    signalforge_http_request_pool_object_handlers.free_obj = signalforge_http_request_pool_free_object;
}
/* }}} */

/* {{{ Register exception classes */
void signalforge_client_exception_register_classes(void) {
    zend_class_entry ce;

    /* Base exception */
    INIT_NS_CLASS_ENTRY(ce, "Signalforge\\NativeHttp", "HttpException", NULL);
    signalforge_http_exception_ce = zend_register_internal_class_ex(&ce, zend_ce_exception);

    /* NetworkException - implements PSR-18 NetworkExceptionInterface */
    INIT_NS_CLASS_ENTRY(ce, "Signalforge\\NativeHttp", "NetworkException", NULL);
    signalforge_network_exception_ce = zend_register_internal_class_ex(&ce, signalforge_http_exception_ce);
    if (psr18_network_exception_interface_ce) {
        zend_class_implements(signalforge_network_exception_ce, 1, psr18_network_exception_interface_ce);
    }

    /* RequestException - implements PSR-18 RequestExceptionInterface */
    INIT_NS_CLASS_ENTRY(ce, "Signalforge\\NativeHttp", "RequestException", NULL);
    signalforge_request_exception_ce = zend_register_internal_class_ex(&ce, signalforge_http_exception_ce);
    if (psr18_request_exception_interface_ce) {
        zend_class_implements(signalforge_request_exception_ce, 1, psr18_request_exception_interface_ce);
    }
}
/* }}} */

#endif /* HAVE_SIGNALFORGE_HTTP_CLIENT */
