/*
  +----------------------------------------------------------------------+
  | Signalforge HTTP Extension - Shared CURL Connection Cache            |
  +----------------------------------------------------------------------+
  | Copyright (c) 2026 Signalforge                                       |
  +----------------------------------------------------------------------+
  | This source file is subject to MIT license.                          |
  +----------------------------------------------------------------------+
*/

#ifndef SIGNALFORGE_CLIENT_SHARE_H
#define SIGNALFORGE_CLIENT_SHARE_H

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT

#include <curl/curl.h>

#ifdef ZTS
#include <pthread.h>
#endif

/* Shared curl handle structure */
struct _signalforge_client_share {
    CURLSH *share_handle;
#ifdef ZTS
    /* Mutexes for thread-safe access to shared caches */
    pthread_mutex_t dns_mutex;
    pthread_mutex_t connect_mutex;
    pthread_mutex_t cookie_mutex;
    pthread_mutex_t ssl_session_mutex;
#endif
};

/* Function declarations */
signalforge_client_share_t *signalforge_client_share_create(void);
void signalforge_client_share_destroy(signalforge_client_share_t *share);
CURLSH *signalforge_client_share_get_handle(signalforge_client_share_t *share);

#endif /* HAVE_SIGNALFORGE_HTTP_CLIENT */

#endif /* SIGNALFORGE_CLIENT_SHARE_H */
