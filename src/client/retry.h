/*
  +----------------------------------------------------------------------+
  | Signalforge HTTP Extension - Retry/Backoff Logic                     |
  +----------------------------------------------------------------------+
  | Copyright (c) 2026 Signalforge                                       |
  +----------------------------------------------------------------------+
  | This source file is subject to MIT license.                          |
  +----------------------------------------------------------------------+
*/

#ifndef SIGNALFORGE_CLIENT_RETRY_H
#define SIGNALFORGE_CLIENT_RETRY_H

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT

#include <curl/curl.h>
#include "client.h"

/* Function declarations */
signalforge_client_retry_config_t *signalforge_client_retry_config_create(
    int max_retries,
    int delay_ms,
    int max_delay_ms,
    double backoff_multiplier
);
void signalforge_client_retry_config_destroy(signalforge_client_retry_config_t *config);
int signalforge_client_should_retry(signalforge_client_retry_config_t *config, int attempt, CURLcode result, long http_code);
int signalforge_client_retry_delay(signalforge_client_retry_config_t *config, int attempt);

#endif /* HAVE_SIGNALFORGE_HTTP_CLIENT */

#endif /* SIGNALFORGE_CLIENT_RETRY_H */
