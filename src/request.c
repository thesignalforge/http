/*
 * Signalforge HTTP Request Class Implementation
 *
 * This file implements the Signalforge\Http\Request class, providing
 * high-performance access to HTTP request data with zero-copy operations
 * and full PSR-7 compliance.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_signalforge_http.h"
#include "psr7_interfaces.h"
#include "request.h"
#include "response.h"  // For signalforge_validate_header_name
#include "stream.h"
#include "uploadedfile.h"
#include "uri.h"
#include "zend_smart_str.h"
#include "ext/json/php_json.h"
#include <sys/stat.h>
#include <ctype.h>

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

zend_class_entry *signalforge_request_ce = NULL;
static zend_object_handlers signalforge_request_object_handlers;

/* SPL Exception classes (available in PHP 8.3) */
extern PHPAPI zend_class_entry *spl_ce_InvalidArgumentException;
extern PHPAPI zend_class_entry *spl_ce_RuntimeException;

/* Forward declaration */
extern zend_class_entry *signalforge_stream_ce;

/* Helper: Convert string to uppercase in-place (PHP 8.4+ compatibility)
 * php_strtoupper was removed in PHP 8.4, so we implement our own version.
 */
static inline void signalforge_strtoupper(char *str, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        str[i] = toupper((unsigned char)str[i]);
    }
}

/* ============================================================================
 * OBJECT HANDLERS
 * ============================================================================ */

static zend_object *signalforge_request_create_object(zend_class_entry *ce)
{
    signalforge_request_object *intern;

    intern = zend_object_alloc(sizeof(signalforge_request_object), ce);

    /* Initialize superglobal zvals to UNDEF */
    ZVAL_UNDEF(&intern->zv_server);
    ZVAL_UNDEF(&intern->zv_get);
    ZVAL_UNDEF(&intern->zv_post);
    ZVAL_UNDEF(&intern->zv_cookie);
    ZVAL_UNDEF(&intern->zv_files);
    
    /* Initialize owned HashTables to NULL */
    intern->ht_headers = NULL;
    intern->ht_input = NULL;
    intern->ht_attributes = NULL;

    /* Initialize zvals to UNDEF */
    ZVAL_UNDEF(&intern->zv_body);
    ZVAL_UNDEF(&intern->zv_json);
    ZVAL_UNDEF(&intern->zv_path);
    ZVAL_UNDEF(&intern->zv_method);
    ZVAL_UNDEF(&intern->zv_content_type);
    ZVAL_UNDEF(&intern->zv_request_method);
    ZVAL_UNDEF(&intern->zv_method_override_header);
    ZVAL_UNDEF(&intern->zv_method_override_post);
    ZVAL_UNDEF(&intern->zv_uri);
    ZVAL_UNDEF(&intern->zv_query_string);

    /* Initialize SAPI strings */
    intern->request_uri = NULL;
    intern->request_uri_len = 0;
    intern->query_string = NULL;
    intern->query_string_len = 0;
    intern->request_method = NULL;
    intern->request_method_len = 0;

    /* Initialize protocol version */
    intern->protocol_version = NULL;

    /* Initialize flags */
    intern->flags = 0;

    /* Initialize standard object */
    zend_object_std_init(&intern->std, ce);
    object_properties_init(&intern->std, ce);
    intern->std.handlers = &signalforge_request_object_handlers;

    return &intern->std;
}

static void signalforge_request_free_object(zend_object *object)
{
    signalforge_request_object *intern = signalforge_request_from_obj(object);

    /* Release superglobal zval references */
    if (!Z_ISUNDEF(intern->zv_server)) zval_ptr_dtor(&intern->zv_server);
    if (!Z_ISUNDEF(intern->zv_get)) zval_ptr_dtor(&intern->zv_get);
    if (!Z_ISUNDEF(intern->zv_post)) zval_ptr_dtor(&intern->zv_post);
    if (!Z_ISUNDEF(intern->zv_cookie)) zval_ptr_dtor(&intern->zv_cookie);
    if (!Z_ISUNDEF(intern->zv_files)) zval_ptr_dtor(&intern->zv_files);

    /* Release refcounted zvals */
    if (!Z_ISUNDEF(intern->zv_body)) zval_ptr_dtor(&intern->zv_body);
    if (!Z_ISUNDEF(intern->zv_json)) zval_ptr_dtor(&intern->zv_json);
    if (!Z_ISUNDEF(intern->zv_path)) zval_ptr_dtor(&intern->zv_path);
    if (!Z_ISUNDEF(intern->zv_method)) zval_ptr_dtor(&intern->zv_method);
    if (!Z_ISUNDEF(intern->zv_content_type)) zval_ptr_dtor(&intern->zv_content_type);
    if (!Z_ISUNDEF(intern->zv_request_method)) zval_ptr_dtor(&intern->zv_request_method);
    if (!Z_ISUNDEF(intern->zv_method_override_header)) zval_ptr_dtor(&intern->zv_method_override_header);
    if (!Z_ISUNDEF(intern->zv_method_override_post)) zval_ptr_dtor(&intern->zv_method_override_post);
    if (!Z_ISUNDEF(intern->zv_uri)) zval_ptr_dtor(&intern->zv_uri);
    if (!Z_ISUNDEF(intern->zv_query_string)) zval_ptr_dtor(&intern->zv_query_string);
    
    /* Free owned HashTables */
    if (intern->ht_headers) {
        zend_hash_destroy(intern->ht_headers);
        FREE_HASHTABLE(intern->ht_headers);
    }
    if (intern->ht_input) {
        zend_hash_destroy(intern->ht_input);
        FREE_HASHTABLE(intern->ht_input);
    }
    if (intern->ht_attributes) {
        zend_hash_destroy(intern->ht_attributes);
        FREE_HASHTABLE(intern->ht_attributes);
    }

    /* Release protocol_version string */
    if (intern->protocol_version) {
        zend_string_release(intern->protocol_version);
    }

    /* Destroy standard object properties */
    zend_object_std_dtor(&intern->std);
}

/* Forward declaration for clone */
signalforge_request_object *signalforge_request_clone(signalforge_request_object *src, zval *return_value);

static zend_object *signalforge_request_clone_obj(zend_object *object)
{
    signalforge_request_object *old_intern = signalforge_request_from_obj(object);
    zval new_zv;

    /* Use existing zero-copy clone helper */
    signalforge_request_clone(old_intern, &new_zv);
    return Z_OBJ(new_zv);
}

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

/**
 * Extract and normalize headers from $_SERVER HashTable into a zval array.
 *
 * WHY: HTTP headers are case-insensitive per RFC 7230, but $_SERVER stores them
 * as HTTP_* uppercase with underscores. We normalize to lowercase with dashes
 * for PSR-7 compliance and efficient case-insensitive lookups. This single-pass
 * normalization avoids repeated case conversions during header access.
 *
 * @param server HashTable containing $_SERVER data
 * @return HashTable with normalized lowercase header names
 */
HashTable *signalforge_extract_headers(HashTable *server)
{
    HashTable *ht;
    
    /* Allocate and initialize HashTable */
    ALLOC_HASHTABLE(ht);
    zend_hash_init(ht, 16, NULL, ZVAL_PTR_DTOR, 0);

    ZEND_HASH_FOREACH_STR_KEY_VAL(server, zend_string *key, zval *value) {
        if (key && Z_TYPE_P(value) == IS_STRING) {
            zend_string *normalized = NULL;
            
            /* Check if this is an HTTP_* header */
            if (ZSTR_LEN(key) > 5 && memcmp(ZSTR_VAL(key), "HTTP_", 5) == 0) {
                /* Extract header name (skip "HTTP_") */
                const char *header_name = ZSTR_VAL(key) + 5;
                size_t header_name_len = ZSTR_LEN(key) - 5;
                normalized = signalforge_normalize_header_name(header_name, header_name_len);
            }
            /* Handle special headers without HTTP_ prefix */
            else if (zend_string_equals_literal(key, "CONTENT_TYPE")) {
                normalized = zend_string_init_interned("content-type", sizeof("content-type") - 1, 1);
            }
            else if (zend_string_equals_literal(key, "CONTENT_LENGTH")) {
                normalized = zend_string_init_interned("content-length", sizeof("content-length") - 1, 1);
            }
            
            if (normalized) {
                zval header_val;
                zval *existing_val = zend_hash_find(ht, normalized);
                
                if (existing_val && Z_TYPE_P(existing_val) == IS_ARRAY) {
                    /* Add to existing array - addref BEFORE add_next_index_zval takes ownership */
                    Z_TRY_ADDREF_P(value);
                    add_next_index_zval(existing_val, value);
                } else {
                    /* Create new entry */
                    if (existing_val) {
                        /* Convert existing single value to array */
                        zval header_array;
                        array_init(&header_array);
                        Z_TRY_ADDREF_P(existing_val);
                        add_next_index_zval(&header_array, existing_val);
                        Z_TRY_ADDREF_P(value);
                        add_next_index_zval(&header_array, value);
                        zend_hash_update(ht, normalized, &header_array);
                    } else {
                        ZVAL_COPY(&header_val, value);
                        zend_hash_update(ht, normalized, &header_val);
                    }
                }
                zend_string_release(normalized);
            }
        }
    } ZEND_HASH_FOREACH_END();
    
    return ht;
}

/**
 * Normalize HTTP header name to lowercase with dashes.
 *
 * WHY: HTTP header names are case-insensitive (RFC 7230), but we need consistent
 * storage for efficient HashTable lookups. Converting underscores to dashes
 * ensures compliance with HTTP/1.1 header format. Single-pass O(n) algorithm
 * avoids repeated normalization on each header access.
 *
 * @param src Original header name
 * @param src_len Length of header name
 * @return Normalized header name as interned string for memory efficiency
 */
zend_string *signalforge_normalize_header_name(const char *src, size_t src_len)
{
    zend_string *result;
    char *dst;
    size_t i;

    if (src_len == 0) {
        return ZSTR_EMPTY_ALLOC();
    }

    /* Allocate string */
    result = zend_string_alloc(src_len, 0);
    dst = ZSTR_VAL(result);

    /* Normalize: lowercase and replace _ with - (HTTP/1.1 compliant) */
    for (i = 0; i < src_len; i++) {
        dst[i] = (src[i] == '_') ? '-' : tolower((unsigned char)src[i]);
    }
    dst[src_len] = '\0';
    ZSTR_LEN(result) = src_len;

    return result;
}

/**
 * Read request body using direct SAPI access for maximum performance.
 * Falls back to php://input stream if SAPI data not available.
 * Caches result in intern->zv_body.
 */
zend_string *signalforge_read_body(signalforge_request_object *intern)
{
    zend_string *body;

    if (intern->flags & SF_REQ_FLAG_BODY_READ) {
        if (Z_TYPE(intern->zv_body) == IS_STRING) {
            return Z_STR(intern->zv_body);
        }
        return ZSTR_EMPTY_ALLOC();
    }

    intern->flags |= SF_REQ_FLAG_BODY_READ;

    /* Read body from php://input stream */
    php_stream *stream = php_stream_open_wrapper("php://input", "rb", 0, NULL);
    if (!stream) {
        ZVAL_EMPTY_STRING(&intern->zv_body);
        return Z_STR(intern->zv_body);
    }

    body = php_stream_copy_to_mem(stream, PHP_STREAM_COPY_ALL, 0);
    php_stream_close(stream);

    if (body) {
        ZVAL_STR(&intern->zv_body, body);
        return body;
    }

    ZVAL_EMPTY_STRING(&intern->zv_body);
    return Z_STR(intern->zv_body);
}

/**
 * Parse path component from REQUEST_URI.
 * Strips query string and fragment.
 */
void signalforge_parse_path(signalforge_request_object *intern)
{
    const char *uri;
    size_t uri_len;
    const char *end;

    if (intern->flags & SF_REQ_FLAG_PATH_PARSED) {
        return;
    }

    intern->flags |= SF_REQ_FLAG_PATH_PARSED;

    if (Z_TYPE(intern->zv_uri) != IS_STRING || Z_STRLEN(intern->zv_uri) == 0) {
        ZVAL_EMPTY_STRING(&intern->zv_path);
        return;
    }

    uri = Z_STRVAL(intern->zv_uri);
    uri_len = Z_STRLEN(intern->zv_uri);

    /* Find query string or fragment */
    end = strpbrk(uri, "?#");
    if (end) {
        size_t path_len = end - uri;
        ZVAL_STRINGL(&intern->zv_path, uri, path_len);
    } else {
        ZVAL_STRINGL(&intern->zv_path, uri, uri_len);
    }
}

/**
 * Resolve HTTP method considering overrides.
 * Priority: X-HTTP-Method-Override header > _method POST field > REQUEST_METHOD
 */
void signalforge_resolve_method(signalforge_request_object *intern)
{
    if (intern->flags & SF_REQ_FLAG_METHOD_RESOLVED) {
        return;
    }

    intern->flags |= SF_REQ_FLAG_METHOD_RESOLVED;

    /* Check X-HTTP-Method-Override header (stored as refcounted zval) */
    if (Z_TYPE(intern->zv_method_override_header) == IS_STRING) {
        /* Convert to uppercase for consistency */
        zend_string *method = zend_string_dup(Z_STR(intern->zv_method_override_header), 0);
        signalforge_strtoupper(ZSTR_VAL(method), ZSTR_LEN(method));
        ZVAL_STR(&intern->zv_method, method);
        return;
    }

    /* Check _method POST field (stored as refcounted zval) */
    if (Z_TYPE(intern->zv_method_override_post) == IS_STRING) {
        zend_string *method = zend_string_dup(Z_STR(intern->zv_method_override_post), 0);
        signalforge_strtoupper(ZSTR_VAL(method), ZSTR_LEN(method));
        ZVAL_STR(&intern->zv_method, method);
        return;
    }

    /* Fall back to REQUEST_METHOD from stored zval */
    if (Z_TYPE(intern->zv_request_method) == IS_STRING) {
        ZVAL_COPY(&intern->zv_method, &intern->zv_request_method);
        /* Convert to uppercase for consistency */
        if (Z_STRLEN(intern->zv_method) > 0) {
            signalforge_strtoupper(Z_STRVAL(intern->zv_method), Z_STRLEN(intern->zv_method));
        }
    } else {
        ZVAL_STRING(&intern->zv_method, "GET");
    }
}

/**
 * Parse Content-Type header to extract just the MIME type.
 * Strips charset, boundary, and other parameters.
 */
void signalforge_parse_content_type(signalforge_request_object *intern)
{
    zval *ct_val;
    const char *ct_str;
    size_t ct_len;
    const char *semicolon;

    if (intern->flags & SF_REQ_FLAG_CTYPE_PARSED) {
        return;
    }

    intern->flags |= SF_REQ_FLAG_CTYPE_PARSED;

    if (!intern->ht_headers) {
        ZVAL_NULL(&intern->zv_content_type);
        return;
    }

    ct_val = zend_hash_str_find(intern->ht_headers, "content-type", 12);
    if (!ct_val || Z_TYPE_P(ct_val) != IS_STRING) {
        ZVAL_NULL(&intern->zv_content_type);
        return;
    }

    ct_str = Z_STRVAL_P(ct_val);
    ct_len = Z_STRLEN_P(ct_val);

    /* Find semicolon that starts parameters */
    semicolon = memchr(ct_str, ';', ct_len);
    if (semicolon) {
        /* Trim trailing whitespace before semicolon */
        size_t mime_len = semicolon - ct_str;
        while (mime_len > 0 && ct_str[mime_len - 1] == ' ') {
            mime_len--;
        }
        ZVAL_STRINGL(&intern->zv_content_type, ct_str, mime_len);
    } else {
        ZVAL_STRINGL(&intern->zv_content_type, ct_str, ct_len);
    }
}

/**
 * Merge input from multiple sources (lazy evaluation)
 */
/**
 * Merge input data from multiple sources with priority ordering.
 *
 * WHY: PSR-7 doesn't specify input precedence, but common practice prioritizes:
 * 1. JSON body (structured data from APIs)
 * 2. POST data (form submissions)
 * 3. GET data (query parameters)
 * This allows JSON APIs to override query params while maintaining form compatibility.
 * Lazy evaluation ensures JSON parsing only occurs for JSON content types.
 *
 * @param intern Request object instance
 * @return HashTable containing merged input data
 */
HashTable *signalforge_merge_input(signalforge_request_object *intern)
{
    if (intern->flags & SF_REQ_FLAG_INPUT_MERGED) {
        return intern->ht_input;
    }

    /* Allocate input HashTable */
    if (!intern->ht_input) {
        ALLOC_HASHTABLE(intern->ht_input);
        zend_hash_init(intern->ht_input, 32, NULL, ZVAL_PTR_DTOR, 0);
    }

    /* Priority 1: JSON body (highest priority for API requests) */
    signalforge_parse_content_type(intern);
    if (!Z_ISNULL(intern->zv_content_type) &&
        strcasecmp(Z_STRVAL(intern->zv_content_type), "application/json") == 0) {

        /* Parse JSON if not already done - reuse cached result from getParsedBody() */
        if (!(intern->flags & SF_REQ_FLAG_JSON_PARSED)) {
            zend_string *body = signalforge_read_body(intern);
            if (body && ZSTR_LEN(body) > 0) {
                /* Parse JSON and cache in zv_json */
                php_json_decode(&intern->zv_json, ZSTR_VAL(body), ZSTR_LEN(body), 1, PHP_JSON_PARSER_DEFAULT_DEPTH);
                intern->flags |= SF_REQ_FLAG_JSON_PARSED;
                
                /* Merge JSON data into input HashTable if it's an array */
                if (Z_TYPE(intern->zv_json) == IS_ARRAY) {
                    ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL(intern->zv_json), zend_ulong idx, zend_string *key, zval *value) {
                        zval val_copy;
                        ZVAL_COPY(&val_copy, value);
                        if (key) {
                            zend_hash_add(intern->ht_input, key, &val_copy);
                        } else {
                            zend_hash_index_add(intern->ht_input, idx, &val_copy);
                        }
                    } ZEND_HASH_FOREACH_END();
                }
            } else {
                /* Empty body - set to null */
                ZVAL_NULL(&intern->zv_json);
                intern->flags |= SF_REQ_FLAG_JSON_PARSED;
            }
        } else if (Z_TYPE(intern->zv_json) == IS_ARRAY) {
            /* JSON already parsed - merge cached result into input HashTable */
            ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL(intern->zv_json), zend_ulong idx, zend_string *key, zval *value) {
                zval val_copy;
                ZVAL_COPY(&val_copy, value);
                if (key) {
                    zend_hash_add(intern->ht_input, key, &val_copy);
                } else {
                    zend_hash_index_add(intern->ht_input, idx, &val_copy);
                }
            } ZEND_HASH_FOREACH_END();
        }
    } else {
        /* Priority 2: POST parameters */
        if (Z_TYPE(intern->zv_post) == IS_ARRAY) {
            ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL(intern->zv_post), zend_string *key, zval *value) {
                if (key && !zend_string_equals_literal(key, "_method")) {
                    zval val_copy;
                    ZVAL_COPY(&val_copy, value);
                    zend_hash_add(intern->ht_input, key, &val_copy);
                }
            } ZEND_HASH_FOREACH_END();
        }
    }

    /* Priority 3: GET parameters (only if not JSON/POST) */
    if (Z_TYPE(intern->zv_get) == IS_ARRAY && !(intern->flags & SF_REQ_FLAG_JSON_PARSED) && 
        (Z_TYPE(intern->zv_post) != IS_ARRAY || zend_hash_num_elements(Z_ARRVAL(intern->zv_post)) == 0)) {
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL(intern->zv_get), zend_string *key, zval *value) {
            zval val_copy;
            ZVAL_COPY(&val_copy, value);
            zend_hash_add(intern->ht_input, key, &val_copy);
        } ZEND_HASH_FOREACH_END();
    }

    intern->flags |= SF_REQ_FLAG_INPUT_MERGED;
    return intern->ht_input;
}

/**
 * Get value from HashTable with string key
 */
zval *signalforge_hash_get(HashTable *ht, const char *key, size_t key_len)
{
    if (!ht) return NULL;
    return zend_hash_str_find(ht, key, key_len);
}

/**
 * Clone headers HashTable for immutability
 */
static HashTable *signalforge_clone_headers(HashTable *src)
{
    HashTable *dst;
    zend_string *key;
    zval *val;

    if (!src) {
        ALLOC_HASHTABLE(dst);
        zend_hash_init(dst, 16, NULL, ZVAL_PTR_DTOR, 0);
        return dst;
    }

    ALLOC_HASHTABLE(dst);
    zend_hash_init(dst, zend_hash_num_elements(src), NULL, ZVAL_PTR_DTOR, 0);

    ZEND_HASH_FOREACH_STR_KEY_VAL(src, key, val) {
        Z_TRY_ADDREF_P(val);
        if (key) {
            zend_hash_add(dst, key, val);
        }
    } ZEND_HASH_FOREACH_END();

    return dst;
}

/**
 * Clone Request instance for immutability
 */
signalforge_request_object *signalforge_request_clone(signalforge_request_object *src, zval *return_value)
{
    signalforge_request_object *dst;

    /* Create new instance */
    object_init_ex(return_value, signalforge_request_ce);
    dst = Z_SIGNALFORGE_REQUEST_P(return_value);

    /* Copy superglobal zval references with proper refcounting */
    ZVAL_COPY(&dst->zv_server, &src->zv_server);
    ZVAL_COPY(&dst->zv_get, &src->zv_get);
    ZVAL_COPY(&dst->zv_post, &src->zv_post);
    ZVAL_COPY(&dst->zv_cookie, &src->zv_cookie);
    ZVAL_COPY(&dst->zv_files, &src->zv_files);

    /* Clone owned HashTables */
    if (src->ht_headers) {
        dst->ht_headers = signalforge_clone_headers(src->ht_headers);
    }
    if (src->ht_input) {
        dst->ht_input = zend_array_dup(src->ht_input);
    }
    if (src->ht_attributes) {
        dst->ht_attributes = zend_array_dup(src->ht_attributes);
    }

    /* Copy zvals */
    ZVAL_COPY(&dst->zv_body, &src->zv_body);
    ZVAL_COPY(&dst->zv_json, &src->zv_json);
    ZVAL_COPY(&dst->zv_path, &src->zv_path);
    ZVAL_COPY(&dst->zv_method, &src->zv_method);
    ZVAL_COPY(&dst->zv_content_type, &src->zv_content_type);
    ZVAL_COPY(&dst->zv_request_method, &src->zv_request_method);
    ZVAL_COPY(&dst->zv_method_override_header, &src->zv_method_override_header);
    ZVAL_COPY(&dst->zv_method_override_post, &src->zv_method_override_post);
    ZVAL_COPY(&dst->zv_uri, &src->zv_uri);
    ZVAL_COPY(&dst->zv_query_string, &src->zv_query_string);

    /* Set pointers from zvals (pointers into owned zend_strings) */
    if (Z_TYPE(dst->zv_uri) == IS_STRING) {
        dst->request_uri = Z_STRVAL(dst->zv_uri);
        dst->request_uri_len = Z_STRLEN(dst->zv_uri);
    } else {
        dst->request_uri = NULL;
        dst->request_uri_len = 0;
    }
    if (Z_TYPE(dst->zv_query_string) == IS_STRING) {
        dst->query_string = Z_STRVAL(dst->zv_query_string);
        dst->query_string_len = Z_STRLEN(dst->zv_query_string);
    } else {
        dst->query_string = NULL;
        dst->query_string_len = 0;
    }
    if (Z_TYPE(dst->zv_request_method) == IS_STRING) {
        dst->request_method = Z_STRVAL(dst->zv_request_method);
        dst->request_method_len = Z_STRLEN(dst->zv_request_method);
    } else {
        dst->request_method = NULL;
        dst->request_method_len = 0;
    }

    /* Copy protocol version */
    if (src->protocol_version) {
        dst->protocol_version = zend_string_copy(src->protocol_version);
    }

    /* Copy flags */
    dst->flags = src->flags;

    return dst;
}

/**
 * Direct superglobal access macro (inlined for maximum performance).
 *
 * WHY: PHP's superglobals are stored in EG(symbol_table) as regular variables.
 * Direct HashTable access bypasses PHP's array layer, avoiding function call
 * overhead and array conversion. zend_is_auto_global_str() ensures the superglobal
 * is initialized if not already present. This provides ~10x faster access than
 * using PHP's $GLOBALS or direct variable access.
 */
#define signalforge_get_superglobal(name, name_len) \
    ({ zend_is_auto_global_str((name), (name_len)); \
       zend_hash_str_find(&EG(symbol_table), (name), (name_len)); })

/* ============================================================================
 * PSR-7 METHOD IMPLEMENTATIONS
 * ============================================================================ */

/**
 * Capture current HTTP request from superglobals.
 *
 * Creates a new Request instance populated with data from PHP's superglobals
 * ($_SERVER, $_GET, $_POST, $_COOKIE, $_FILES). Performs lazy parsing of
 * headers, URI, and other request components. This is the primary factory
 * method for creating Request instances from the current HTTP request.
 *
 * @return RequestInterface New request instance
 * @throws RuntimeException When $_SERVER is not available
 */
PHP_METHOD(Signalforge_Http_Request, capture)
{
    signalforge_request_object *intern;
    zval *server_zv, *get_zv, *post_zv, *cookie_zv, *files_zv;
    zval *uri_zv, *qs_zv, *method_zv;

    ZEND_PARSE_PARAMETERS_NONE();

    /* Create new instance */
    object_init_ex(return_value, signalforge_request_ce);
    intern = Z_SIGNALFORGE_REQUEST_P(return_value);

    /* Get references to PHP's superglobals from symbol table */
    server_zv = signalforge_get_superglobal("_SERVER", sizeof("_SERVER") - 1);
    get_zv = signalforge_get_superglobal("_GET", sizeof("_GET") - 1);
    post_zv = signalforge_get_superglobal("_POST", sizeof("_POST") - 1);
    cookie_zv = signalforge_get_superglobal("_COOKIE", sizeof("_COOKIE") - 1);
    files_zv = signalforge_get_superglobal("_FILES", sizeof("_FILES") - 1);

    /* Ensure $_SERVER is available */
    if (!server_zv || Z_TYPE_P(server_zv) != IS_ARRAY) {
        zend_throw_exception(zend_ce_exception,
            "Request::capture() requires $_SERVER to be available.", 0);
        RETURN_THROWS();
    }

    /* Store references to superglobals with proper refcounting */
    ZVAL_COPY(&intern->zv_server, server_zv);
    if (get_zv && Z_TYPE_P(get_zv) == IS_ARRAY) {
        ZVAL_COPY(&intern->zv_get, get_zv);
    } else {
        ZVAL_UNDEF(&intern->zv_get);
    }
    if (post_zv && Z_TYPE_P(post_zv) == IS_ARRAY) {
        ZVAL_COPY(&intern->zv_post, post_zv);
    } else {
        ZVAL_UNDEF(&intern->zv_post);
    }
    if (cookie_zv && Z_TYPE_P(cookie_zv) == IS_ARRAY) {
        ZVAL_COPY(&intern->zv_cookie, cookie_zv);
    } else {
        ZVAL_UNDEF(&intern->zv_cookie);
    }
    if (files_zv && Z_TYPE_P(files_zv) == IS_ARRAY) {
        ZVAL_COPY(&intern->zv_files, files_zv);
    } else {
        ZVAL_UNDEF(&intern->zv_files);
    }

    /* Initialize attributes HashTable */
    ALLOC_HASHTABLE(intern->ht_attributes);
    zend_hash_init(intern->ht_attributes, 8, NULL, ZVAL_PTR_DTOR, 0);

    /* Get request info from $_SERVER */
    uri_zv = zend_hash_str_find(Z_ARRVAL(intern->zv_server), "REQUEST_URI", sizeof("REQUEST_URI") - 1);
    if (uri_zv && Z_TYPE_P(uri_zv) == IS_STRING) {
        ZVAL_COPY(&intern->zv_uri, uri_zv);
        intern->request_uri = Z_STRVAL(intern->zv_uri);
        intern->request_uri_len = Z_STRLEN(intern->zv_uri);
    } else {
        ZVAL_EMPTY_STRING(&intern->zv_uri);
        intern->request_uri = Z_STRVAL(intern->zv_uri);
        intern->request_uri_len = Z_STRLEN(intern->zv_uri);
    }

    qs_zv = zend_hash_str_find(Z_ARRVAL(intern->zv_server), "QUERY_STRING", sizeof("QUERY_STRING") - 1);
    if (qs_zv && Z_TYPE_P(qs_zv) == IS_STRING) {
        ZVAL_COPY(&intern->zv_query_string, qs_zv);
        intern->query_string = Z_STRVAL(intern->zv_query_string);
        intern->query_string_len = Z_STRLEN(intern->zv_query_string);
    } else {
        ZVAL_NULL(&intern->zv_query_string);
        intern->query_string = NULL;
        intern->query_string_len = 0;
    }

    /* Extract REQUEST_METHOD */
    method_zv = zend_hash_str_find(Z_ARRVAL(intern->zv_server), "REQUEST_METHOD", sizeof("REQUEST_METHOD") - 1);
    if (method_zv && Z_TYPE_P(method_zv) == IS_STRING) {
        ZVAL_COPY(&intern->zv_request_method, method_zv);
        intern->request_method = Z_STRVAL(intern->zv_request_method);
        intern->request_method_len = Z_STRLEN(intern->zv_request_method);
    } else {
        ZVAL_STRING(&intern->zv_request_method, "GET");
        intern->request_method = Z_STRVAL(intern->zv_request_method);
        intern->request_method_len = Z_STRLEN(intern->zv_request_method);
    }

    /* Extract method override values */
    ZVAL_UNDEF(&intern->zv_method_override_header);
    ZVAL_UNDEF(&intern->zv_method_override_post);
    
    zval *override_header_zv = zend_hash_str_find(Z_ARRVAL(intern->zv_server), "HTTP_X_HTTP_METHOD_OVERRIDE", sizeof("HTTP_X_HTTP_METHOD_OVERRIDE") - 1);
    if (override_header_zv && Z_TYPE_P(override_header_zv) == IS_STRING) {
        ZVAL_COPY(&intern->zv_method_override_header, override_header_zv);
    }

    if (Z_TYPE(intern->zv_post) == IS_ARRAY) {
        zval *method_post_zv = zend_hash_str_find(Z_ARRVAL(intern->zv_post), "_method", sizeof("_method") - 1);
        if (method_post_zv && Z_TYPE_P(method_post_zv) == IS_STRING) {
            ZVAL_COPY(&intern->zv_method_override_post, method_post_zv);
        }
    }

    /* Build normalized headers HashTable */
    intern->ht_headers = signalforge_extract_headers(Z_ARRVAL(intern->zv_server));

    /* Set default protocol version - non-persistent (per-request allocation) */
    intern->protocol_version = zend_string_init("1.1", sizeof("1.1") - 1, 0);
}
/* }}} */

/* {{{ proto Request Request::create(string $method, mixed $uri, array $serverParams = [])
 * Creates a new Request instance with the specified method and URI.
 * This is the PSR-17 factory method for creating Request instances programmatically.
 *
 * @param string $method HTTP method (GET, POST, PUT, DELETE, etc.)
 * @param mixed $uri URI string or Uri object
 * @param array $serverParams Optional server parameters
 * @return Request New request instance
 * @throws InvalidArgumentException When method is invalid
 */
PHP_METHOD(Signalforge_Http_Request, create)
{
    signalforge_request_object *intern;
    zend_string *method;
    zval *uri_param = NULL;
    zval *server_params = NULL;
    zend_string *uri_str = NULL;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STR(method)
        Z_PARAM_ZVAL(uri_param)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_OR_NULL(server_params)
    ZEND_PARSE_PARAMETERS_END();

    /* Validate HTTP method */
    const char *method_str = ZSTR_VAL(method);
    size_t method_len = ZSTR_LEN(method);
    zend_bool valid = 0;

    if (method_len == 3) {
        if (strncasecmp(method_str, "GET", 3) == 0) valid = 1;
        else if (strncasecmp(method_str, "PUT", 3) == 0) valid = 1;
    } else if (method_len == 4) {
        if (strncasecmp(method_str, "POST", 4) == 0) valid = 1;
        else if (strncasecmp(method_str, "HEAD", 4) == 0) valid = 1;
    } else if (method_len == 5) {
        if (strncasecmp(method_str, "PATCH", 5) == 0) valid = 1;
        else if (strncasecmp(method_str, "TRACE", 5) == 0) valid = 1;
    } else if (method_len == 6) {
        if (strncasecmp(method_str, "DELETE", 6) == 0) valid = 1;
    } else if (method_len == 7) {
        if (strncasecmp(method_str, "OPTIONS", 7) == 0) valid = 1;
        else if (strncasecmp(method_str, "CONNECT", 7) == 0) valid = 1;
    }

    if (!valid) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
            "Invalid HTTP method '%s'. Must be one of: GET, HEAD, POST, PUT, PATCH, DELETE, OPTIONS, TRACE, CONNECT",
            method_str);
        RETURN_THROWS();
    }

    /* Extract URI string from parameter */
    if (Z_TYPE_P(uri_param) == IS_STRING) {
        uri_str = Z_STR_P(uri_param);
    } else if (Z_TYPE_P(uri_param) == IS_OBJECT && instanceof_function(Z_OBJCE_P(uri_param), signalforge_uri_ce)) {
        /* Uri object - convert to string */
        signalforge_uri_object *uri_obj = Z_SIGNALFORGE_URI_P(uri_param);
        smart_str buf = {0};

        if (uri_obj->scheme && ZSTR_LEN(uri_obj->scheme) > 0) {
            smart_str_append(&buf, uri_obj->scheme);
            smart_str_appendl(&buf, "://", 3);
        }
        if (uri_obj->host && ZSTR_LEN(uri_obj->host) > 0) {
            if (uri_obj->user && ZSTR_LEN(uri_obj->user) > 0) {
                smart_str_append(&buf, uri_obj->user);
                if (uri_obj->pass && ZSTR_LEN(uri_obj->pass) > 0) {
                    smart_str_appendc(&buf, ':');
                    smart_str_append(&buf, uri_obj->pass);
                }
                smart_str_appendc(&buf, '@');
            }
            smart_str_append(&buf, uri_obj->host);
            if (uri_obj->port != SIGNALFORGE_PORT_UNSET &&
                !signalforge_is_standard_port(uri_obj->scheme, uri_obj->port)) {
                smart_str_appendc(&buf, ':');
                smart_str_append_long(&buf, uri_obj->port);
            }
        }
        if (uri_obj->path && ZSTR_LEN(uri_obj->path) > 0) {
            smart_str_append(&buf, uri_obj->path);
        } else if (!uri_obj->host || ZSTR_LEN(uri_obj->host) == 0) {
            smart_str_appendc(&buf, '/');
        }
        if (uri_obj->query && ZSTR_LEN(uri_obj->query) > 0) {
            smart_str_appendc(&buf, '?');
            smart_str_append(&buf, uri_obj->query);
        }
        if (uri_obj->fragment && ZSTR_LEN(uri_obj->fragment) > 0) {
            smart_str_appendc(&buf, '#');
            smart_str_append(&buf, uri_obj->fragment);
        }

        smart_str_0(&buf);
        uri_str = buf.s ? buf.s : zend_empty_string;
    } else {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "URI must be a string or Uri object", 0);
        RETURN_THROWS();
    }

    /* Create new instance */
    object_init_ex(return_value, signalforge_request_ce);
    intern = Z_SIGNALFORGE_REQUEST_P(return_value);

    /* Initialize with serverParams or empty arrays */
    if (server_params && Z_TYPE_P(server_params) == IS_ARRAY) {
        ZVAL_COPY(&intern->zv_server, server_params);
    } else {
        array_init(&intern->zv_server);
    }

    /* Initialize empty superglobal arrays */
    array_init(&intern->zv_get);
    array_init(&intern->zv_post);
    array_init(&intern->zv_cookie);
    array_init(&intern->zv_files);

    /* Initialize attributes HashTable */
    ALLOC_HASHTABLE(intern->ht_attributes);
    zend_hash_init(intern->ht_attributes, 8, NULL, ZVAL_PTR_DTOR, 0);

    /* Set the URI */
    ZVAL_STR_COPY(&intern->zv_uri, uri_str);
    intern->request_uri = Z_STRVAL(intern->zv_uri);
    intern->request_uri_len = Z_STRLEN(intern->zv_uri);

    /* Set query string if present in URI */
    const char *query_pos = strchr(Z_STRVAL(intern->zv_uri), '?');
    if (query_pos) {
        size_t query_len = Z_STRLEN(intern->zv_uri) - (query_pos - Z_STRVAL(intern->zv_uri)) - 1;
        ZVAL_STRINGL(&intern->zv_query_string, query_pos + 1, query_len);
        intern->query_string = Z_STRVAL(intern->zv_query_string);
        intern->query_string_len = query_len;
    } else {
        ZVAL_EMPTY_STRING(&intern->zv_query_string);
        intern->query_string = Z_STRVAL(intern->zv_query_string);
        intern->query_string_len = 0;
    }

    /* Set the method (uppercase) */
    zend_string *upper_method = zend_string_init(method_str, method_len, 0);
    signalforge_strtoupper(ZSTR_VAL(upper_method), ZSTR_LEN(upper_method));
    ZVAL_STR(&intern->zv_method, upper_method);
    intern->flags |= SF_REQ_FLAG_METHOD_RESOLVED;

    /* Also set request_method for consistency */
    ZVAL_STR_COPY(&intern->zv_request_method, upper_method);
    intern->request_method = Z_STRVAL(intern->zv_request_method);
    intern->request_method_len = Z_STRLEN(intern->zv_request_method);

    /* Initialize undefined zvals */
    ZVAL_UNDEF(&intern->zv_method_override_header);
    ZVAL_UNDEF(&intern->zv_method_override_post);
    ZVAL_UNDEF(&intern->zv_body);
    ZVAL_UNDEF(&intern->zv_json);
    ZVAL_UNDEF(&intern->zv_path);
    ZVAL_UNDEF(&intern->zv_content_type);

    /* Initialize empty headers (will be built lazily from server params if needed) */
    ALLOC_HASHTABLE(intern->ht_headers);
    zend_hash_init(intern->ht_headers, 8, NULL, ZVAL_PTR_DTOR, 0);
    intern->flags |= SF_REQ_FLAG_HEADERS_EXTRACTED;

    /* Set default protocol version */
    intern->protocol_version = zend_string_init("1.1", sizeof("1.1") - 1, 0);
}
/* }}} */

/* ============================================================================
 * PSR-7 MessageInterface Methods
 * ============================================================================ */

/* {{{ getProtocolVersion() */
PHP_METHOD(Signalforge_Http_Request, getProtocolVersion)
{
    signalforge_request_object *intern = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();
    
    if (intern->protocol_version) {
        RETURN_STR(zend_string_copy(intern->protocol_version));
    }
    RETURN_STRING("1.1");
}
/* }}} */

/* {{{ withProtocolVersion($version) */
PHP_METHOD(Signalforge_Http_Request, withProtocolVersion)
{
    zend_string *version;
    signalforge_request_object *src, *dst;
    
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(version)
    ZEND_PARSE_PARAMETERS_END();
    
    /* Validate protocol version */
    if (!zend_string_equals_literal(version, "1.0") && 
        !zend_string_equals_literal(version, "1.1")) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Protocol version must be '1.0' or '1.1'", 0);
        RETURN_THROWS();
    }
    
    src = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    dst = signalforge_request_clone(src, return_value);
    
    /* Update protocol version */
    if (dst->protocol_version) {
        zend_string_release(dst->protocol_version);
    }
    dst->protocol_version = zend_string_copy(version);
}
/* }}} */

/* {{{ getHeaders() */
PHP_METHOD(Signalforge_Http_Request, getHeaders)
{
    signalforge_request_object *intern = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();
    
    if (intern->ht_headers) {
        /* Convert HashTable to array, ensuring values are arrays */
        array_init(return_value);
        ZEND_HASH_FOREACH_STR_KEY_VAL(intern->ht_headers, zend_string *key, zval *val) {
            if (key) {
                zval header_array;
                if (Z_TYPE_P(val) == IS_ARRAY) {
                    /* Value is already an array, copy it */
                    ZVAL_COPY(&header_array, val);
                } else {
                    /* Wrap single value in array */
                    array_init(&header_array);
                    Z_TRY_ADDREF_P(val);
                    add_next_index_zval(&header_array, val);
                }
                zend_hash_add(Z_ARRVAL_P(return_value), key, &header_array);
            }
        } ZEND_HASH_FOREACH_END();
    } else {
        array_init(return_value);
    }
}
/* }}} */

/* {{{ hasHeader($name) */
PHP_METHOD(Signalforge_Http_Request, hasHeader)
{
    zend_string *name;
    signalforge_request_object *intern = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    char normalized_name[256];
    size_t normalized_len;
    
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(name)
    ZEND_PARSE_PARAMETERS_END();
    
    if (!intern->ht_headers) {
        RETURN_FALSE;
    }
    
    zend_string *normalized = signalforge_normalize_header_name(ZSTR_VAL(name), ZSTR_LEN(name));
    zend_bool exists = zend_hash_exists(intern->ht_headers, normalized);
    zend_string_release(normalized);
    RETURN_BOOL(exists);
}
/* }}} */

/* {{{ getHeader($name) */
PHP_METHOD(Signalforge_Http_Request, getHeader)
{
    zend_string *name;
    signalforge_request_object *intern = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    zval *val;
    
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(name)
    ZEND_PARSE_PARAMETERS_END();
    
    array_init(return_value);
    
    if (!intern->ht_headers) {
        return;
    }
    
    zend_string *normalized = signalforge_normalize_header_name(ZSTR_VAL(name), ZSTR_LEN(name));
    val = zend_hash_find(intern->ht_headers, normalized);
    zend_string_release(normalized);
    if (val) {
        if (Z_TYPE_P(val) == IS_ARRAY) {
            ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(val), zval *item) {
                zval item_copy;
                ZVAL_COPY(&item_copy, item);
                add_next_index_zval(return_value, &item_copy);
            } ZEND_HASH_FOREACH_END();
        } else {
            zval val_copy;
            ZVAL_COPY(&val_copy, val);
            add_next_index_zval(return_value, &val_copy);
        }
    }
}
/* }}} */

/* {{{ getHeaderLine($name) */
PHP_METHOD(Signalforge_Http_Request, getHeaderLine)
{
    zend_string *name;
    signalforge_request_object *intern = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    zval *val;
    smart_str str = {0};
    
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(name)
    ZEND_PARSE_PARAMETERS_END();
    
    if (!intern->ht_headers) {
        RETURN_EMPTY_STRING();
    }
    
    zend_string *normalized = signalforge_normalize_header_name(ZSTR_VAL(name), ZSTR_LEN(name));
    val = zend_hash_find(intern->ht_headers, normalized);
    zend_string_release(normalized);
    if (!val) {
        RETURN_EMPTY_STRING();
    }
    
    if (Z_TYPE_P(val) == IS_ARRAY) {
        int first = 1;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(val), zval *item) {
            if (!first) {
                smart_str_appendc(&str, ',');
            }
            if (Z_TYPE_P(item) == IS_STRING) {
                smart_str_appendl(&str, Z_STRVAL_P(item), Z_STRLEN_P(item));
            }
            first = 0;
        } ZEND_HASH_FOREACH_END();
    } else if (Z_TYPE_P(val) == IS_STRING) {
        smart_str_appendl(&str, Z_STRVAL_P(val), Z_STRLEN_P(val));
    }
    
    smart_str_0(&str);
    RETURN_STR(str.s);
}
/* }}} */

/* {{{ withHeader($name, $value) */
PHP_METHOD(Signalforge_Http_Request, withHeader)
{
    zend_string *name;
    zval *value;
    signalforge_request_object *src, *dst;
    char normalized_name[256];
    size_t normalized_len;
    zval header_val;
    
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(name)
        Z_PARAM_ZVAL(value)
    ZEND_PARSE_PARAMETERS_END();
    
    /* Validate header name */
    if (!signalforge_validate_header_name(ZSTR_VAL(name), ZSTR_LEN(name))) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid header name '%s'", ZSTR_VAL(name));
        RETURN_THROWS();
    }

    /* Validate header value */
    if (Z_TYPE_P(value) == IS_ARRAY) {
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(value), zval *item) {
            if (Z_TYPE_P(item) == IS_STRING) {
                if (!signalforge_validate_header_value(Z_STRVAL_P(item), Z_STRLEN_P(item))) {
                    zend_throw_exception(spl_ce_InvalidArgumentException,
                        "Invalid header value (contains CR/LF/NUL)", 0);
                    RETURN_THROWS();
                }
            }
        } ZEND_HASH_FOREACH_END();
    } else if (Z_TYPE_P(value) == IS_STRING) {
        if (!signalforge_validate_header_value(Z_STRVAL_P(value), Z_STRLEN_P(value))) {
            zend_throw_exception(spl_ce_InvalidArgumentException,
                "Invalid header value (contains CR/LF/NUL)", 0);
            RETURN_THROWS();
        }
    }

    /* Normalize header name */
    zend_string *normalized = signalforge_normalize_header_name(ZSTR_VAL(name), ZSTR_LEN(name));
    
    /* Normalize value to array */
    if (Z_TYPE_P(value) == IS_ARRAY) {
        ZVAL_COPY(&header_val, value);
    } else {
        array_init(&header_val);
        Z_TRY_ADDREF_P(value);
        add_next_index_zval(&header_val, value);
    }
    
    src = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    dst = signalforge_request_clone(src, return_value);
    
    /* Ensure headers HashTable exists */
    if (!dst->ht_headers) {
        ALLOC_HASHTABLE(dst->ht_headers);
        zend_hash_init(dst->ht_headers, 16, NULL, ZVAL_PTR_DTOR, 0);
    }
    
    /* Set header */
    zend_hash_update(dst->ht_headers, normalized, &header_val);
    zend_string_release(normalized);
}
/* }}} */

/* {{{ withAddedHeader($name, $value) */
PHP_METHOD(Signalforge_Http_Request, withAddedHeader)
{
    zend_string *name;
    zval *value;
    signalforge_request_object *src, *dst;
    char normalized_name[256];
    size_t normalized_len;
    zval *existing_val;
    
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(name)
        Z_PARAM_ZVAL(value)
    ZEND_PARSE_PARAMETERS_END();
    
    if (!signalforge_validate_header_name(ZSTR_VAL(name), ZSTR_LEN(name))) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid header name '%s'", ZSTR_VAL(name));
        RETURN_THROWS();
    }

    /* Validate header value */
    if (Z_TYPE_P(value) == IS_NULL) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Header value cannot be null", 0);
        RETURN_THROWS();
    } else if (Z_TYPE_P(value) == IS_ARRAY) {
        if (zend_hash_num_elements(Z_ARRVAL_P(value)) == 0) {
            /* Empty array - don't add header */
            RETURN_ZVAL(ZEND_THIS, 1, 0);
        }
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(value), zval *item) {
            if (Z_TYPE_P(item) == IS_STRING) {
                if (!signalforge_validate_header_value(Z_STRVAL_P(item), Z_STRLEN_P(item))) {
                    zend_throw_exception(spl_ce_InvalidArgumentException,
                        "Invalid header value (contains CR/LF/NUL)", 0);
                    RETURN_THROWS();
                }
            }
        } ZEND_HASH_FOREACH_END();
    } else if (Z_TYPE_P(value) == IS_STRING) {
        if (!signalforge_validate_header_value(Z_STRVAL_P(value), Z_STRLEN_P(value))) {
            zend_throw_exception(spl_ce_InvalidArgumentException,
                "Invalid header value (contains CR/LF/NUL)", 0);
            RETURN_THROWS();
        }
    }

    zend_string *normalized = signalforge_normalize_header_name(ZSTR_VAL(name), ZSTR_LEN(name));
    
    src = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    dst = signalforge_request_clone(src, return_value);
    
    /* Ensure headers HashTable exists */
    if (!dst->ht_headers) {
        ALLOC_HASHTABLE(dst->ht_headers);
        zend_hash_init(dst->ht_headers, 16, NULL, ZVAL_PTR_DTOR, 0);
    }
    
    existing_val = zend_hash_find(dst->ht_headers, normalized);

    zval header_array;
    array_init(&header_array);

    /* Copy existing values to new array */
    if (existing_val) {
        if (Z_TYPE_P(existing_val) == IS_ARRAY) {
            /* Copy all existing array elements */
            zval *item;
            ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(existing_val), item) {
                zval item_copy;
                ZVAL_COPY(&item_copy, item);
                add_next_index_zval(&header_array, &item_copy);
            } ZEND_HASH_FOREACH_END();
        } else {
            /* Copy single existing value */
            zval existing_copy;
            ZVAL_COPY(&existing_copy, existing_val);
            add_next_index_zval(&header_array, &existing_copy);
        }
    }

    /* Add new value */
    zval val_copy;
    ZVAL_COPY(&val_copy, value);
    add_next_index_zval(&header_array, &val_copy);

    /* Update headers */
    zend_hash_update(dst->ht_headers, normalized, &header_array);
    
    zend_string_release(normalized);
}
/* }}} */

/* {{{ withoutHeader($name) */
PHP_METHOD(Signalforge_Http_Request, withoutHeader)
{
    zend_string *name;
    signalforge_request_object *src, *dst;
    
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(name)
    ZEND_PARSE_PARAMETERS_END();
    
    zend_string *normalized = signalforge_normalize_header_name(ZSTR_VAL(name), ZSTR_LEN(name));
    
    src = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    
    /* If header doesn't exist, return original */
    if (!src->ht_headers || !zend_hash_exists(src->ht_headers, normalized)) {
        zend_string_release(normalized);
        RETURN_ZVAL(ZEND_THIS, 1, 0);
    }
    
    dst = signalforge_request_clone(src, return_value);
    
    zend_hash_del(dst->ht_headers, normalized);
    zend_string_release(normalized);
}
/* }}} */

/* {{{ getBody() */
PHP_METHOD(Signalforge_Http_Request, getBody)
{
    signalforge_request_object *intern = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);

    ZEND_PARSE_PARAMETERS_NONE();

    /* Return stored body stream if one was set with withBody() */
    if (!Z_ISUNDEF(intern->zv_body)) {
        /* Return a copy - don't destroy source since object still owns it */
        RETURN_ZVAL(&intern->zv_body, 1, 0);
    }

    /* Otherwise, create stream from raw body string */
    zend_string *body_str;
    zval stream_zv, body_zv;

    /* Read body if not already read */
    body_str = signalforge_read_body(intern);

    /* Create Stream from body string using static method */
    ZVAL_STR(&body_zv, body_str);
    zend_call_method(NULL, signalforge_stream_ce, NULL, "fromstring", sizeof("fromstring")-1, &stream_zv, 1, &body_zv, NULL);

    if (Z_TYPE(stream_zv) == IS_OBJECT) {
        RETURN_ZVAL(&stream_zv, 0, 0);
    }

    /* Fallback: create empty stream */
    zval empty_zv;
    ZVAL_EMPTY_STRING(&empty_zv);
    zend_call_method(NULL, signalforge_stream_ce, NULL, "fromstring", sizeof("fromstring")-1, return_value, 1, &empty_zv, NULL);
}
/* }}} */

/* {{{ withBody(StreamInterface $body) */
PHP_METHOD(Signalforge_Http_Request, withBody)
{
    zval *body;
    signalforge_request_object *src, *dst;
    
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(body, signalforge_stream_ce)
    ZEND_PARSE_PARAMETERS_END();
    
    src = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    dst = signalforge_request_clone(src, return_value);
    
    /* Store body reference */
    if (!Z_ISUNDEF(dst->zv_body)) zval_ptr_dtor(&dst->zv_body);
    ZVAL_COPY(&dst->zv_body, body);
    
    /* Clear body read flag since we're replacing it */
    dst->flags &= ~SF_REQ_FLAG_BODY_READ;
}
/* }}} */

/* ============================================================================
 * PSR-7 RequestInterface Methods
 * ============================================================================ */

/* {{{ getRequestTarget() */
PHP_METHOD(Signalforge_Http_Request, getRequestTarget)
{
    signalforge_request_object *intern = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();

    if (Z_TYPE(intern->zv_uri) == IS_STRING) {
        RETURN_ZVAL(&intern->zv_uri, 1, 0);
    }
    RETURN_STRING("/");
}
/* }}} */

/* {{{ withRequestTarget($requestTarget) */
PHP_METHOD(Signalforge_Http_Request, withRequestTarget)
{
    zend_string *target;
    signalforge_request_object *src, *dst;
    
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(target)
    ZEND_PARSE_PARAMETERS_END();
    
    src = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    dst = signalforge_request_clone(src, return_value);
    
    /* Update URI */
    if (!Z_ISUNDEF(dst->zv_uri)) zval_ptr_dtor(&dst->zv_uri);
    ZVAL_STR(&dst->zv_uri, zend_string_copy(target));
    dst->request_uri = Z_STRVAL(dst->zv_uri);
    dst->request_uri_len = Z_STRLEN(dst->zv_uri);
    
    /* Clear path cache */
    dst->flags &= ~SF_REQ_FLAG_PATH_PARSED;
}
/* }}} */

/* {{{ getMethod() */
/**
 * Get the HTTP method for this request.
 *
 * Returns the HTTP method (GET, POST, etc.) with any method override
 * handling applied. Checks for X-HTTP-Method-Override headers and
 * _method POST parameters. Result is cached after first resolution.
 *
 * @return string HTTP method (uppercase)
 */
PHP_METHOD(Signalforge_Http_Request, getMethod)
{
    signalforge_request_object *intern = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();
    
    signalforge_resolve_method(intern);
    
    if (Z_TYPE(intern->zv_method) == IS_STRING) {
        RETURN_ZVAL(&intern->zv_method, 1, 0);
    }
    RETURN_STRING("GET");
}
/* }}} */

/* {{{ withMethod($method) */
PHP_METHOD(Signalforge_Http_Request, withMethod)
{
    zend_string *method;
    signalforge_request_object *src, *dst;
    
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(method)
    ZEND_PARSE_PARAMETERS_END();
    
    /* Validate method */
    if (ZSTR_LEN(method) == 0) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "HTTP method cannot be empty", 0);
        RETURN_THROWS();
    }
    
    /* Validate against RFC 7231 standard methods */
    const char *method_str = ZSTR_VAL(method);
    size_t method_len = ZSTR_LEN(method);
    zend_bool valid = 0;
    
    /* Case-insensitive comparison against standard methods */
    if (method_len == 3 && strncasecmp(method_str, "GET", 3) == 0) valid = 1;
    else if (method_len == 4 && strncasecmp(method_str, "POST", 4) == 0) valid = 1;
    else if (method_len == 3 && strncasecmp(method_str, "PUT", 3) == 0) valid = 1;
    else if (method_len == 6 && strncasecmp(method_str, "DELETE", 6) == 0) valid = 1;
    else if (method_len == 5 && strncasecmp(method_str, "PATCH", 5) == 0) valid = 1;
    else if (method_len == 4 && strncasecmp(method_str, "HEAD", 4) == 0) valid = 1;
    else if (method_len == 7 && strncasecmp(method_str, "OPTIONS", 7) == 0) valid = 1;
    else if (method_len == 5 && strncasecmp(method_str, "TRACE", 5) == 0) valid = 1;
    else if (method_len == 7 && strncasecmp(method_str, "CONNECT", 7) == 0) valid = 1;
    
    if (!valid) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
            "Invalid HTTP method '%s'. Must be one of: GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS, TRACE, CONNECT", method_str);
        RETURN_THROWS();
    }
    
    src = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    dst = signalforge_request_clone(src, return_value);
    
    /* Set method */
    if (!Z_ISUNDEF(dst->zv_method)) zval_ptr_dtor(&dst->zv_method);
    ZVAL_STR(&dst->zv_method, zend_string_copy(method));
    signalforge_strtoupper(Z_STRVAL(dst->zv_method), Z_STRLEN(dst->zv_method));
    dst->flags |= SF_REQ_FLAG_METHOD_RESOLVED;
}
/* }}} */

/* {{{ getUri() */
/**
 * Get the URI for this request.
 *
 * Returns a UriInterface object representing the request URI.
 * If an absolute URI was set via withUri(), returns that.
 * Otherwise, reconstructs from $_SERVER data (REQUEST_URI, HTTP_HOST, etc).
 *
 * @return UriInterface Request URI object
 */
PHP_METHOD(Signalforge_Http_Request, getUri)
{
    signalforge_request_object *intern = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();

    /* Check if stored URI is already an absolute URI (contains ://) */
    if (Z_TYPE(intern->zv_uri) == IS_STRING && Z_STRLEN(intern->zv_uri) > 0) {
        const char *uri_str = Z_STRVAL(intern->zv_uri);
        size_t uri_len = Z_STRLEN(intern->zv_uri);

        /* If URI contains "://", it's absolute - use it directly */
        if (strstr(uri_str, "://") != NULL) {
            zend_object *uri_obj = signalforge_uri_create_from_string(uri_str, uri_len);
            RETURN_OBJ(uri_obj);
        }
    }

    /* Build full URI from server params */
    smart_str uri_buf = {0};

    /* Get scheme (https or http) */
    zval *https = NULL;
    if (Z_TYPE(intern->zv_server) == IS_ARRAY) {
        https = zend_hash_str_find(Z_ARRVAL(intern->zv_server), "HTTPS", sizeof("HTTPS")-1);
    }
    if (https && Z_TYPE_P(https) == IS_STRING &&
        strcasecmp(Z_STRVAL_P(https), "off") != 0 && Z_STRLEN_P(https) > 0) {
        smart_str_appendl(&uri_buf, "https://", 8);
    } else {
        smart_str_appendl(&uri_buf, "http://", 7);
    }

    /* Get host */
    zval *host = NULL;
    if (Z_TYPE(intern->zv_server) == IS_ARRAY) {
        host = zend_hash_str_find(Z_ARRVAL(intern->zv_server), "HTTP_HOST", sizeof("HTTP_HOST")-1);
        if (!host) {
            host = zend_hash_str_find(Z_ARRVAL(intern->zv_server), "SERVER_NAME", sizeof("SERVER_NAME")-1);
        }
    }
    if (host && Z_TYPE_P(host) == IS_STRING) {
        smart_str_appendl(&uri_buf, Z_STRVAL_P(host), Z_STRLEN_P(host));
    } else {
        smart_str_appendl(&uri_buf, "localhost", 9);
    }

    /* Get path from stored URI or REQUEST_URI */
    if (Z_TYPE(intern->zv_uri) == IS_STRING && Z_STRLEN(intern->zv_uri) > 0) {
        smart_str_appendl(&uri_buf, Z_STRVAL(intern->zv_uri), Z_STRLEN(intern->zv_uri));
    } else if (intern->request_uri && intern->request_uri_len > 0) {
        smart_str_appendl(&uri_buf, intern->request_uri, intern->request_uri_len);
    } else {
        smart_str_appendl(&uri_buf, "/", 1);
    }

    smart_str_0(&uri_buf);

    /* Create Uri object from the string */
    if (uri_buf.s) {
        zend_object *uri_obj = signalforge_uri_create_from_string(ZSTR_VAL(uri_buf.s), ZSTR_LEN(uri_buf.s));
        RETVAL_OBJ(uri_obj);
        smart_str_free(&uri_buf);
    } else {
        /* Fallback: create empty Uri */
        object_init_ex(return_value, signalforge_uri_ce);
    }
}
/* }}} */

/* {{{ withUri(UriInterface|string $uri, $preserveHost = false) */
PHP_METHOD(Signalforge_Http_Request, withUri)
{
    zval *uri;
    zend_bool preserve_host = 0;
    signalforge_request_object *src, *dst;
    zend_string *uri_str = NULL;
    signalforge_uri_object *uri_obj = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_ZVAL(uri)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(preserve_host)
    ZEND_PARSE_PARAMETERS_END();

    /* Accept both Uri objects and strings */
    if (Z_TYPE_P(uri) == IS_OBJECT && instanceof_function(Z_OBJCE_P(uri), signalforge_uri_ce)) {
        /* Uri object - get string representation */
        uri_obj = Z_SIGNALFORGE_URI_P(uri);

        /* Build URI string from object */
        smart_str buf = {0};

        if (uri_obj->scheme && ZSTR_LEN(uri_obj->scheme) > 0) {
            smart_str_append(&buf, uri_obj->scheme);
            smart_str_appendc(&buf, ':');
        }
        if (uri_obj->host && ZSTR_LEN(uri_obj->host) > 0) {
            smart_str_appendl(&buf, "//", 2);
            if (uri_obj->user && ZSTR_LEN(uri_obj->user) > 0) {
                smart_str_append(&buf, uri_obj->user);
                if (uri_obj->pass && ZSTR_LEN(uri_obj->pass) > 0) {
                    smart_str_appendc(&buf, ':');
                    smart_str_append(&buf, uri_obj->pass);
                }
                smart_str_appendc(&buf, '@');
            }
            smart_str_append(&buf, uri_obj->host);
            if (uri_obj->port != SIGNALFORGE_PORT_UNSET &&
                !signalforge_is_standard_port(uri_obj->scheme, uri_obj->port)) {
                smart_str_appendc(&buf, ':');
                smart_str_append_long(&buf, uri_obj->port);
            }
        }
        if (uri_obj->path && ZSTR_LEN(uri_obj->path) > 0) {
            if (uri_obj->host && ZSTR_LEN(uri_obj->host) > 0 &&
                ZSTR_VAL(uri_obj->path)[0] != '/') {
                smart_str_appendc(&buf, '/');
            }
            smart_str_append(&buf, uri_obj->path);
        }
        if (uri_obj->query && ZSTR_LEN(uri_obj->query) > 0) {
            smart_str_appendc(&buf, '?');
            smart_str_append(&buf, uri_obj->query);
        }
        if (uri_obj->fragment && ZSTR_LEN(uri_obj->fragment) > 0) {
            smart_str_appendc(&buf, '#');
            smart_str_append(&buf, uri_obj->fragment);
        }

        smart_str_0(&buf);
        uri_str = buf.s ? buf.s : zend_string_init("", 0, 0);
    } else if (Z_TYPE_P(uri) == IS_STRING) {
        /* String - use directly */
        uri_str = zend_string_copy(Z_STR_P(uri));
    } else {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "URI must be a UriInterface or string", 0);
        RETURN_THROWS();
    }

    src = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    dst = signalforge_request_clone(src, return_value);

    /* Update URI */
    if (!Z_ISUNDEF(dst->zv_uri)) zval_ptr_dtor(&dst->zv_uri);
    ZVAL_STR(&dst->zv_uri, zend_string_copy(uri_str));
    dst->request_uri = Z_STRVAL(dst->zv_uri);
    dst->request_uri_len = Z_STRLEN(dst->zv_uri);

    /* Handle Host header preservation */
    if (!preserve_host) {
        zend_string *new_host = NULL;
        zend_long new_port = SIGNALFORGE_PORT_UNSET;

        if (uri_obj) {
            /* Get host from Uri object */
            if (uri_obj->host && ZSTR_LEN(uri_obj->host) > 0) {
                new_host = uri_obj->host;
                new_port = uri_obj->port;
            }
        } else {
            /* Extract host from string */
            char *host_start = strstr(ZSTR_VAL(uri_str), "://");
            if (host_start) {
                host_start += 3;
                /* Skip userinfo if present */
                char *at = strchr(host_start, '@');
                char *slash = strchr(host_start, '/');
                if (at && (!slash || at < slash)) {
                    host_start = at + 1;
                }

                char *host_end = slash;
                if (!host_end) host_end = strchr(host_start, '?');
                if (!host_end) host_end = strchr(host_start, '#');
                if (!host_end) host_end = ZSTR_VAL(uri_str) + ZSTR_LEN(uri_str);

                if (host_end > host_start) {
                    new_host = zend_string_init(host_start, host_end - host_start, 0);
                }
            }
        }

        if (new_host && ZSTR_LEN(new_host) > 0) {
            /* Ensure headers HashTable exists */
            if (!dst->ht_headers) {
                ALLOC_HASHTABLE(dst->ht_headers);
                zend_hash_init(dst->ht_headers, 16, NULL, ZVAL_PTR_DTOR, 0);
            }

            /* Build host value with port if non-standard */
            smart_str host_buf = {0};
            smart_str_append(&host_buf, new_host);
            if (new_port != SIGNALFORGE_PORT_UNSET && new_port != 80 && new_port != 443) {
                smart_str_appendc(&host_buf, ':');
                smart_str_append_long(&host_buf, new_port);
            }
            smart_str_0(&host_buf);

            /* Create zval for host value */
            zval host_val;
            array_init(&host_val);
            if (host_buf.s) {
                add_next_index_str(&host_val, host_buf.s);
            }

            /* Normalize header name and update */
            zend_string *normalized = signalforge_normalize_header_name("host", sizeof("host")-1);
            zend_hash_update(dst->ht_headers, normalized, &host_val);
            zend_string_release(normalized);

            /* Free temp host string if we allocated it */
            if (!uri_obj && new_host) {
                zend_string_release(new_host);
            }
        }
    }

    /* Clear path cache */
    dst->flags &= ~SF_REQ_FLAG_PATH_PARSED;

    zend_string_release(uri_str);
}
/* }}} */

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

/* Validate that array doesn't contain nested arrays */
static zend_bool signalforge_validate_flat_array(zval *array)
{
    if (Z_TYPE_P(array) != IS_ARRAY) {
        return 0;
    }

    zval *val;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(array), val) {
        if (Z_TYPE_P(val) == IS_ARRAY) {
            return 0; /* Nested array found */
        }
    } ZEND_HASH_FOREACH_END();

    return 1; /* Array is flat */
}

/* ============================================================================
 * PSR-7 ServerRequestInterface Methods
 * ============================================================================ */

/* {{{ getServerParams() */
PHP_METHOD(Signalforge_Http_Request, getServerParams)
{
    signalforge_request_object *intern = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();
    
    if (Z_TYPE(intern->zv_server) == IS_ARRAY) {
        RETURN_ARR(zend_array_dup(Z_ARRVAL(intern->zv_server)));
    }
    array_init(return_value);
}
/* }}} */

/* {{{ getCookieParams() */
PHP_METHOD(Signalforge_Http_Request, getCookieParams)
{
    signalforge_request_object *intern = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();
    
    if (Z_TYPE(intern->zv_cookie) == IS_ARRAY) {
        RETURN_ARR(zend_array_dup(Z_ARRVAL(intern->zv_cookie)));
    }
    array_init(return_value);
}
/* }}} */

/* {{{ withCookieParams(array $cookies) */
PHP_METHOD(Signalforge_Http_Request, withCookieParams)
{
    zval *cookies;
    signalforge_request_object *src, *dst;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(cookies)
    ZEND_PARSE_PARAMETERS_END();

    /* Validate that cookies is an array */
    if (Z_TYPE_P(cookies) != IS_ARRAY) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Cookies must be an array", 0);
        RETURN_THROWS();
    }

    /* Validate that cookies array is flat (no nested arrays) */
    if (!signalforge_validate_flat_array(cookies)) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Cookie values must not be arrays", 0);
        RETURN_THROWS();
    }

    src = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    dst = signalforge_request_clone(src, return_value);

    /* Update cookie reference with proper refcounting */
    if (!Z_ISUNDEF(dst->zv_cookie)) zval_ptr_dtor(&dst->zv_cookie);
    ZVAL_COPY(&dst->zv_cookie, cookies);
}
/* }}} */

/* {{{ getQueryParams() */
PHP_METHOD(Signalforge_Http_Request, getQueryParams)
{
    signalforge_request_object *intern = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();
    
    if (Z_TYPE(intern->zv_get) == IS_ARRAY) {
        RETURN_ARR(zend_array_dup(Z_ARRVAL(intern->zv_get)));
    }
    array_init(return_value);
}
/* }}} */

/* {{{ withQueryParams(array $query) */
PHP_METHOD(Signalforge_Http_Request, withQueryParams)
{
    zval *query;
    signalforge_request_object *src, *dst;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(query)
    ZEND_PARSE_PARAMETERS_END();

    /* Validate that query is an array */
    if (Z_TYPE_P(query) != IS_ARRAY) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Query params must be an array", 0);
        RETURN_THROWS();
    }

    /* Validate that query array is flat (no nested arrays) */
    if (!signalforge_validate_flat_array(query)) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Query param values must not be arrays", 0);
        RETURN_THROWS();
    }

    src = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    dst = signalforge_request_clone(src, return_value);

    /* Update query reference with proper refcounting */
    if (!Z_ISUNDEF(dst->zv_get)) zval_ptr_dtor(&dst->zv_get);
    ZVAL_COPY(&dst->zv_get, query);

    /* Clear input cache */
    dst->flags &= ~SF_REQ_FLAG_INPUT_MERGED;
    if (dst->ht_input) {
        zend_hash_destroy(dst->ht_input);
        FREE_HASHTABLE(dst->ht_input);
        dst->ht_input = NULL;
    }
}
/* }}} */

/* {{{ getUploadedFiles() */
PHP_METHOD(Signalforge_Http_Request, getUploadedFiles)
{
    signalforge_request_object *intern = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    zend_string *key;
    zval *val;

    ZEND_PARSE_PARAMETERS_NONE();

    array_init(return_value);

    if (Z_TYPE(intern->zv_files) == IS_ARRAY) {
        /* Convert $_FILES array entries to UploadedFile objects */
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL(intern->zv_files), key, val) {
            if (key && Z_TYPE_P(val) == IS_ARRAY) {
                zval uploaded_file_zv;
                /* Create UploadedFile object from $_FILES entry */
                signalforge_uploadedfile_from_files_array(val, &uploaded_file_zv);

                /* Add to result array */
                zend_hash_update(Z_ARRVAL_P(return_value), key, &uploaded_file_zv);
            } else {
                /* For non-array values, copy them as-is */
                zval copy_val;
                ZVAL_COPY(&copy_val, val);
                zend_hash_update(Z_ARRVAL_P(return_value), key, &copy_val);
            }
        } ZEND_HASH_FOREACH_END();
    }
}
/* }}} */

/* {{{ withUploadedFiles(array $uploadedFiles) */
PHP_METHOD(Signalforge_Http_Request, withUploadedFiles)
{
    zval *files;
    signalforge_request_object *src, *dst;
    
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(files)
    ZEND_PARSE_PARAMETERS_END();
    
    src = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    dst = signalforge_request_clone(src, return_value);
    
    /* Update files reference with proper refcounting */
    if (!Z_ISUNDEF(dst->zv_files)) zval_ptr_dtor(&dst->zv_files);
    ZVAL_COPY(&dst->zv_files, files);
}
/* }}} */

/* {{{ getParsedBody() */
PHP_METHOD(Signalforge_Http_Request, getParsedBody)
{
    signalforge_request_object *intern = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();

    /* Return explicitly set parsed body (from withParsedBody()) */
    if (!Z_ISUNDEF(intern->zv_json)) {
        if (Z_TYPE(intern->zv_json) != IS_NULL) {
            RETURN_ZVAL(&intern->zv_json, 1, 0);
        }
        RETURN_NULL();
    }

    /* Return POST data if available (for form submissions) */
    if (Z_TYPE(intern->zv_post) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL(intern->zv_post)) > 0) {
        RETURN_ARR(zend_array_dup(Z_ARRVAL(intern->zv_post)));
    }

    /* Check if JSON body */
    signalforge_parse_content_type(intern);
    if (!Z_ISNULL(intern->zv_content_type) &&
        strcasecmp(Z_STRVAL(intern->zv_content_type), "application/json") == 0) {

        /* Parse JSON if not already done */
        if (!(intern->flags & SF_REQ_FLAG_JSON_PARSED)) {
            zend_string *body = signalforge_read_body(intern);
            if (body && ZSTR_LEN(body) > 0) {
                php_json_decode(&intern->zv_json, ZSTR_VAL(body), ZSTR_LEN(body), 1, PHP_JSON_PARSER_DEFAULT_DEPTH);
                intern->flags |= SF_REQ_FLAG_JSON_PARSED;
            } else {
                ZVAL_NULL(&intern->zv_json);
                intern->flags |= SF_REQ_FLAG_JSON_PARSED;
            }
        }

        /* Return the parsed JSON */
        if (Z_TYPE(intern->zv_json) != IS_NULL) {
            RETURN_ZVAL(&intern->zv_json, 1, 0);
        }
    }

    RETURN_NULL();
}
/* }}} */

/* {{{ withParsedBody($data) */
PHP_METHOD(Signalforge_Http_Request, withParsedBody)
{
    zval *data;
    signalforge_request_object *src, *dst;
    
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(data)
    ZEND_PARSE_PARAMETERS_END();

    /* Validate that data is array, object, or null */
    if (Z_TYPE_P(data) != IS_ARRAY && Z_TYPE_P(data) != IS_OBJECT && Z_TYPE_P(data) != IS_NULL) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Parsed body must be an array, object, or null", 0);
        RETURN_THROWS();
    }

    src = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    dst = signalforge_request_clone(src, return_value);
    
    /* Store parsed body */
    if (!Z_ISUNDEF(dst->zv_json)) zval_ptr_dtor(&dst->zv_json);
    ZVAL_COPY(&dst->zv_json, data);
    dst->flags |= SF_REQ_FLAG_JSON_PARSED;
}
/* }}} */

/* {{{ getAttributes() */
PHP_METHOD(Signalforge_Http_Request, getAttributes)
{
    signalforge_request_object *intern = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();
    
    if (intern->ht_attributes) {
        RETURN_ARR(zend_array_dup(intern->ht_attributes));
    }
    array_init(return_value);
}
/* }}} */

/* {{{ getAttribute($name, $default = null) */
PHP_METHOD(Signalforge_Http_Request, getAttribute)
{
    zend_string *name;
    zval *default_val = NULL;
    signalforge_request_object *intern = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    zval *val;
    
    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(name)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL(default_val)
    ZEND_PARSE_PARAMETERS_END();
    
    if (intern->ht_attributes) {
        val = zend_hash_find(intern->ht_attributes, name);
        if (val) {
            RETURN_ZVAL(val, 1, 0);
        }
    }
    
    if (default_val) {
        RETURN_ZVAL(default_val, 1, 0);
    }
    RETURN_NULL();
}
/* }}} */

/* {{{ withAttribute($name, $value) */
PHP_METHOD(Signalforge_Http_Request, withAttribute)
{
    zend_string *name;
    zval *value;
    signalforge_request_object *src, *dst;
    
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(name)
        Z_PARAM_ZVAL(value)
    ZEND_PARSE_PARAMETERS_END();
    
    src = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    dst = signalforge_request_clone(src, return_value);
    
    /* Ensure attributes HashTable exists */
    if (!dst->ht_attributes) {
        ALLOC_HASHTABLE(dst->ht_attributes);
        zend_hash_init(dst->ht_attributes, 8, NULL, ZVAL_PTR_DTOR, 0);
    }
    
    zval val_copy;
    ZVAL_COPY(&val_copy, value);
    zend_hash_update(dst->ht_attributes, name, &val_copy);
}
/* }}} */

/* {{{ withoutAttribute($name) */
PHP_METHOD(Signalforge_Http_Request, withoutAttribute)
{
    zend_string *name;
    signalforge_request_object *src, *dst;
    
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(name)
    ZEND_PARSE_PARAMETERS_END();
    
    src = Z_SIGNALFORGE_REQUEST_P(ZEND_THIS);
    
    /* If attribute doesn't exist, return original */
    if (!src->ht_attributes || !zend_hash_exists(src->ht_attributes, name)) {
        RETURN_ZVAL(ZEND_THIS, 1, 0);
    }
    
    dst = signalforge_request_clone(src, return_value);
    zend_hash_del(dst->ht_attributes, name);
}
/* }}} */

/* ============================================================================
 * ARGINFO DEFINITIONS
 * ============================================================================ */

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_request_capture, 0, 0, Signalforge\\NativeHttp\\Request, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_request_create, 0, 2, Signalforge\\NativeHttp\\Request, 0)
    ZEND_ARG_TYPE_INFO(0, method, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, uri, IS_MIXED, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, serverParams, IS_ARRAY, 1, "[]")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_request_getProtocolVersion, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_request_withProtocolVersion, 0, 1, Signalforge\\NativeHttp\\Request, 0)
    ZEND_ARG_TYPE_INFO(0, version, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_request_getHeaders, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_request_hasHeader, 0, 1, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_request_getHeader, 0, 1, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_request_getHeaderLine, 0, 1, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_request_withHeader, 0, 2, Signalforge\\NativeHttp\\Request, 0)
    ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_request_withAddedHeader, 0, 2, Signalforge\\NativeHttp\\Request, 0)
    ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_request_withoutHeader, 0, 1, Signalforge\\NativeHttp\\Request, 0)
    ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_request_getBody, 0, 0, Psr\\Http\\Message\\StreamInterface, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_request_withBody, 0, 1, Signalforge\\NativeHttp\\Request, 0)
    ZEND_ARG_OBJ_INFO(0, body, Psr\\Http\\Message\\StreamInterface, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_request_getRequestTarget, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_request_withRequestTarget, 0, 1, Signalforge\\NativeHttp\\Request, 0)
    ZEND_ARG_TYPE_INFO(0, requestTarget, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_request_getMethod, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_request_withMethod, 0, 1, Signalforge\\NativeHttp\\Request, 0)
    ZEND_ARG_TYPE_INFO(0, method, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_request_getUri, 0, 0, Signalforge\\NativeHttp\\Uri, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_request_withUri, 0, 1, Signalforge\\NativeHttp\\Request, 0)
    ZEND_ARG_TYPE_INFO(0, uri, IS_MIXED, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, preserveHost, _IS_BOOL, 0, "false")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_request_getServerParams, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_request_getCookieParams, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_request_withCookieParams, 0, 1, Signalforge\\NativeHttp\\Request, 0)
    ZEND_ARG_TYPE_INFO(0, cookies, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_request_getQueryParams, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_request_withQueryParams, 0, 1, Signalforge\\NativeHttp\\Request, 0)
    ZEND_ARG_TYPE_INFO(0, query, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_request_getUploadedFiles, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_request_withUploadedFiles, 0, 1, Signalforge\\NativeHttp\\Request, 0)
    ZEND_ARG_TYPE_INFO(0, uploadedFiles, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_request_getParsedBody, 0, 0, IS_MIXED, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_request_withParsedBody, 0, 1, Signalforge\\NativeHttp\\Request, 0)
    ZEND_ARG_TYPE_INFO(0, data, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_request_getAttributes, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_request_getAttribute, 0, 1, IS_MIXED, 1)
    ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, default, IS_MIXED, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_request_withAttribute, 0, 2, Signalforge\\NativeHttp\\Request, 0)
    ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_request_withoutAttribute, 0, 1, Signalforge\\NativeHttp\\Request, 0)
    ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()

/* ============================================================================
 * METHOD REGISTRATION
 * ============================================================================ */

static const zend_function_entry signalforge_request_methods[] = {
    PHP_ME(Signalforge_Http_Request, capture, arginfo_request_capture, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(Signalforge_Http_Request, create, arginfo_request_create, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)

    /* MessageInterface */
    PHP_ME(Signalforge_Http_Request, getProtocolVersion, arginfo_request_getProtocolVersion, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, withProtocolVersion, arginfo_request_withProtocolVersion, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, getHeaders, arginfo_request_getHeaders, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, hasHeader, arginfo_request_hasHeader, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, getHeader, arginfo_request_getHeader, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, getHeaderLine, arginfo_request_getHeaderLine, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, withHeader, arginfo_request_withHeader, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, withAddedHeader, arginfo_request_withAddedHeader, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, withoutHeader, arginfo_request_withoutHeader, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, getBody, arginfo_request_getBody, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, withBody, arginfo_request_withBody, ZEND_ACC_PUBLIC)
    
    /* RequestInterface */
    PHP_ME(Signalforge_Http_Request, getRequestTarget, arginfo_request_getRequestTarget, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, withRequestTarget, arginfo_request_withRequestTarget, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, getMethod, arginfo_request_getMethod, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, withMethod, arginfo_request_withMethod, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, getUri, arginfo_request_getUri, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, withUri, arginfo_request_withUri, ZEND_ACC_PUBLIC)
    
    /* ServerRequestInterface */
    PHP_ME(Signalforge_Http_Request, getServerParams, arginfo_request_getServerParams, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, getCookieParams, arginfo_request_getCookieParams, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, withCookieParams, arginfo_request_withCookieParams, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, getQueryParams, arginfo_request_getQueryParams, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, withQueryParams, arginfo_request_withQueryParams, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, getUploadedFiles, arginfo_request_getUploadedFiles, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, withUploadedFiles, arginfo_request_withUploadedFiles, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, getParsedBody, arginfo_request_getParsedBody, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, withParsedBody, arginfo_request_withParsedBody, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, getAttributes, arginfo_request_getAttributes, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, getAttribute, arginfo_request_getAttribute, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, withAttribute, arginfo_request_withAttribute, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Request, withoutAttribute, arginfo_request_withoutAttribute, ZEND_ACC_PUBLIC)
    
    PHP_FE_END
};

void signalforge_request_register_class(void)
{
    zend_class_entry ce;

    INIT_NS_CLASS_ENTRY(ce, "Signalforge\\NativeHttp", "Request", signalforge_request_methods);
    signalforge_request_ce = zend_register_internal_class(&ce);
    signalforge_request_ce->ce_flags |= ZEND_ACC_FINAL;
    signalforge_request_ce->create_object = signalforge_request_create_object;

    /* Copy object handlers */
    memcpy(&signalforge_request_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    signalforge_request_object_handlers.offset = XtOffsetOf(signalforge_request_object, std);
    signalforge_request_object_handlers.free_obj = signalforge_request_free_object;
    signalforge_request_object_handlers.clone_obj = signalforge_request_clone_obj;
}

