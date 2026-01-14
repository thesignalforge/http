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
    php_info_print_table_row(2, "Streamforge Integration", "enabled");
    php_info_print_table_row(2, "Max Streamforge Uploads", ZEND_TOSTR(SIGNALFORGE_MAX_STREAMFORGE_UPLOADS));
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

    /* Register classes */
    signalforge_uri_register_class();  /* Uri first - Request depends on it */
    signalforge_request_register_class();
    signalforge_response_register_class();
    signalforge_stream_register_class();
    signalforge_uploadedfile_register_class();

    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(signalforge_http)
{
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
