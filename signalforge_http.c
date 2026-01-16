/*
 * signalforge_http.c
 *
 * Signalforge HTTP Extension - Main Module File
 *
 * This is the module entry point. It handles:
 * - Extension registration with PHP
 * - Module initialization/shutdown
 * - Request initialization/shutdown
 * - phpinfo() output
 *
 * PSR Standards Implemented:
 * - PSR-7:  HTTP Message interfaces (Request, Response, Stream, etc.)
 * - PSR-17: HTTP Factory interfaces (always enabled)
 * - PSR-18: HTTP Client interface (requires ZTS + libcurl)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_signalforge_http.h"
#include "src/psr7_interfaces.h"
#include <unistd.h>  /* For unlink() */
#include "src/request.h"
#include "src/response.h"
#include "src/stream.h"
#include "src/uploadedfile.h"
#include "src/uri.h"
#include "src/factories/factories.h"

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT
#include <curl/curl.h>
#include "src/client/client.h"
#endif

/* Module globals declaration */
ZEND_DECLARE_MODULE_GLOBALS(signalforge_http)

/* ============================================================================
 * (No global interned strings currently used)
 * ============================================================================ */

/* ============================================================================
 * MODULE INFORMATION
 * ============================================================================ */

PHP_MINFO_FUNCTION(signalforge_http)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "signalforge_http support", "enabled");
    php_info_print_table_row(2, "Version", PHP_SIGNALFORGE_HTTP_VERSION);
    php_info_print_table_row(2, "Target SAPI", "php-fpm (FastCGI)");
    php_info_print_table_row(2, "ZTS Support",
#ifdef ZTS
        "enabled"
#else
        "disabled"
#endif
    );
    php_info_print_table_end();

    /* PSR Standards Support */
    php_info_print_table_start();
    php_info_print_table_header(2, "PSR Standards", "Status");
    php_info_print_table_row(2, "PSR-7 (HTTP Message)", "enabled");
    php_info_print_table_row(2, "PSR-17 (HTTP Factories)", "enabled");
#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT
    php_info_print_table_row(2, "PSR-18 (HTTP Client)", "enabled");
#else
    php_info_print_table_row(2, "PSR-18 (HTTP Client)", "disabled (requires libcurl)");
#endif
    php_info_print_table_end();

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT
    /* HTTP Client Configuration */
    php_info_print_table_start();
    php_info_print_table_header(2, "HTTP Client (PSR-18)", "Value");
    php_info_print_table_row(2, "libcurl version", curl_version());
    php_info_print_table_row(2, "Default pool size", ZEND_TOSTR(SIGNALFORGE_CLIENT_DEFAULT_POOL_SIZE));
    php_info_print_table_row(2, "Default timeout", ZEND_TOSTR(SIGNALFORGE_CLIENT_DEFAULT_TIMEOUT) "s");
    php_info_print_table_row(2, "Default connect timeout", ZEND_TOSTR(SIGNALFORGE_CLIENT_DEFAULT_CONNECT_TIMEOUT) "s");
#ifdef HAVE_HTTP3
    php_info_print_table_row(2, "HTTP/3 Support", "enabled");
#else
    php_info_print_table_row(2, "HTTP/3 Support", "disabled");
#endif
    php_info_print_table_end();
#endif

    /* Streamforge Integration */
    php_info_print_table_start();
    php_info_print_table_header(2, "Streamforge Integration", "Value");
    php_info_print_table_row(2, "Status", "enabled");
    php_info_print_table_row(2, "Max uploads per request", ZEND_TOSTR(SIGNALFORGE_MAX_STREAMFORGE_UPLOADS));
    php_info_print_table_end();
}

/* ============================================================================
 * MODULE LIFECYCLE
 * ============================================================================ */

/* GINIT/GSHUTDOWN not needed - no meaningful globals to initialize */

PHP_MINIT_FUNCTION(signalforge_http)
{
#ifdef ZTS
    ZEND_TSRMLS_CACHE_UPDATE();
#endif

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT
    /* Initialize libcurl globally (must be done before any curl operations) */
    if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
        php_error_docref(NULL, E_WARNING, "Failed to initialize libcurl");
        return FAILURE;
    }
#endif

    /* Register PSR-7 interfaces first (classes implement these) */
    signalforge_register_psr7_interfaces();

    /* Register PSR-17 factory interfaces */
    signalforge_register_psr17_interfaces();

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT
    /* Register PSR-18 client interfaces */
    signalforge_register_psr18_interfaces();
#endif

    /* Register PSR-7 implementation classes */
    signalforge_uri_register_class();  /* Uri first - Request depends on it */
    signalforge_request_register_class();
    signalforge_response_register_class();
    signalforge_stream_register_class();
    signalforge_uploadedfile_register_class();

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT
    /* Register PSR-18 client classes */
    signalforge_client_exception_register_classes();  /* Exceptions first */
    signalforge_client_register_class();
    signalforge_http_request_pool_register_class();
#endif

    /* Register PSR-17 factory classes */
    signalforge_factories_register_classes();

    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(signalforge_http)
{
#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT
    /* Clean up libcurl global state */
    curl_global_cleanup();
#endif

    return SUCCESS;
}

PHP_RINIT_FUNCTION(signalforge_http)
{
#ifdef ZTS
    ZEND_TSRMLS_CACHE_UPDATE();
#endif

    /* Initialize streamforge state for this request */
    SIGNALFORGE_HTTP_G(streamforge_detected) = 0;
    SIGNALFORGE_HTTP_G(streamforge_upload_count) = 0;

    /* Clear temp path tracking arrays */
    for (int i = 0; i < SIGNALFORGE_MAX_STREAMFORGE_UPLOADS; i++) {
        SIGNALFORGE_HTTP_G(streamforge_temp_paths)[i] = NULL;
        SIGNALFORGE_HTTP_G(streamforge_temp_moved)[i] = 0;
    }

    return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(signalforge_http)
{
#ifdef ZTS
    ZEND_TSRMLS_CACHE_UPDATE();
#endif

    /*
     * Clean up streamforge temp files that weren't moved.
     *
     * When streamforge handles uploads, it writes files to temp paths. If the
     * PHP application doesn't call moveTo() on an uploaded file, we need to
     * delete the temp file to prevent disk space leaks.
     *
     * Files that were moved have streamforge_temp_moved[i] = 1.
     */
    for (int i = 0; i < SIGNALFORGE_HTTP_G(streamforge_upload_count); i++) {
        if (SIGNALFORGE_HTTP_G(streamforge_temp_paths)[i] != NULL) {
            /* Delete if not moved */
            if (!SIGNALFORGE_HTTP_G(streamforge_temp_moved)[i]) {
                unlink(SIGNALFORGE_HTTP_G(streamforge_temp_paths)[i]);
            }

            /* Free the path string */
            efree(SIGNALFORGE_HTTP_G(streamforge_temp_paths)[i]);
            SIGNALFORGE_HTTP_G(streamforge_temp_paths)[i] = NULL;
        }
    }

    /* Reset state */
    SIGNALFORGE_HTTP_G(streamforge_detected) = 0;
    SIGNALFORGE_HTTP_G(streamforge_upload_count) = 0;

    return SUCCESS;
}

/* ============================================================================
 * MODULE ENTRY
 * ============================================================================ */

zend_module_entry signalforge_http_module_entry = {
    STANDARD_MODULE_HEADER,
    PHP_SIGNALFORGE_HTTP_EXTNAME,
    NULL,                    /* Functions */
    PHP_MINIT(signalforge_http),
    PHP_MSHUTDOWN(signalforge_http),
    PHP_RINIT(signalforge_http),
    PHP_RSHUTDOWN(signalforge_http),
    PHP_MINFO(signalforge_http),
    PHP_SIGNALFORGE_HTTP_VERSION,
    PHP_MODULE_GLOBALS(signalforge_http),
    NULL,  /* No GINIT needed */
    NULL,  /* No GSHUTDOWN needed */
    NULL,
    STANDARD_MODULE_PROPERTIES_EX
};

#ifdef COMPILE_DL_SIGNALFORGE_HTTP
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(signalforge_http)
#endif
