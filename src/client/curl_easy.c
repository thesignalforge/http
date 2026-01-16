/*
  +----------------------------------------------------------------------+
  | Signalforge HTTP Extension - Synchronous CURL Execution              |
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
#include "curl_easy.h"
#include "request_data.h"
#include "response_data.h"
#include "retry.h"
#include "share.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PHP_SIGNALFORGE_HTTP_CLIENT_VERSION "1.0.0"

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
    signalforge_curl_context_t *ctx = (signalforge_curl_context_t *)userdata;
    size_t total_size = size * nmemb;

    if (ctx->response_body_len + total_size > ctx->response_body_capacity) {
        size_t new_capacity = ctx->response_body_capacity * 2;
        if (new_capacity < ctx->response_body_len + total_size) {
            new_capacity = ctx->response_body_len + total_size + 4096;
        }

        char *new_body = erealloc(ctx->response_body, new_capacity);
        if (!new_body) {
            return 0;
        }

        ctx->response_body = new_body;
        ctx->response_body_capacity = new_capacity;
    }

    memcpy(ctx->response_body + ctx->response_body_len, ptr, total_size);
    ctx->response_body_len += total_size;

    return total_size;
}

/* Header callback for response headers */
static size_t header_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    signalforge_curl_context_t *ctx = (signalforge_curl_context_t *)userdata;
    size_t total_size = size * nmemb;

    if (ctx->response_headers_len + total_size > ctx->response_headers_capacity) {
        size_t new_capacity = ctx->response_headers_capacity * 2;
        if (new_capacity < ctx->response_headers_len + total_size) {
            new_capacity = ctx->response_headers_len + total_size + 1024;
        }

        char *new_headers = erealloc(ctx->response_headers, new_capacity);
        if (!new_headers) {
            return 0;
        }

        ctx->response_headers = new_headers;
        ctx->response_headers_capacity = new_capacity;
    }

    memcpy(ctx->response_headers + ctx->response_headers_len, ptr, total_size);
    ctx->response_headers_len += total_size;

    return total_size;
}

/* Parse response headers */
void signalforge_curl_parse_headers(signalforge_curl_context_t *ctx) {
    if (!ctx->response_headers || ctx->response_headers_len == 0) {
        return;
    }

    size_t header_count = 0;
    char *ptr = ctx->response_headers;
    char *end = ctx->response_headers + ctx->response_headers_len;

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

    ctx->response->headers = emalloc(sizeof(signalforge_client_header_t) * header_count);
    if (!ctx->response->headers) {
        return;
    }

    size_t idx = 0;
    ptr = ctx->response_headers;

    while (ptr < end && idx < header_count) {
        char *line_end = memchr(ptr, '\n', end - ptr);
        if (!line_end) break;

        char *colon = memchr(ptr, ':', line_end - ptr);
        if (colon) {
            size_t name_len = colon - ptr;
            char *name = emalloc(name_len + 1);
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

                char *value = emalloc(value_len + 1);
                if (value) {
                    memcpy(value, value_start, value_len);
                    value[value_len] = '\0';

                    ctx->response->headers[idx].name = name;
                    ctx->response->headers[idx].value = value;
                    idx++;
                } else {
                    efree(name);
                }
            }
        }

        ptr = line_end + 1;
    }

    ctx->response->header_count = idx;
}

/* Setup curl handle with request data */
int signalforge_curl_setup(
    CURL *curl,
    signalforge_curl_context_t *ctx,
    CURLSH *share,
    signalforge_client_config_t *config
) {
    if (!curl || !ctx || !ctx->request) {
        return -1;
    }

    ctx->curl_handle = curl;

    /* URL and method */
    curl_easy_setopt(curl, CURLOPT_URL, ctx->request->url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, ctx->request->method);

    /* Response callbacks */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, ctx);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, ctx);

    /* Timeouts */
    if (config->connect_timeout > 0) {
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, config->connect_timeout);
    }
    if (config->timeout > 0) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, config->timeout);
    }

    /* HTTP version */
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

    /* Redirects */
    if (config->follow_redirects) {
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, config->max_redirects);
    } else {
        /* Explicitly disable redirect following */
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    }

    /* SSL */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, config->verify_peer ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, config->verify_host ? 2L : 0L);

    if (config->ca_cert) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, config->ca_cert);
    }

    /* Proxy */
    if (config->proxy) {
        curl_easy_setopt(curl, CURLOPT_PROXY, config->proxy);
    }

    /* User agent */
    if (config->user_agent) {
        curl_easy_setopt(curl, CURLOPT_USERAGENT, config->user_agent);
    } else {
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Signalforge-NativeHttp-Client/" PHP_SIGNALFORGE_HTTP_CLIENT_VERSION);
    }

    /* TCP optimizations */
    curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 60L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);

    /* Buffer sizes */
    curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 128L * 1024L);
    curl_easy_setopt(curl, CURLOPT_UPLOAD_BUFFERSIZE, 128L * 1024L);

    /* Enable automatic decompression - empty string means accept all supported encodings */
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");

    /* Shared handle for connection pooling */
    if (share) {
        curl_easy_setopt(curl, CURLOPT_SHARE, share);
    }

    /* Request headers */
    if (ctx->request->headers && ctx->request->header_count > 0) {
        struct curl_slist *list = NULL;
        for (size_t i = 0; i < ctx->request->header_count; i++) {
            size_t header_len = strlen(ctx->request->headers[i].name) +
                               strlen(ctx->request->headers[i].value) + 3;
            char *header = emalloc(header_len);
            if (header) {
                snprintf(header, header_len, "%s: %s",
                        ctx->request->headers[i].name,
                        ctx->request->headers[i].value);
                list = curl_slist_append(list, header);
                efree(header);
            }
        }
        if (list) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
            ctx->header_list = list;
        }
    }

    /* Request body - handle differently for POST vs PUT/PATCH */
    if (ctx->request->body && ctx->request->body_len > 0) {
        /* For POST, use CURLOPT_POSTFIELDS directly (not READFUNCTION)
         * CURLOPT_READFUNCTION only works with CURLOPT_UPLOAD */
        if (strcmp(ctx->request->method, "POST") == 0) {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, ctx->request->body);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)ctx->request->body_len);
        } else {
            /* PUT, PATCH, or other methods with body - use streaming approach */
            ctx->body_read_ctx.data = ctx->request->body;
            ctx->body_read_ctx.data_len = ctx->request->body_len;
            ctx->body_read_ctx.bytes_read = 0;

            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)ctx->request->body_len);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, request_body_read_callback);
            curl_easy_setopt(curl, CURLOPT_READDATA, &ctx->body_read_ctx);
        }
    }

    return 0;
}

/* Process curl response after execution */
void signalforge_curl_process_response(CURL *curl, signalforge_curl_context_t *ctx) {
    /* Get HTTP response code */
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &ctx->response->http_code);

    /* Get timing information */
    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &ctx->response->total_time);
    curl_easy_getinfo(curl, CURLINFO_NAMELOOKUP_TIME, &ctx->response->namelookup_time);
    curl_easy_getinfo(curl, CURLINFO_CONNECT_TIME, &ctx->response->connect_time);
    curl_easy_getinfo(curl, CURLINFO_PRETRANSFER_TIME, &ctx->response->pretransfer_time);
    curl_easy_getinfo(curl, CURLINFO_STARTTRANSFER_TIME, &ctx->response->starttransfer_time);

    /* Transfer body ownership to response */
    if (ctx->response_body && ctx->response_body_len > 0) {
        ctx->response->body = ctx->response_body;
        ctx->response->body_len = ctx->response_body_len;
        ctx->response_body = NULL;
        ctx->response_body_len = 0;
        ctx->response_body_capacity = 0;
    }

    /* Parse headers */
    signalforge_curl_parse_headers(ctx);

    /* Copy request metadata */
    if (ctx->request) {
        ctx->response->request_id = ctx->request->request_id;
        ctx->response->user_data = ctx->request->user_data;
    }
}

/* Cleanup execution context */
void signalforge_curl_cleanup_context(signalforge_curl_context_t *ctx) {
    if (!ctx) return;

    if (ctx->header_list) {
        curl_slist_free_all(ctx->header_list);
        ctx->header_list = NULL;
    }

    if (ctx->response_body) {
        efree(ctx->response_body);
        ctx->response_body = NULL;
    }
    ctx->response_body_len = 0;
    ctx->response_body_capacity = 0;

    if (ctx->response_headers) {
        efree(ctx->response_headers);
        ctx->response_headers = NULL;
    }
    ctx->response_headers_len = 0;
    ctx->response_headers_capacity = 0;

    ctx->body_read_ctx.data = NULL;
    ctx->body_read_ctx.data_len = 0;
    ctx->body_read_ctx.bytes_read = 0;
}

/* Reset context for retry */
void signalforge_curl_reset_context(signalforge_curl_context_t *ctx) {
    if (!ctx) return;

    /* Clear response accumulators but keep allocations */
    ctx->response_body_len = 0;
    ctx->response_headers_len = 0;

    /* Reset body read position for re-send */
    ctx->body_read_ctx.bytes_read = 0;
}

/* Execute HTTP request synchronously */
signalforge_client_response_t *signalforge_curl_easy_execute(
    signalforge_client_request_t *request,
    signalforge_client_share_t *share,
    signalforge_client_config_t *config
) {
    if (!request || !config) {
        return NULL;
    }

    /* Create curl handle */
    CURL *curl = curl_easy_init();
    if (!curl) {
        return NULL;
    }

    /* Create response object */
    signalforge_client_response_t *response = signalforge_client_response_create();
    if (!response) {
        curl_easy_cleanup(curl);
        return NULL;
    }

    /* Initialize execution context */
    signalforge_curl_context_t ctx = {0};
    ctx.request = request;
    ctx.response = response;

    /* Allocate response buffers */
    ctx.response_body_capacity = 4096;
    ctx.response_body = emalloc(ctx.response_body_capacity);
    ctx.response_headers_capacity = 1024;
    ctx.response_headers = emalloc(ctx.response_headers_capacity);

    if (!ctx.response_body || !ctx.response_headers) {
        if (ctx.response_body) efree(ctx.response_body);
        if (ctx.response_headers) efree(ctx.response_headers);
        signalforge_client_response_destroy(response);
        curl_easy_cleanup(curl);
        return NULL;
    }

    /* Get CURLSH handle if available */
    CURLSH *share_handle = share ? signalforge_client_share_get_handle(share) : NULL;

    /* Setup curl handle */
    if (signalforge_curl_setup(curl, &ctx, share_handle, config) != 0) {
        signalforge_curl_cleanup_context(&ctx);
        signalforge_client_response_destroy(response);
        curl_easy_cleanup(curl);
        return NULL;
    }

    /* Execute with retry logic */
    int retry_count = 0;
    int max_retries = config->retry_config ? config->retry_config->max_retries : 0;
    CURLcode result;
    int discard_handle = 0;

    do {
        result = curl_easy_perform(curl);

        if (result == CURLE_OK) {
            /* Success - process response */
            signalforge_curl_process_response(curl, &ctx);
            break;
        }

        /* Error occurred */
        response->is_error = 1;
        response->curl_code = result;

        /* Check if handle should be discarded (connection issues) */
        switch (result) {
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

        /* Check if we should retry */
        if (config->retry_config &&
            signalforge_client_should_retry(config->retry_config, retry_count, result, 0)) {

            int delay = signalforge_client_retry_delay(config->retry_config, retry_count);
            usleep(delay * 1000);

            /* Reset for retry */
            signalforge_curl_reset_context(&ctx);
            curl_easy_reset(curl);

            /* Re-setup curl handle */
            if (signalforge_curl_setup(curl, &ctx, share_handle, config) != 0) {
                break;
            }

            response->is_error = 0;
            response->curl_code = 0;
            retry_count++;
        } else {
            /* No retry - set error message */
            response->error_message = estrdup(curl_easy_strerror(result));
            break;
        }

    } while (retry_count <= max_retries);

    /* Cleanup */
    signalforge_curl_cleanup_context(&ctx);
    curl_easy_cleanup(curl);
    (void)discard_handle; /* Not used in non-pooled mode */

    return response;
}

#endif /* HAVE_SIGNALFORGE_HTTP_CLIENT */
