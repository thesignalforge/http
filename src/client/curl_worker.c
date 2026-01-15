/*
  +----------------------------------------------------------------------+
  | Signalforge HTTP Extension - CURL Worker Thread Implementation       |
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
#include "thread_pool.h"
#include "curl_worker.h"
#include "request_data.h"
#include "response_data.h"
#include "retry.h"
#include "queue.h"
#include "share.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PHP_SIGNALFORGE_HTTP_CLIENT_VERSION "1.0.0"

/**
 * Create a new handle pool
 */
signalforge_curl_handle_pool_t *signalforge_curl_handle_pool_create(void) {
    signalforge_curl_handle_pool_t *pool = calloc(1, sizeof(signalforge_curl_handle_pool_t));
    if (!pool) {
        return NULL;
    }

    pool->available_count = 0;
    pool->total_created = 0;

    return pool;
}

/**
 * Acquire a handle from pool or create new one
 */
CURL *signalforge_curl_handle_pool_acquire(signalforge_curl_handle_pool_t *pool, int *from_pool) {
    CURL *handle = NULL;

    if (!pool) {
        if (from_pool) *from_pool = 0;
        return curl_easy_init();
    }

    /* Try to get handle from pool (LIFO) */
    if (pool->available_count > 0) {
        pool->available_count--;
        handle = pool->handles[pool->available_count];
        pool->handles[pool->available_count] = NULL;

        curl_easy_reset(handle);

        if (from_pool) *from_pool = 1;
        return handle;
    }

    /* Pool empty - create new handle */
    handle = curl_easy_init();
    if (handle) {
        pool->total_created++;
    }

    if (from_pool) *from_pool = 0;
    return handle;
}

/**
 * Release handle back to pool for reuse
 */
void signalforge_curl_handle_pool_release(signalforge_curl_handle_pool_t *pool, CURL *handle) {
    if (!handle) {
        return;
    }

    if (!pool) {
        curl_easy_cleanup(handle);
        return;
    }

    if (pool->available_count < SIGNALFORGE_CURL_HANDLE_POOL_SIZE) {
        pool->handles[pool->available_count] = handle;
        pool->available_count++;
    } else {
        curl_easy_cleanup(handle);
    }
}

/**
 * Discard handle without returning to pool
 */
void signalforge_curl_handle_pool_discard(signalforge_curl_handle_pool_t *pool, CURL *handle) {
    if (!handle) {
        return;
    }

    curl_easy_cleanup(handle);
    (void)pool;
}

/**
 * Destroy pool and all handles within it
 */
void signalforge_curl_handle_pool_destroy(signalforge_curl_handle_pool_t *pool) {
    if (!pool) {
        return;
    }

    for (int i = 0; i < pool->available_count; i++) {
        if (pool->handles[i]) {
            curl_easy_cleanup(pool->handles[i]);
            pool->handles[i] = NULL;
        }
    }

    free(pool);
}

/* Zero-copy request body read callback */
static size_t request_body_read_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
    signalforge_body_read_context_t *ctx = (signalforge_body_read_context_t *)userdata;

    if (!ctx || !ctx->data) {
        return 0;
    }

    size_t remaining = ctx->data_len - ctx->bytes_read;
    if (remaining == 0) {
        return 0;
    }

    size_t max_bytes = size * nitems;
    size_t to_send = (remaining < max_bytes) ? remaining : max_bytes;

    memcpy(buffer, ctx->data + ctx->bytes_read, to_send);
    ctx->bytes_read += to_send;

    return to_send;
}

/* Write callback for response body */
static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    signalforge_active_request_t *req = (signalforge_active_request_t *)userdata;
    size_t total_size = size * nmemb;

    if (req->response_body_len + total_size > req->response_body_capacity) {
        size_t new_capacity = req->response_body_capacity * 2;
        if (new_capacity < req->response_body_len + total_size) {
            new_capacity = req->response_body_len + total_size + 4096;
        }

        char *new_body = realloc(req->response_body, new_capacity);
        if (!new_body) {
            return 0;
        }

        req->response_body = new_body;
        req->response_body_capacity = new_capacity;
    }

    memcpy(req->response_body + req->response_body_len, ptr, total_size);
    req->response_body_len += total_size;

    return total_size;
}

/* Header callback for response headers */
static size_t header_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    signalforge_active_request_t *req = (signalforge_active_request_t *)userdata;
    size_t total_size = size * nmemb;

    if (req->response_headers_len + total_size > req->response_headers_capacity) {
        size_t new_capacity = req->response_headers_capacity * 2;
        if (new_capacity < req->response_headers_len + total_size) {
            new_capacity = req->response_headers_len + total_size + 1024;
        }

        char *new_headers = realloc(req->response_headers, new_capacity);
        if (!new_headers) {
            return 0;
        }

        req->response_headers = new_headers;
        req->response_headers_capacity = new_capacity;
    }

    memcpy(req->response_headers + req->response_headers_len, ptr, total_size);
    req->response_headers_len += total_size;

    return total_size;
}

/* Parse response headers */
static void parse_response_headers(signalforge_active_request_t *req) {
    if (!req->response_headers || req->response_headers_len == 0) {
        return;
    }

    size_t header_count = 0;
    char *ptr = req->response_headers;
    char *end = req->response_headers + req->response_headers_len;

    while (ptr < end) {
        char *line_end = memchr(ptr, '\n', end - ptr);
        if (!line_end) break;

        if (memchr(ptr, ':', line_end - ptr)) {
            header_count++;
        }

        ptr = line_end + 1;
    }

    if (header_count == 0) {
        return;
    }

    req->response->headers = malloc(sizeof(signalforge_client_header_t) * header_count);
    if (!req->response->headers) {
        return;
    }

    size_t idx = 0;
    ptr = req->response_headers;

    while (ptr < end && idx < header_count) {
        char *line_end = memchr(ptr, '\n', end - ptr);
        if (!line_end) break;

        char *colon = memchr(ptr, ':', line_end - ptr);
        if (colon) {
            size_t name_len = colon - ptr;
            char *name = malloc(name_len + 1);
            if (name) {
                memcpy(name, ptr, name_len);
                name[name_len] = '\0';

                while (name_len > 0 && name[name_len - 1] == ' ') {
                    name[--name_len] = '\0';
                }

                char *value_start = colon + 1;
                while (value_start < line_end && *value_start == ' ') {
                    value_start++;
                }

                size_t value_len = line_end - value_start;
                if (value_len > 0 && value_start[value_len - 1] == '\r') {
                    value_len--;
                }

                char *value = malloc(value_len + 1);
                if (value) {
                    memcpy(value, value_start, value_len);
                    value[value_len] = '\0';

                    req->response->headers[idx].name = name;
                    req->response->headers[idx].value = value;
                    idx++;
                } else {
                    free(name);
                }
            }
        }

        ptr = line_end + 1;
    }

    req->response->header_count = idx;
}

/* Setup curl handle */
static int setup_curl_handle(
    signalforge_active_request_t *req,
    signalforge_curl_handle_pool_t *handle_pool,
    CURLSH *share_handle,
    signalforge_client_config_t *config
) {
    int from_pool = 0;
    CURL *curl = signalforge_curl_handle_pool_acquire(handle_pool, &from_pool);
    if (!curl) {
        return -1;
    }

    req->curl_handle = curl;
    req->handle_from_pool = from_pool;

    curl_easy_setopt(curl, CURLOPT_URL, req->request->url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, req->request->method);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, req);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, req);

    if (config->connect_timeout > 0) {
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, config->connect_timeout);
    }
    if (config->timeout > 0) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, config->timeout);
    }

    switch (config->http_version) {
        case SIGNALFORGE_HTTP_VERSION_1_0:
            curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
            break;
        case SIGNALFORGE_HTTP_VERSION_1_1:
            curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
            break;
        case SIGNALFORGE_HTTP_VERSION_2_0:
            curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
            curl_easy_setopt(curl, CURLOPT_PIPEWAIT, 1L);
            break;
#ifdef HAVE_HTTP3
        case SIGNALFORGE_HTTP_VERSION_3_0:
            curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_3);
            break;
#endif
        default:
            curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
            break;
    }

    if (config->follow_redirects) {
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, config->max_redirects);
    }

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, config->verify_peer ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, config->verify_host ? 2L : 0L);

    if (config->ca_cert) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, config->ca_cert);
    }

    if (config->proxy) {
        curl_easy_setopt(curl, CURLOPT_PROXY, config->proxy);
    }

    if (config->user_agent) {
        curl_easy_setopt(curl, CURLOPT_USERAGENT, config->user_agent);
    } else {
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Signalforge-NativeHttp-Client/" PHP_SIGNALFORGE_HTTP_CLIENT_VERSION);
    }

    curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 60L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);

    curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 128L * 1024L);
    curl_easy_setopt(curl, CURLOPT_UPLOAD_BUFFERSIZE, 128L * 1024L);

    if (share_handle) {
        curl_easy_setopt(curl, CURLOPT_SHARE, share_handle);
    }

    if (req->request->headers && req->request->header_count > 0) {
        struct curl_slist *list = NULL;
        for (size_t i = 0; i < req->request->header_count; i++) {
            char *header = malloc(strlen(req->request->headers[i].name) + strlen(req->request->headers[i].value) + 3);
            if (header) {
                sprintf(header, "%s: %s", req->request->headers[i].name, req->request->headers[i].value);
                list = curl_slist_append(list, header);
                free(header);
            }
        }
        if (list) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
            req->header_list = list;
        }
    }

    if (req->request->body && req->request->body_len > 0) {
        req->body_read_ctx.data = req->request->body;
        req->body_read_ctx.data_len = req->request->body_len;
        req->body_read_ctx.bytes_read = 0;

        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)req->request->body_len);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, request_body_read_callback);
        curl_easy_setopt(curl, CURLOPT_READDATA, &req->body_read_ctx);
    }

    return 0;
}

/* Process completed request */
static void process_completed_request(signalforge_active_request_t *req) {
    curl_easy_getinfo(req->curl_handle, CURLINFO_RESPONSE_CODE, &req->response->http_code);

    curl_easy_getinfo(req->curl_handle, CURLINFO_TOTAL_TIME, &req->response->total_time);
    curl_easy_getinfo(req->curl_handle, CURLINFO_NAMELOOKUP_TIME, &req->response->namelookup_time);
    curl_easy_getinfo(req->curl_handle, CURLINFO_CONNECT_TIME, &req->response->connect_time);
    curl_easy_getinfo(req->curl_handle, CURLINFO_PRETRANSFER_TIME, &req->response->pretransfer_time);
    curl_easy_getinfo(req->curl_handle, CURLINFO_STARTTRANSFER_TIME, &req->response->starttransfer_time);

    if (req->response_body && req->response_body_len > 0) {
        req->response->body = req->response_body;
        req->response->body_len = req->response_body_len;
        req->response_body = NULL;
        req->response_body_len = 0;
        req->response_body_capacity = 0;
    }

    parse_response_headers(req);

    req->response->request_id = req->request->request_id;
    req->response->user_data = req->request->user_data;
}

/* Cleanup active request */
static void cleanup_active_request(signalforge_active_request_t *req, signalforge_curl_handle_pool_t *handle_pool, int discard_handle) {
    if (req->curl_handle) {
        if (discard_handle) {
            signalforge_curl_handle_pool_discard(handle_pool, req->curl_handle);
        } else {
            signalforge_curl_handle_pool_release(handle_pool, req->curl_handle);
        }
        req->curl_handle = NULL;
    }

    if (req->header_list) {
        curl_slist_free_all(req->header_list);
        req->header_list = NULL;
    }

    free(req->response_body);
    req->response_body = NULL;

    free(req->response_headers);
    req->response_headers = NULL;

    req->body_read_ctx.data = NULL;
    req->body_read_ctx.data_len = 0;
    req->body_read_ctx.bytes_read = 0;
}

/**
 * Worker thread main function
 */
void *signalforge_client_worker_thread(void *arg) {
    signalforge_client_worker_t *worker = (signalforge_client_worker_t *)arg;
    signalforge_client_pool_t *pool = worker->pool;

    worker->multi_handle = curl_multi_init();
    if (!worker->multi_handle) {
        return NULL;
    }

    signalforge_curl_handle_pool_t *handle_pool = signalforge_curl_handle_pool_create();
    if (!handle_pool) {
        curl_multi_cleanup(worker->multi_handle);
        worker->multi_handle = NULL;
        return NULL;
    }

    curl_multi_setopt(worker->multi_handle, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);
    curl_multi_setopt(worker->multi_handle, CURLMOPT_MAX_HOST_CONNECTIONS, 6L);
    curl_multi_setopt(worker->multi_handle, CURLMOPT_MAXCONNECTS, 100L);

    while (!worker->shutdown) {
        signalforge_client_request_t *request = (signalforge_client_request_t *)
            signalforge_client_queue_pop(worker->request_queue, 100);

        if (request) {
            signalforge_active_request_t *active_req = calloc(1, sizeof(signalforge_active_request_t));
            if (active_req) {
                active_req->request = request;
                active_req->response = signalforge_client_response_create();

                active_req->response_body_capacity = 4096;
                active_req->response_body = malloc(active_req->response_body_capacity);
                active_req->response_headers_capacity = 1024;
                active_req->response_headers = malloc(active_req->response_headers_capacity);

                if (active_req->response && active_req->response_body && active_req->response_headers) {
                    CURLSH *share = signalforge_client_share_get_handle(pool->share);
                    if (setup_curl_handle(active_req, handle_pool, share, pool->config) == 0) {
                        curl_multi_add_handle(worker->multi_handle, active_req->curl_handle);

                        int retry_attempt = 0;
                        int should_retry = 0;
                        int discard_handle = 0;

                        do {
                            int still_running = 0;
                            CURLMcode mc;

                            do {
                                mc = curl_multi_perform(worker->multi_handle, &still_running);
                            } while (mc == CURLM_CALL_MULTI_PERFORM);

                            if (mc != CURLM_OK) {
                                active_req->response->is_error = 1;
                                active_req->response->curl_code = CURLE_FAILED_INIT;
                                break;
                            }

                            if (still_running) {
#ifdef HAVE_CURL_MULTI_POLL
                                mc = curl_multi_poll(worker->multi_handle, NULL, 0, 1000, NULL);
#else
                                mc = curl_multi_wait(worker->multi_handle, NULL, 0, 1000, NULL);
#endif
                                if (mc != CURLM_OK) {
                                    break;
                                }
                            }

                            int msgs_in_queue;
                            CURLMsg *msg;

                            while ((msg = curl_multi_info_read(worker->multi_handle, &msgs_in_queue))) {
                                if (msg->msg == CURLMSG_DONE) {
                                    active_req->response->curl_code = msg->data.result;

                                    if (msg->data.result != CURLE_OK) {
                                        active_req->response->is_error = 1;
                                        active_req->response->error_message = strdup(curl_easy_strerror(msg->data.result));

                                        switch (msg->data.result) {
                                            case CURLE_SSL_CONNECT_ERROR:
                                            case CURLE_SSL_CERTPROBLEM:
                                            case CURLE_SSL_CIPHER:
                                            case CURLE_SSL_CACERT:
                                            case CURLE_SSL_PINNEDPUBKEYNOTMATCH:
                                            case CURLE_RECV_ERROR:
                                            case CURLE_SEND_ERROR:
                                                discard_handle = 1;
                                                break;
                                            default:
                                                discard_handle = 0;
                                                break;
                                        }

                                        signalforge_client_config_t *cfg = (signalforge_client_config_t *)active_req->request->config;
                                        if (cfg && cfg->retry_config) {
                                            should_retry = signalforge_client_should_retry(
                                                cfg->retry_config,
                                                retry_attempt,
                                                msg->data.result,
                                                active_req->response->http_code
                                            );

                                            if (should_retry) {
                                                int delay = signalforge_client_retry_delay(cfg->retry_config, retry_attempt);
                                                usleep(delay * 1000);

                                                active_req->response_body_len = 0;
                                                active_req->response_headers_len = 0;
                                                active_req->body_read_ctx.bytes_read = 0;

                                                retry_attempt++;

                                                curl_multi_remove_handle(worker->multi_handle, active_req->curl_handle);
                                                curl_multi_add_handle(worker->multi_handle, active_req->curl_handle);
                                                still_running = 1;
                                            }
                                        }
                                    } else {
                                        should_retry = 0;
                                        discard_handle = 0;
                                    }
                                }
                            }
                        } while (should_retry);

                        curl_multi_remove_handle(worker->multi_handle, active_req->curl_handle);

                        process_completed_request(active_req);

                        signalforge_client_queue_push(pool->response_queue, active_req->response);

                        cleanup_active_request(active_req, handle_pool, discard_handle);
                        signalforge_client_request_destroy(active_req->request);
                        free(active_req);
                    } else {
                        active_req->response->is_error = 1;
                        active_req->response->curl_code = CURLE_FAILED_INIT;
                        signalforge_client_queue_push(pool->response_queue, active_req->response);

                        cleanup_active_request(active_req, handle_pool, 0);
                        signalforge_client_request_destroy(active_req->request);
                        free(active_req);
                    }
                } else {
                    if (active_req->response) {
                        signalforge_client_response_destroy(active_req->response);
                    }
                    free(active_req->response_body);
                    free(active_req->response_headers);
                    free(active_req);
                    signalforge_client_request_destroy(request);
                }
            } else {
                signalforge_client_request_destroy(request);
            }
        }
    }

    signalforge_curl_handle_pool_destroy(handle_pool);

    curl_multi_cleanup(worker->multi_handle);
    worker->multi_handle = NULL;

    return NULL;
}

#endif /* HAVE_SIGNALFORGE_HTTP_CLIENT */
