/*
 * request.h
 *
 * Signalforge HTTP Request class internal header
 */

#ifndef SIGNALFORGE_REQUEST_H
#define SIGNALFORGE_REQUEST_H

#include "php_signalforge_http.h"

/* ============================================================================
 * FLAGS
 * ============================================================================ */

#define SF_REQ_FLAG_BODY_READ        (1 << 0)
#define SF_REQ_FLAG_JSON_PARSED      (1 << 1)
#define SF_REQ_FLAG_PATH_PARSED      (1 << 2)
#define SF_REQ_FLAG_METHOD_RESOLVED   (1 << 3)
#define SF_REQ_FLAG_INPUT_MERGED     (1 << 4)
#define SF_REQ_FLAG_CTYPE_PARSED     (1 << 5)
#define SF_REQ_FLAG_HEADERS_EXTRACTED (1 << 6)

/* ============================================================================
 * INTERNAL OBJECT STRUCTURE
 * ============================================================================ */

typedef struct _signalforge_request_object {
    /* Refcounted zvals holding superglobal array references */
    zval zv_server;              // $_SERVER reference
    zval zv_get;                 // $_GET reference
    zval zv_post;                // $_POST reference
    zval zv_cookie;              // $_COOKIE reference
    zval zv_files;               // $_FILES reference
    
    /* Owned HashTables */
    HashTable *ht_headers;      // Normalized headers (lowercase keys)
    HashTable *ht_input;         // Merged input (JSON/POST/GET)
    HashTable *ht_attributes;    // Request attributes (PSR-7)
    
    /* Lazy-cached zvals (refcounted) */
    zval zv_body;                // Raw body string
    zval zv_json;                // Parsed JSON
    zval zv_path;                // Path component
    zval zv_method;              // Resolved method
    zval zv_content_type;        // MIME type (without params)
    zval zv_request_method;      // Original REQUEST_METHOD
    zval zv_method_override_header; // X-HTTP-Method-Override
    zval zv_method_override_post;   // _method POST field
    zval zv_uri;                  // Full URI string
    zval zv_query_string;         // Query string
    
    /* Direct SAPI strings (NOT owned, from SAPI globals) */
    const char *request_uri;
    size_t request_uri_len;
    const char *query_string;
    size_t query_string_len;
    const char *request_method;
    size_t request_method_len;
    
    /* Protocol version */
    zend_string *protocol_version; // "1.1" or "1.0"
    
    /* State flags */
    uint8_t flags;
    
    /* Standard zend_object MUST be last member */
    zend_object std;
} signalforge_request_object;

/* ============================================================================
 * OBJECT HELPERS
 * ============================================================================ */

/* Convert zend_object* to our internal struct */
static inline signalforge_request_object *signalforge_request_from_obj(zend_object *obj)
{
    return (signalforge_request_object *)((char *)(obj) - XtOffsetOf(signalforge_request_object, std));
}

/* Convenience macro for use in methods */
#define Z_SIGNALFORGE_REQUEST_P(zv) signalforge_request_from_obj(Z_OBJ_P(zv))

/* ============================================================================
 * INITIALIZATION FUNCTIONS
 * ============================================================================ */

void signalforge_request_register_class(void);

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

HashTable *signalforge_extract_headers(HashTable *server);
zend_string *signalforge_normalize_header_name(const char *src, size_t src_len);
zend_string *signalforge_read_body(signalforge_request_object *intern);
void signalforge_parse_path(signalforge_request_object *intern);
void signalforge_resolve_method(signalforge_request_object *intern);
void signalforge_parse_content_type(signalforge_request_object *intern);
HashTable *signalforge_merge_input(signalforge_request_object *intern);
zval *signalforge_hash_get(HashTable *ht, const char *key, size_t key_len);
signalforge_request_object *signalforge_request_clone(signalforge_request_object *src, zval *return_value);
zend_bool signalforge_validate_header_name(const char *name, size_t len);

#endif /* SIGNALFORGE_REQUEST_H */

