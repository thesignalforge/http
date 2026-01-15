/*
  +----------------------------------------------------------------------+
  | Signalforge HTTP Extension - Retry/Backoff Logic Implementation      |
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
#include "retry.h"
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

/**
 * Create a retry configuration
 */
signalforge_client_retry_config_t *signalforge_client_retry_config_create(
    int max_retries,
    int delay_ms,
    int max_delay_ms,
    double backoff_multiplier
) {
    signalforge_client_retry_config_t *config = malloc(sizeof(signalforge_client_retry_config_t));
    if (!config) {
        return NULL;
    }

    config->max_retries = max_retries;
    config->delay_ms = delay_ms;
    config->max_delay_ms = max_delay_ms > 0 ? max_delay_ms : 60000; /* Default 60s max */
    config->backoff_multiplier = backoff_multiplier > 0.0 ? backoff_multiplier : 2.0;

    /* Default retry conditions */
    config->retry_on_timeout = 1;
    config->retry_on_connect_error = 1;
    config->retry_on_5xx = 1;
    config->retry_on_429 = 1;

    return config;
}

/**
 * Destroy retry configuration
 */
void signalforge_client_retry_config_destroy(signalforge_client_retry_config_t *config) {
    if (config) {
        free(config);
    }
}

/**
 * Determine if a request should be retried
 */
int signalforge_client_should_retry(
    signalforge_client_retry_config_t *config,
    int attempt,
    CURLcode result,
    long http_code
) {
    if (!config || attempt >= config->max_retries) {
        return 0;
    }

    /* Check curl error codes */
    switch (result) {
        case CURLE_OK:
            /* Check HTTP status codes */
            if (config->retry_on_5xx && http_code >= 500 && http_code < 600) {
                return 1;
            }
            if (config->retry_on_429 && http_code == 429) {
                return 1;
            }
            return 0;

        case CURLE_OPERATION_TIMEDOUT:
            return config->retry_on_timeout;

        case CURLE_COULDNT_RESOLVE_HOST:
        case CURLE_COULDNT_RESOLVE_PROXY:
        case CURLE_COULDNT_CONNECT:
        case CURLE_SEND_ERROR:
        case CURLE_RECV_ERROR:
            return config->retry_on_connect_error;

        /* Network/SSL errors that might be transient */
        case CURLE_SSL_CONNECT_ERROR:
        case CURLE_PARTIAL_FILE:
        case CURLE_GOT_NOTHING:
            return config->retry_on_connect_error;

        /* Errors that should not be retried */
        case CURLE_URL_MALFORMAT:
        case CURLE_UNSUPPORTED_PROTOCOL:
        case CURLE_FAILED_INIT:
        case CURLE_OUT_OF_MEMORY:
        case CURLE_SSL_CERTPROBLEM:
        case CURLE_SSL_CIPHER:
        case CURLE_PEER_FAILED_VERIFICATION:
            return 0;

        default:
            /* Be conservative - don't retry unknown errors */
            return 0;
    }
}

/**
 * Calculate retry delay with exponential backoff
 * Returns delay in milliseconds
 */
int signalforge_client_retry_delay(
    signalforge_client_retry_config_t *config,
    int attempt
) {
    if (!config || attempt < 0) {
        return 0;
    }

    /* Calculate exponential backoff: delay * (multiplier ^ attempt) */
    double delay = config->delay_ms * pow(config->backoff_multiplier, attempt);

    /* Cap at max_delay_ms */
    if (delay > config->max_delay_ms) {
        delay = config->max_delay_ms;
    }

    return (int)delay;
}

#endif /* HAVE_SIGNALFORGE_HTTP_CLIENT */
