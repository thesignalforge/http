/*
 * php_signalforge_http.h
 *
 * Main header file for Signalforge HTTP Extension
 *
 * ZTS (Zend Thread Safety) Compatibility:
 * - All class entries are process-wide, initialized during MINIT
 * - Object handlers are static and immutable after MINIT
 * - Per-request data stored in object instances (not globals)
 * - ZEND_TSRMLS_CACHE properly defined and updated in lifecycle hooks
 * - Compatible with PHP 8.0+ ZTS builds (php-fpm worker threads)
 */

#ifndef PHP_SIGNALFORGE_HTTP_H
#define PHP_SIGNALFORGE_HTTP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"

extern zend_module_entry signalforge_http_module_entry;

/* Class entries for PSR-7 interface implementation */
extern zend_class_entry *signalforge_request_ce;
extern zend_class_entry *signalforge_response_ce;
extern zend_class_entry *signalforge_stream_ce;
extern zend_class_entry *signalforge_uploadedfile_ce;
extern zend_class_entry *signalforge_uri_ce;
#define phpext_signalforge_http_ptr &signalforge_http_module_entry

#define PHP_SIGNALFORGE_HTTP_VERSION "1.0.0"
#define PHP_SIGNALFORGE_HTTP_EXTNAME "signalforge_http"

#ifdef PHP_WIN32
#	define PHP_SIGNALFORGE_HTTP_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_SIGNALFORGE_HTTP_API __attribute__ ((visibility("default")))
#else
#	define PHP_SIGNALFORGE_HTTP_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

/* ============================================================================
 * Module Globals
 *
 * Per-request storage for streamforge integration and cleanup tracking.
 * Automatically thread-safe in ZTS builds, regular globals in non-ZTS builds.
 * ============================================================================ */

/* Maximum streamforge uploads to track for cleanup */
#define SIGNALFORGE_MAX_STREAMFORGE_UPLOADS 64

ZEND_BEGIN_MODULE_GLOBALS(signalforge_http)
    /* Streamforge integration */
    zend_bool streamforge_detected;     /* True if HTTP_X_STREAMFORGE=1 present */
    int streamforge_upload_count;       /* Number of uploads from streamforge */

    /* Temp file paths that need cleanup on RSHUTDOWN if not moved */
    char *streamforge_temp_paths[SIGNALFORGE_MAX_STREAMFORGE_UPLOADS];
    zend_bool streamforge_temp_moved[SIGNALFORGE_MAX_STREAMFORGE_UPLOADS];
ZEND_END_MODULE_GLOBALS(signalforge_http)

ZEND_EXTERN_MODULE_GLOBALS(signalforge_http)

/* Accessor macro - automatically handles ZTS and non-ZTS */
#define SIGNALFORGE_HTTP_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(signalforge_http, v)

/* Function declarations */
PHP_MINIT_FUNCTION(signalforge_http);
PHP_MSHUTDOWN_FUNCTION(signalforge_http);
PHP_RINIT_FUNCTION(signalforge_http);
PHP_RSHUTDOWN_FUNCTION(signalforge_http);
PHP_MINFO_FUNCTION(signalforge_http);

#endif /* PHP_SIGNALFORGE_HTTP_H */