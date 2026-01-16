/*
  +----------------------------------------------------------------------+
  | Signalforge HTTP Extension - Shared CURL Connection Cache            |
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
#include "share.h"
#include <stdlib.h>

#ifdef ZTS
/* Lock callback for curl share - only needed in threaded mode */
static void curl_share_lock(CURL *handle, curl_lock_data data, curl_lock_access access, void *userptr) {
    signalforge_client_share_t *share = (signalforge_client_share_t *)userptr;
    (void)handle;
    (void)access;

    switch (data) {
        case CURL_LOCK_DATA_DNS:
            pthread_mutex_lock(&share->dns_mutex);
            break;
        case CURL_LOCK_DATA_CONNECT:
            pthread_mutex_lock(&share->connect_mutex);
            break;
        case CURL_LOCK_DATA_COOKIE:
            pthread_mutex_lock(&share->cookie_mutex);
            break;
        case CURL_LOCK_DATA_SSL_SESSION:
            pthread_mutex_lock(&share->ssl_session_mutex);
            break;
        default:
            break;
    }
}

/* Unlock callback for curl share - only needed in threaded mode */
static void curl_share_unlock(CURL *handle, curl_lock_data data, void *userptr) {
    signalforge_client_share_t *share = (signalforge_client_share_t *)userptr;
    (void)handle;

    switch (data) {
        case CURL_LOCK_DATA_DNS:
            pthread_mutex_unlock(&share->dns_mutex);
            break;
        case CURL_LOCK_DATA_CONNECT:
            pthread_mutex_unlock(&share->connect_mutex);
            break;
        case CURL_LOCK_DATA_COOKIE:
            pthread_mutex_unlock(&share->cookie_mutex);
            break;
        case CURL_LOCK_DATA_SSL_SESSION:
            pthread_mutex_unlock(&share->ssl_session_mutex);
            break;
        default:
            break;
    }
}
#endif /* ZTS */

/**
 * Create a new shared curl handle
 *
 * In ZTS mode, mutexes are used to protect shared caches.
 * In non-ZTS mode, no mutexes are needed (single-threaded).
 */
signalforge_client_share_t *signalforge_client_share_create(void) {
    signalforge_client_share_t *share = malloc(sizeof(signalforge_client_share_t));
    if (!share) {
        return NULL;
    }

#ifdef ZTS
    /* Initialize mutexes for thread-safe access */
    if (pthread_mutex_init(&share->dns_mutex, NULL) != 0) {
        free(share);
        return NULL;
    }

    if (pthread_mutex_init(&share->connect_mutex, NULL) != 0) {
        pthread_mutex_destroy(&share->dns_mutex);
        free(share);
        return NULL;
    }

    if (pthread_mutex_init(&share->cookie_mutex, NULL) != 0) {
        pthread_mutex_destroy(&share->connect_mutex);
        pthread_mutex_destroy(&share->dns_mutex);
        free(share);
        return NULL;
    }

    if (pthread_mutex_init(&share->ssl_session_mutex, NULL) != 0) {
        pthread_mutex_destroy(&share->cookie_mutex);
        pthread_mutex_destroy(&share->connect_mutex);
        pthread_mutex_destroy(&share->dns_mutex);
        free(share);
        return NULL;
    }
#endif

    /* Create curl share handle */
    share->share_handle = curl_share_init();
    if (!share->share_handle) {
#ifdef ZTS
        pthread_mutex_destroy(&share->ssl_session_mutex);
        pthread_mutex_destroy(&share->cookie_mutex);
        pthread_mutex_destroy(&share->connect_mutex);
        pthread_mutex_destroy(&share->dns_mutex);
#endif
        free(share);
        return NULL;
    }

#ifdef ZTS
    /* Configure lock callbacks for thread-safe access */
    curl_share_setopt(share->share_handle, CURLSHOPT_LOCKFUNC, curl_share_lock);
    curl_share_setopt(share->share_handle, CURLSHOPT_UNLOCKFUNC, curl_share_unlock);
    curl_share_setopt(share->share_handle, CURLSHOPT_USERDATA, share);
#endif

    /* Share DNS cache */
    curl_share_setopt(share->share_handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);

    /* Share connection cache */
    curl_share_setopt(share->share_handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);

    /* Share SSL session cache */
    curl_share_setopt(share->share_handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);

    return share;
}

/**
 * Destroy the shared curl handle
 */
void signalforge_client_share_destroy(signalforge_client_share_t *share) {
    if (!share) {
        return;
    }

    if (share->share_handle) {
        curl_share_cleanup(share->share_handle);
    }

#ifdef ZTS
    pthread_mutex_destroy(&share->ssl_session_mutex);
    pthread_mutex_destroy(&share->cookie_mutex);
    pthread_mutex_destroy(&share->connect_mutex);
    pthread_mutex_destroy(&share->dns_mutex);
#endif

    free(share);
}

/**
 * Get the CURLSH handle
 */
CURLSH *signalforge_client_share_get_handle(signalforge_client_share_t *share) {
    return share ? share->share_handle : NULL;
}

#endif /* HAVE_SIGNALFORGE_HTTP_CLIENT */
