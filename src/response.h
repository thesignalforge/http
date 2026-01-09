/*
 * response.h
 *
 * Signalforge HTTP Response class internal header
 */

#ifndef SIGNALFORGE_RESPONSE_H
#define SIGNALFORGE_RESPONSE_H

#include "php_signalforge_http.h"
#include "stream.h"


/* ============================================================================
 * INTERNAL OBJECT STRUCTURE
 * ============================================================================ */

typedef struct _signalforge_response_object {
    /* Status */
    zend_long status_code;
    zend_string *reason_phrase;  // NULL = auto from status code
    
    /* Headers - stored as HashTable (lowercase keys) */
    HashTable *ht_headers;              // OWNED, must be freed
    
    /* Body */
    zval zv_body;                       // StreamInterface object or string
    zend_bool body_is_stream;           // Track if body is a stream
    
    /* Protocol version */
    zend_string *protocol_version;      // "1.1" or "1.0"

    /* Standard zend_object MUST be last */
    zend_object std;
} signalforge_response_object;

/* ============================================================================
 * OBJECT HELPERS
 * ============================================================================ */

/* Convert zend_object* to our internal struct */
static inline signalforge_response_object *signalforge_response_from_obj(zend_object *obj)
{
    return (signalforge_response_object *)((char *)(obj) - XtOffsetOf(signalforge_response_object, std));
}

/* Convenience macro for use in methods */
#define Z_SIGNALFORGE_RESPONSE_P(zv) signalforge_response_from_obj(Z_OBJ_P(zv))

/* ============================================================================
 * INITIALIZATION FUNCTIONS
 * ============================================================================ */

void signalforge_response_register_class(void);

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

const char *signalforge_get_reason_phrase(zend_long status_code);
zend_string *signalforge_serialize_headers(HashTable *headers);
signalforge_response_object *signalforge_response_clone(signalforge_response_object *src, zval *return_value);
zend_bool signalforge_validate_header_name(const char *name, size_t len);
zend_bool signalforge_validate_header_value(const char *value, size_t len);
zend_bool signalforge_validate_status_code(zend_long code);
zend_string *signalforge_normalize_header_name(const char *src, size_t src_len); // Shared with request.c

#endif /* SIGNALFORGE_RESPONSE_H */

