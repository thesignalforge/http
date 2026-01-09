/*
 * stream.h
 *
 * Signalforge HTTP Stream class internal header
 */

#ifndef SIGNALFORGE_STREAM_H
#define SIGNALFORGE_STREAM_H

#include "php_signalforge_http.h"

/* ============================================================================
 * INTERNAL OBJECT STRUCTURE
 * ============================================================================ */

typedef struct _signalforge_stream_object {
    /* Stream resource or string */
    zval zv_resource;                   // PHP stream resource
    zend_string *string_data;           // If created from string (zero-copy reference)
    
    /* Position tracking */
    zend_long position;
    zend_long size;
    
    /* Capabilities */
    zend_bool readable;
    zend_bool writable;
    zend_bool seekable;
    
    /* Metadata cache */
    HashTable *ht_metadata;             // OWNED, must be freed
    zend_bool metadata_loaded;
    
    /* Standard zend_object MUST be last */
    zend_object std;
} signalforge_stream_object;

/* ============================================================================
 * OBJECT HELPERS
 * ============================================================================ */

/* Convert zend_object* to our internal struct */
static inline signalforge_stream_object *signalforge_stream_from_obj(zend_object *obj)
{
    return (signalforge_stream_object *)((char *)(obj) - XtOffsetOf(signalforge_stream_object, std));
}

/* Convenience macro for use in methods */
#define Z_SIGNALFORGE_STREAM_P(zv) signalforge_stream_from_obj(Z_OBJ_P(zv))

/* ============================================================================
 * INITIALIZATION FUNCTIONS
 * ============================================================================ */

void signalforge_stream_register_class(void);

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

php_stream *signalforge_get_php_stream(signalforge_stream_object *intern);
void signalforge_load_metadata(signalforge_stream_object *intern);

#endif /* SIGNALFORGE_STREAM_H */

