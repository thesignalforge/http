/*
 * uri.h
 *
 * Signalforge HTTP Uri class internal header
 * Implements PSR-7 UriInterface with RFC 3986 compliant parsing
 */

#ifndef SIGNALFORGE_URI_H
#define SIGNALFORGE_URI_H

#include "php_signalforge_http.h"

/* ============================================================================
 * INTERNAL OBJECT STRUCTURE
 * ============================================================================ */

typedef struct _signalforge_uri_object {
    /* URI Components (all OWNED, must be freed) */
    zend_string *scheme;        /* "http", "https", "" for relative */
    zend_string *user;          /* Username or NULL */
    zend_string *pass;          /* Password or NULL */
    zend_string *host;          /* "example.com", "[::1]" for IPv6 */
    zend_long port;             /* -1 = not specified, 0+ = explicit port */
    zend_string *path;          /* "/path/to/resource" */
    zend_string *query;         /* "key=value&foo=bar" (without ?) */
    zend_string *fragment;      /* "section" (without #) */

    /* Standard zend_object MUST be last */
    zend_object std;
} signalforge_uri_object;

/* ============================================================================
 * OBJECT HELPERS
 * ============================================================================ */

/* Convert zend_object* to our internal struct */
static inline signalforge_uri_object *signalforge_uri_from_obj(zend_object *obj)
{
    return (signalforge_uri_object *)((char *)(obj) - XtOffsetOf(signalforge_uri_object, std));
}

/* Convenience macro for use in methods */
#define Z_SIGNALFORGE_URI_P(zv) signalforge_uri_from_obj(Z_OBJ_P(zv))

/* ============================================================================
 * STANDARD PORTS
 * ============================================================================ */

#define SIGNALFORGE_PORT_HTTP    80
#define SIGNALFORGE_PORT_HTTPS   443
#define SIGNALFORGE_PORT_UNSET   -1

/* ============================================================================
 * INITIALIZATION FUNCTIONS
 * ============================================================================ */

void signalforge_uri_register_class(void);

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

/* Parse a URI string into components */
int signalforge_parse_uri(const char *uri, size_t len, signalforge_uri_object *result);

/* Check if port is standard for the given scheme */
static inline bool signalforge_is_standard_port(zend_string *scheme, zend_long port)
{
    if (port == SIGNALFORGE_PORT_UNSET) {
        return 1; /* Not specified = standard */
    }
    if (scheme == NULL || ZSTR_LEN(scheme) == 0) {
        return 0;
    }
    if (port == SIGNALFORGE_PORT_HTTP &&
        ZSTR_LEN(scheme) == 4 &&
        strncasecmp(ZSTR_VAL(scheme), "http", 4) == 0) {
        return 1;
    }
    if (port == SIGNALFORGE_PORT_HTTPS &&
        ZSTR_LEN(scheme) == 5 &&
        strncasecmp(ZSTR_VAL(scheme), "https", 5) == 0) {
        return 1;
    }
    return 0;
}

/* Clone a URI object */
signalforge_uri_object *signalforge_uri_clone(signalforge_uri_object *src, zval *return_value);

/* Create Uri from string (for internal use) */
zend_object *signalforge_uri_create_from_string(const char *uri, size_t len);

#endif /* SIGNALFORGE_URI_H */
