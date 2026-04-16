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
#ifdef PHP_WIN32
#include <io.h>
#define unlink _unlink
#else
#include <unistd.h>  /* For unlink() */
#endif
#include "src/request.h"
#include "src/response.h"
#include "src/stream.h"
#include "src/uploadedfile.h"
#include "src/uri.h"

/* Module globals declaration */
ZEND_DECLARE_MODULE_GLOBALS(signalforge_http)

/* ============================================================================
 * INI SETTINGS
 *
 * signalforge_http.max_body_size — bytes; 0 disables limit. Default 16 MiB.
 * Caps every read of php://input or stream contents to prevent memory
 * exhaustion from attacker-controlled body sizes. (audit H-H-4)
 * ============================================================================ */

PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY(
        "signalforge_http.max_body_size",
        "16777216",          /* 16 MiB */
        PHP_INI_ALL,
        OnUpdateLong,
        max_body_size,
        zend_signalforge_http_globals,
        signalforge_http_globals
    )
PHP_INI_END()

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

/* GINIT - zero-initialize module globals for defense-in-depth */
static PHP_GINIT_FUNCTION(signalforge_http)
{
#if defined(COMPILE_DL_SIGNALFORGE_HTTP) && defined(ZTS)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif
    memset(signalforge_http_globals, 0, sizeof(*signalforge_http_globals));
}

PHP_MINIT_FUNCTION(signalforge_http)
{
#ifdef ZTS
    ZEND_TSRMLS_CACHE_UPDATE();
#endif

    REGISTER_INI_ENTRIES();

    /* Register PSR-7 interfaces FIRST - classes depend on them */
    signalforge_register_psr7_interfaces();

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
    UNREGISTER_INI_ENTRIES();
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
    PHP_GINIT(signalforge_http),
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
