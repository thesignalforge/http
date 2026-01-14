/*
 * response.c
 *
 * Signalforge HTTP Response Class Implementation
 *
 * This file implements the Signalforge\Http\Response class, providing
 * PSR-7 compliant Response with zero-copy optimizations.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_signalforge_http.h"
#include "psr7_interfaces.h"
#include "response.h"
#include "request.h"  // For signalforge_normalize_header_name
#include "stream.h"
#include "zend_smart_str.h"
#include "SAPI.h"
#include "ext/json/php_json.h"

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

zend_class_entry *signalforge_response_ce = NULL;
static zend_object_handlers signalforge_response_object_handlers;

/* Forward declaration */
extern zend_class_entry *signalforge_stream_ce;

/* SPL Exception classes (available in PHP 8.3) */
extern PHPAPI zend_class_entry *spl_ce_InvalidArgumentException;
extern PHPAPI zend_class_entry *spl_ce_RuntimeException;

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

const char *signalforge_get_reason_phrase(zend_long status_code)
{
    switch (status_code) {
        case 100: return "Continue";
        case 101: return "Switching Protocols";
        case 102: return "Processing";
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 203: return "Non-Authoritative Information";
        case 204: return "No Content";
        case 205: return "Reset Content";
        case 206: return "Partial Content";
        case 207: return "Multi-Status";
        case 208: return "Already Reported";
        case 226: return "IM Used";
        case 300: return "Multiple Choices";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 305: return "Use Proxy";
        case 307: return "Temporary Redirect";
        case 308: return "Permanent Redirect";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 402: return "Payment Required";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 406: return "Not Acceptable";
        case 407: return "Proxy Authentication Required";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 411: return "Length Required";
        case 412: return "Precondition Failed";
        case 413: return "Payload Too Large";
        case 414: return "URI Too Long";
        case 415: return "Unsupported Media Type";
        case 416: return "Range Not Satisfiable";
        case 417: return "Expectation Failed";
        case 418: return "I'm a teapot";
        case 421: return "Misdirected Request";
        case 422: return "Unprocessable Entity";
        case 423: return "Locked";
        case 424: return "Failed Dependency";
        case 425: return "Too Early";
        case 426: return "Upgrade Required";
        case 428: return "Precondition Required";
        case 429: return "Too Many Requests";
        case 431: return "Request Header Fields Too Large";
        case 451: return "Unavailable For Legal Reasons";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
        case 506: return "Variant Also Negotiates";
        case 507: return "Insufficient Storage";
        case 508: return "Loop Detected";
        case 510: return "Not Extended";
        case 511: return "Network Authentication Required";
        default: return "Unknown Status";
    }
}

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

zend_string *signalforge_serialize_headers(HashTable *headers)
{
    smart_str str = {0};
    zend_string *key;
    zval *val;

    if (!headers || zend_hash_num_elements(headers) == 0) {
        return zend_string_init("", 0, 0);
    }

    ZEND_HASH_FOREACH_STR_KEY_VAL(headers, key, val) {
        if (!key) continue;

        /* Header name (capitalize first letter of each word) */
        char *name = ZSTR_VAL(key);
        size_t len = ZSTR_LEN(key);
        smart_str_appendl(&str, name, len);
        smart_str_appends(&str, ": ");

        /* Header value(s) */
        if (Z_TYPE_P(val) == IS_ARRAY) {
            zval *item;
            int first = 1;

            ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(val), item) {
                if (!first) {
                    smart_str_appends(&str, ", ");
                }
                first = 0;
                if (Z_TYPE_P(item) == IS_STRING) {
                    smart_str_appendl(&str, Z_STRVAL_P(item), Z_STRLEN_P(item));
                }
            } ZEND_HASH_FOREACH_END();
        } else if (Z_TYPE_P(val) == IS_STRING) {
            smart_str_appendl(&str, Z_STRVAL_P(val), Z_STRLEN_P(val));
        }

        smart_str_appends(&str, "\r\n");
    } ZEND_HASH_FOREACH_END();

    smart_str_0(&str);
    return str.s;
}

signalforge_response_object *signalforge_response_clone(signalforge_response_object *src, zval *return_value)
{
    signalforge_response_object *dst;

    /* Create new instance */
    object_init_ex(return_value, signalforge_response_ce);
    dst = Z_SIGNALFORGE_RESPONSE_P(return_value);

    /* Copy headers */
    dst->ht_headers = signalforge_clone_headers(src->ht_headers);

    /* Copy body */
    ZVAL_COPY(&dst->zv_body, &src->zv_body);
    dst->body_is_stream = src->body_is_stream;

    /* Copy protocol and status */
    if (src->protocol_version) {
        dst->protocol_version = zend_string_copy(src->protocol_version);
    } else {
        dst->protocol_version = NULL;
    }
    dst->status_code = src->status_code;
    if (src->reason_phrase) {
        dst->reason_phrase = zend_string_copy(src->reason_phrase);
    } else {
        dst->reason_phrase = NULL;
    }


    return dst;
}

zend_bool signalforge_validate_header_name(const char *name, size_t len)
{
    size_t i;
    /* RFC 7230: token = 1*tchar */
    if (len == 0) {
        return 0;
    }
    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)name[i];
        /* Control chars, space, and DEL are invalid; colon is delimiter */
        if (c <= 0x20 || c >= 0x7f || c == ':' || c == '(' || c == ')' ||
            c == '<' || c == '>' || c == '@' || c == ',' || c == ';' ||
            c == '\\' || c == '"' || c == '/' || c == '[' || c == ']' ||
            c == '?' || c == '=' || c == '{' || c == '}') {
            return 0;
        }
    }
    return 1;
}

zend_bool signalforge_validate_header_value(const char *value, size_t len)
{
    size_t i;
    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)value[i];
        /* NUL (0x00), CR (0x0D), and LF (0x0A) are forbidden per PSR-7-Meta Section 7.1 */
        if (c == '\0' || c == '\r' || c == '\n') {
            return 0;
        }
    }
    return 1;
}

zend_bool signalforge_validate_status_code(zend_long code)
{
    return (code >= 100 && code <= 599);
}

/* ============================================================================
 * OBJECT HANDLERS
 * ============================================================================ */

static zend_object *signalforge_response_create_object(zend_class_entry *ce)
{
    signalforge_response_object *intern;

    intern = zend_object_alloc(sizeof(signalforge_response_object), ce);

    /* Initialize status */
    intern->status_code = 200;
    intern->reason_phrase = NULL;

    /* Initialize headers */
    ALLOC_HASHTABLE(intern->ht_headers);
    zend_hash_init(intern->ht_headers, 16, NULL, ZVAL_PTR_DTOR, 0);

    /* Initialize body */
    ZVAL_NULL(&intern->zv_body);
    intern->body_is_stream = 0;

    /* Initialize protocol - non-persistent (per-request allocation) */
    intern->protocol_version = zend_string_init("1.1", 3, 0);


    /* Initialize standard object */
    zend_object_std_init(&intern->std, ce);
    object_properties_init(&intern->std, ce);
    intern->std.handlers = &signalforge_response_object_handlers;

    return &intern->std;
}

static void signalforge_response_free_object(zend_object *object)
{
    signalforge_response_object *intern = signalforge_response_from_obj(object);

    /* Free strings */
    if (intern->reason_phrase) {
        zend_string_release(intern->reason_phrase);
    }
    if (intern->protocol_version) {
        zend_string_release(intern->protocol_version);
    }

    /* Free HashTable */
    if (intern->ht_headers) {
        zend_hash_destroy(intern->ht_headers);
        FREE_HASHTABLE(intern->ht_headers);
    }

    /* Release body */
    zval_ptr_dtor(&intern->zv_body);

    /* Destroy standard object properties */
    zend_object_std_dtor(&intern->std);
}

/* ============================================================================
 * PSR-7 METHOD IMPLEMENTATIONS
 * ============================================================================ */

/**
 * Create a new Response instance.
 *
 * Factory method for creating Response instances with optional status code,
 * headers, and body. If no body is provided, an empty string stream is created.
 * Headers are normalized to lowercase keys. Status defaults to 200 if not specified.
 *
 * @param int $status HTTP status code (default: 200)
 * @param array $headers Response headers as key-value pairs
 * @param mixed $body Response body (StreamInterface or string)
 * @return ResponseInterface New response instance
 */
PHP_METHOD(Signalforge_Http_Response, create)
{
    zend_long status = 200;
    zval *headers = NULL;
    zval *body = NULL;
    signalforge_response_object *intern;
    zend_string *key;
    zval *val;

    ZEND_PARSE_PARAMETERS_START(0, 3)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(status)
        Z_PARAM_ARRAY_OR_NULL(headers)
        Z_PARAM_ZVAL_OR_NULL(body)
    ZEND_PARSE_PARAMETERS_END();
    
    /* Validate status code */
    if (!signalforge_validate_status_code(status)) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Status code must be between 100 and 599", 0);
        RETURN_THROWS();
    }

    /* Create new instance */
    object_init_ex(return_value, signalforge_response_ce);
    intern = Z_SIGNALFORGE_RESPONSE_P(return_value);

    /* Set status */
    intern->status_code = status;
    intern->reason_phrase = NULL; /* Auto-generate from status */

    /* Handle parameters: headers, body */

    /* Set headers */
    if (headers && Z_TYPE_P(headers) == IS_ARRAY) {
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(headers), key, val) {
            if (key) {
                /* Skip headers with empty array values */
                if (Z_TYPE_P(val) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(val)) == 0) {
                    continue;
                }
                zend_string *normalized = signalforge_normalize_header_name(ZSTR_VAL(key), ZSTR_LEN(key));
                zval val_copy;
                ZVAL_COPY(&val_copy, val);
                zend_hash_update(intern->ht_headers, normalized, &val_copy);
                zend_string_release(normalized);
            }
        } ZEND_HASH_FOREACH_END();
    }
    
    /* Set body */
    if (body) {
        if (Z_TYPE_P(body) == IS_OBJECT && instanceof_function(Z_OBJCE_P(body), signalforge_stream_ce)) {
            ZVAL_COPY(&intern->zv_body, body);
            intern->body_is_stream = 1;
        } else if (Z_TYPE_P(body) == IS_STRING) {
            /* Create Stream from string */
            zval stream_zv, body_zv;
            ZVAL_COPY(&body_zv, body);
            zend_call_method(NULL, signalforge_stream_ce, NULL, "fromstring", sizeof("fromstring")-1, &stream_zv, 1, &body_zv, NULL);
            if (Z_TYPE(stream_zv) == IS_OBJECT) {
                ZVAL_COPY(&intern->zv_body, &stream_zv);
                intern->body_is_stream = 1;
            }
            zval_ptr_dtor(&stream_zv);
            zval_ptr_dtor(&body_zv);
        } else if (Z_TYPE_P(body) == IS_NULL) {
            /* Null body is allowed */
        } else {
            /* Invalid body type */
            zend_throw_exception(spl_ce_InvalidArgumentException,
                "Body must be a StreamInterface, string, or null", 0);
            RETURN_THROWS();
        }
    }
}
/* }}} */

/* {{{ getStatusCode() */
/**
 * Get the HTTP status code.
 *
 * @return int HTTP status code (e.g., 200, 404, 500)
 */
PHP_METHOD(Signalforge_Http_Response, getStatusCode)
{
    signalforge_response_object *intern = Z_SIGNALFORGE_RESPONSE_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();
    RETURN_LONG(intern->status_code);
}
/* }}} */

/* {{{ withStatus($code, $reasonPhrase = '') */
PHP_METHOD(Signalforge_Http_Response, withStatus)
{
    zval *code_zv;
    zend_long code;
    zend_string *reason_phrase = NULL;
    signalforge_response_object *src, *dst;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_ZVAL(code_zv)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(reason_phrase)
    ZEND_PARSE_PARAMETERS_END();

    /* Explicit type validation - reject non-integer types */
    if (Z_TYPE_P(code_zv) != IS_LONG) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Status code must be an integer", 0);
        RETURN_THROWS();
    }

    code = Z_LVAL_P(code_zv);
    
    if (!signalforge_validate_status_code(code)) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Status code must be between 100 and 599", 0);
        RETURN_THROWS();
    }
    
    src = Z_SIGNALFORGE_RESPONSE_P(ZEND_THIS);
    dst = signalforge_response_clone(src, return_value);
    
    dst->status_code = code;
    if (reason_phrase) {
        if (dst->reason_phrase) {
            zend_string_release(dst->reason_phrase);
        }
        dst->reason_phrase = zend_string_copy(reason_phrase);
    } else {
        if (dst->reason_phrase) {
            zend_string_release(dst->reason_phrase);
        }
        dst->reason_phrase = NULL; /* Auto-generate */
    }
}
/* }}} */

/* {{{ getReasonPhrase() */
PHP_METHOD(Signalforge_Http_Response, getReasonPhrase)
{
    signalforge_response_object *intern = Z_SIGNALFORGE_RESPONSE_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();
    
    if (intern->reason_phrase) {
        RETURN_STR(zend_string_copy(intern->reason_phrase));
    }
    
    const char *reason = signalforge_get_reason_phrase(intern->status_code);
    RETURN_STRING(reason);
}
/* }}} */

/* {{{ getProtocolVersion() */
PHP_METHOD(Signalforge_Http_Response, getProtocolVersion)
{
    signalforge_response_object *intern = Z_SIGNALFORGE_RESPONSE_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();
    
    if (intern->protocol_version) {
        RETURN_STR(zend_string_copy(intern->protocol_version));
    }
    RETURN_STRING("1.1");
}
/* }}} */

/* {{{ withProtocolVersion($version) */
PHP_METHOD(Signalforge_Http_Response, withProtocolVersion)
{
    zend_string *version;
    signalforge_response_object *src, *dst;
    
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(version)
    ZEND_PARSE_PARAMETERS_END();
    
    /* PSR-7 allows any protocol version string */
    
    src = Z_SIGNALFORGE_RESPONSE_P(ZEND_THIS);
    dst = signalforge_response_clone(src, return_value);
    
    if (dst->protocol_version) {
        zend_string_release(dst->protocol_version);
    }
    dst->protocol_version = zend_string_copy(version);
}
/* }}} */

/* {{{ getHeaders() */
PHP_METHOD(Signalforge_Http_Response, getHeaders)
{
    signalforge_response_object *intern = Z_SIGNALFORGE_RESPONSE_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();
    
    if (intern->ht_headers) {
        /* Convert HashTable to array, ensuring values are arrays */
        /* Return headers with lowercase keys per RFC */
        array_init(return_value);
        ZEND_HASH_FOREACH_STR_KEY_VAL(intern->ht_headers, zend_string *key, zval *val) {
            if (key) {
                /* Use lowercase key directly (already normalized) */
                zval header_array;
                if (Z_TYPE_P(val) == IS_ARRAY) {
                    ZVAL_COPY(&header_array, val);
                } else {
                    array_init(&header_array);
                    Z_TRY_ADDREF_P(val);
                    add_next_index_zval(&header_array, val);
                }
                /* Use add_assoc_zval_ex to create fresh key rather than reusing source key.
                 * This avoids reference counting issues when source HashTable is destroyed. */
                add_assoc_zval_ex(return_value, ZSTR_VAL(key), ZSTR_LEN(key), &header_array);
            }
        } ZEND_HASH_FOREACH_END();
    } else {
        array_init(return_value);
    }
}
/* }}} */

/* {{{ hasHeader($name) */
PHP_METHOD(Signalforge_Http_Response, hasHeader)
{
    zend_string *name;
    signalforge_response_object *intern = Z_SIGNALFORGE_RESPONSE_P(ZEND_THIS);
    
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
PHP_METHOD(Signalforge_Http_Response, getHeader)
{
    zend_string *name;
    signalforge_response_object *intern = Z_SIGNALFORGE_RESPONSE_P(ZEND_THIS);
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
PHP_METHOD(Signalforge_Http_Response, getHeaderLine)
{
    zend_string *name;
    signalforge_response_object *intern = Z_SIGNALFORGE_RESPONSE_P(ZEND_THIS);
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
    /* Handle edge case where header exists but is empty */
    if (str.s) {
        RETURN_STR(str.s);
    } else {
        RETURN_EMPTY_STRING();
    }
}
/* }}} */

/* {{{ withHeader($name, $value) */
PHP_METHOD(Signalforge_Http_Response, withHeader)
{
    zend_string *name;
    zval *value;
    signalforge_response_object *src, *dst;
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
                        "Invalid header value (contains CR/LF)", 0);
                    RETURN_THROWS();
                }
            }
        } ZEND_HASH_FOREACH_END();
    } else if (Z_TYPE_P(value) == IS_STRING) {
        if (!signalforge_validate_header_value(Z_STRVAL_P(value), Z_STRLEN_P(value))) {
            zend_throw_exception(spl_ce_InvalidArgumentException,
                "Invalid header value (contains CR/LF)", 0);
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
    
    src = Z_SIGNALFORGE_RESPONSE_P(ZEND_THIS);
    dst = signalforge_response_clone(src, return_value);
    
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
PHP_METHOD(Signalforge_Http_Response, withAddedHeader)
{
    zend_string *name;
    zval *value;
    signalforge_response_object *src, *dst;
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
        /* Validate each array element */
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
    
    src = Z_SIGNALFORGE_RESPONSE_P(ZEND_THIS);
    dst = signalforge_response_clone(src, return_value);
    
    /* Ensure headers HashTable exists */
    if (!dst->ht_headers) {
        ALLOC_HASHTABLE(dst->ht_headers);
        zend_hash_init(dst->ht_headers, 16, NULL, ZVAL_PTR_DTOR, 0);
    }
    
    existing_val = zend_hash_find(dst->ht_headers, normalized);
    
    if (existing_val && Z_TYPE_P(existing_val) == IS_ARRAY) {
        zval val_copy;
        ZVAL_COPY(&val_copy, value);
        add_next_index_zval(existing_val, &val_copy);
    } else {
        zval header_array;
        array_init(&header_array);
        if (existing_val) {
            zval existing_copy;
            ZVAL_COPY(&existing_copy, existing_val);
            add_next_index_zval(&header_array, &existing_copy);
        }
        zval val_copy;
        ZVAL_COPY(&val_copy, value);
        add_next_index_zval(&header_array, &val_copy);
        zend_hash_update(dst->ht_headers, normalized, &header_array);
    }
    
    zend_string_release(normalized);
}
/* }}} */

/* {{{ withoutHeader($name) */
PHP_METHOD(Signalforge_Http_Response, withoutHeader)
{
    zend_string *name;
    signalforge_response_object *src, *dst;
    
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(name)
    ZEND_PARSE_PARAMETERS_END();
    
    zend_string *normalized = signalforge_normalize_header_name(ZSTR_VAL(name), ZSTR_LEN(name));
    
    src = Z_SIGNALFORGE_RESPONSE_P(ZEND_THIS);
    
    /* If header doesn't exist, return original */
    if (!src->ht_headers || !zend_hash_exists(src->ht_headers, normalized)) {
        zend_string_release(normalized);
        RETURN_ZVAL(ZEND_THIS, 1, 0);
    }
    
    dst = signalforge_response_clone(src, return_value);
    zend_hash_del(dst->ht_headers, normalized);
    zend_string_release(normalized);
}
/* }}} */

/* {{{ getBody() */
PHP_METHOD(Signalforge_Http_Response, getBody)
{
    signalforge_response_object *intern = Z_SIGNALFORGE_RESPONSE_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();
    
    if (Z_TYPE(intern->zv_body) == IS_OBJECT) {
        RETURN_ZVAL(&intern->zv_body, 1, 0);
    }
    
    /* Create empty stream if no body */
    zval stream_zv, empty_zv;
    ZVAL_EMPTY_STRING(&empty_zv);
    zend_call_method(NULL, signalforge_stream_ce, NULL, "fromstring", sizeof("fromstring")-1, &stream_zv, 1, &empty_zv, NULL);
    
    if (Z_TYPE(stream_zv) == IS_OBJECT) {
        RETURN_ZVAL(&stream_zv, 0, 0);
    }
    
    /* Fallback */
    object_init_ex(return_value, signalforge_stream_ce);
}
/* }}} */

/* {{{ withBody(StreamInterface $body) */
PHP_METHOD(Signalforge_Http_Response, withBody)
{
    zval *body;
    signalforge_response_object *src, *dst;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(body)
    ZEND_PARSE_PARAMETERS_END();

    /* Validate that body is a StreamInterface */
    if (Z_TYPE_P(body) != IS_OBJECT || !instanceof_function(Z_OBJCE_P(body), signalforge_stream_ce)) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Body must be a StreamInterface", 0);
        RETURN_THROWS();
    }

    src = Z_SIGNALFORGE_RESPONSE_P(ZEND_THIS);
    dst = signalforge_response_clone(src, return_value);

    /* Store body */
    if (!Z_ISUNDEF(dst->zv_body)) zval_ptr_dtor(&dst->zv_body);
    ZVAL_COPY(&dst->zv_body, body);
    dst->body_is_stream = 1;
}
/* }}} */

/* ============================================================================
 * FACTORY METHODS
 * ============================================================================ */

/* {{{ json($data, $status = 200) */
PHP_METHOD(Signalforge_Http_Response, json)
{
    zval *data;
    zend_long status = 200;
    signalforge_response_object *intern;
    zend_string *json_str;
    zval stream_zv;
    
    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_ZVAL(data)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(status)
    ZEND_PARSE_PARAMETERS_END();
    
    if (!signalforge_validate_status_code(status)) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Status code must be between 100 and 599", 0);
        RETURN_THROWS();
    }
    
    /* Encode JSON */
    smart_str json_buf = {0};
    zend_result json_result = php_json_encode(&json_buf, data, PHP_JSON_UNESCAPED_SLASHES | PHP_JSON_UNESCAPED_UNICODE);
    if (json_result != SUCCESS || !json_buf.s) {
        smart_str_free(&json_buf);
        zend_throw_exception(spl_ce_RuntimeException,
            "Failed to encode JSON", 0);
        RETURN_THROWS();
    }
    json_str = json_buf.s;
    
    /* Create Stream from JSON string */
    zval json_zv;
    ZVAL_STR(&json_zv, json_str);
    zend_call_method(NULL, signalforge_stream_ce, NULL, "fromstring", sizeof("fromstring")-1, &stream_zv, 1, &json_zv, NULL);
    
    if (Z_TYPE(stream_zv) != IS_OBJECT) {
        zend_string_release(json_str);
        zend_throw_exception(spl_ce_RuntimeException,
            "Failed to create stream", 0);
        RETURN_THROWS();
    }
    
    /* Create Response (ht_headers and protocol_version initialized by create_object) */
    object_init_ex(return_value, signalforge_response_ce);
    intern = Z_SIGNALFORGE_RESPONSE_P(return_value);

    intern->status_code = status;
    intern->reason_phrase = NULL;

    /* Set Content-Type header (ht_headers already initialized by create_object) */
    zval content_type_val;
    ZVAL_STRING(&content_type_val, "application/json");
    zend_string *ct_key = zend_string_init("content-type", sizeof("content-type")-1, 0);
    zend_hash_add(intern->ht_headers, ct_key, &content_type_val);
    zend_string_release(ct_key);

    /* Set body */
    ZVAL_COPY(&intern->zv_body, &stream_zv);
    intern->body_is_stream = 1;
    /* protocol_version already set to "1.1" by create_object */

    zval_ptr_dtor(&stream_zv);
}
/* }}} */

/* {{{ text($text, $status = 200) */
PHP_METHOD(Signalforge_Http_Response, text)
{
    zend_string *text;
    zend_long status = 200;
    signalforge_response_object *intern;
    zval stream_zv;
    
    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(text)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(status)
    ZEND_PARSE_PARAMETERS_END();
    
    if (!signalforge_validate_status_code(status)) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Status code must be between 100 and 599", 0);
        RETURN_THROWS();
    }
    
    /* Create Stream from text */
    zval text_zv;
    ZVAL_STR(&text_zv, text);
    zend_call_method(NULL, signalforge_stream_ce, NULL, "fromstring", sizeof("fromstring")-1, &stream_zv, 1, &text_zv, NULL);
    
    if (Z_TYPE(stream_zv) != IS_OBJECT) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Failed to create stream", 0);
        RETURN_THROWS();
    }
    
    /* Create Response (ht_headers and protocol_version initialized by create_object) */
    object_init_ex(return_value, signalforge_response_ce);
    intern = Z_SIGNALFORGE_RESPONSE_P(return_value);

    intern->status_code = status;
    intern->reason_phrase = NULL;

    /* Set Content-Type header (ht_headers already initialized by create_object) */
    zval content_type_val;
    ZVAL_STRING(&content_type_val, "text/plain");
    zend_string *ct_key = zend_string_init("content-type", sizeof("content-type")-1, 0);
    zend_hash_add(intern->ht_headers, ct_key, &content_type_val);
    zend_string_release(ct_key);

    /* Set body */
    ZVAL_COPY(&intern->zv_body, &stream_zv);
    intern->body_is_stream = 1;
    /* protocol_version already set to "1.1" by create_object */

    zval_ptr_dtor(&stream_zv);
}
/* }}} */

/* {{{ html($html, $status = 200) */
PHP_METHOD(Signalforge_Http_Response, html)
{
    zend_string *html;
    zend_long status = 200;
    signalforge_response_object *intern;
    zval stream_zv;
    
    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(html)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(status)
    ZEND_PARSE_PARAMETERS_END();
    
    if (!signalforge_validate_status_code(status)) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Status code must be between 100 and 599", 0);
        RETURN_THROWS();
    }
    
    /* Create Stream from HTML */
    zval html_zv;
    ZVAL_STR(&html_zv, html);
    zend_call_method(NULL, signalforge_stream_ce, NULL, "fromstring", sizeof("fromstring")-1, &stream_zv, 1, &html_zv, NULL);
    
    if (Z_TYPE(stream_zv) != IS_OBJECT) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Failed to create stream", 0);
        RETURN_THROWS();
    }
    
    /* Create Response (ht_headers and protocol_version initialized by create_object) */
    object_init_ex(return_value, signalforge_response_ce);
    intern = Z_SIGNALFORGE_RESPONSE_P(return_value);

    intern->status_code = status;
    intern->reason_phrase = NULL;

    /* Set Content-Type header (ht_headers already initialized by create_object) */
    zval content_type_val;
    ZVAL_STRING(&content_type_val, "text/html");
    zend_string *ct_key = zend_string_init("content-type", sizeof("content-type")-1, 0);
    zend_hash_add(intern->ht_headers, ct_key, &content_type_val);
    zend_string_release(ct_key);

    /* Set body */
    ZVAL_COPY(&intern->zv_body, &stream_zv);
    intern->body_is_stream = 1;
    /* protocol_version already set to "1.1" by create_object */

    zval_ptr_dtor(&stream_zv);
}
/* }}} */

/* {{{ redirect($url, $status = 302) */
PHP_METHOD(Signalforge_Http_Response, redirect)
{
    zend_string *url;
    zend_long status = 302;
    signalforge_response_object *intern;
    
    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(url)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(status)
    ZEND_PARSE_PARAMETERS_END();
    
    /* Validate redirect status code */
    if (status < 300 || status > 399) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Redirect status code must be between 300 and 399", 0);
        RETURN_THROWS();
    }
    
    /* Create Response (ht_headers and protocol_version initialized by create_object) */
    object_init_ex(return_value, signalforge_response_ce);
    intern = Z_SIGNALFORGE_RESPONSE_P(return_value);

    intern->status_code = status;
    intern->reason_phrase = NULL;

    /* Set Location header (ht_headers already initialized by create_object) */
    zval location_val;
    ZVAL_STR(&location_val, zend_string_copy(url));
    zend_string *loc_key = zend_string_init("location", sizeof("location")-1, 0);
    zend_hash_add(intern->ht_headers, loc_key, &location_val);
    zend_string_release(loc_key);

    /* No body for redirects */
    ZVAL_NULL(&intern->zv_body);
    intern->body_is_stream = 0;
    /* protocol_version already set to "1.1" by create_object */
}
/* }}} */

/* ============================================================================
 * HELPER FUNCTIONS FOR SENDING (defined before methods that use them)
 * ============================================================================ */

static void signalforge_response_send_headers(signalforge_response_object *intern)
{
    zend_string *key;
    zval *val;
    sapi_header_line header_line;
    
    /* Set HTTP response code via SAPI */
    SG(sapi_headers).http_response_code = intern->status_code;
    
    /* Send headers via SAPI - in CLI mode these won't be output to buffer */
    {
        /* Send status line via SAPI header */
        const char *reason = signalforge_get_reason_phrase(intern->status_code);
        smart_str status_line = {0};
        smart_str_append_printf(&status_line, "HTTP/%s %ld %s",
            intern->protocol_version ? ZSTR_VAL(intern->protocol_version) : "1.1",
            intern->status_code, reason);
        smart_str_0(&status_line);
        header_line.line = ZSTR_VAL(status_line.s);
        header_line.line_len = ZSTR_LEN(status_line.s);
        sapi_header_op(SAPI_HEADER_SET_STATUS, &header_line);
        smart_str_free(&status_line);
        
        /* Send headers via SAPI */
        if (intern->ht_headers) {
            ZEND_HASH_FOREACH_STR_KEY_VAL(intern->ht_headers, key, val) {
                if (!key) continue;
                
                if (Z_TYPE_P(val) == IS_ARRAY) {
                    zval *item;
                    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(val), item) {
                        if (Z_TYPE_P(item) == IS_STRING) {
                            smart_str header_str = {0};
                            const char *name = ZSTR_VAL(key);
                            size_t name_len = ZSTR_LEN(key);
                            
                            smart_str_appendl(&header_str, name, name_len);
                            smart_str_appends(&header_str, ": ");
                            smart_str_appendl(&header_str, Z_STRVAL_P(item), Z_STRLEN_P(item));
                            smart_str_0(&header_str);
                            
                            header_line.line = ZSTR_VAL(header_str.s);
                            header_line.line_len = ZSTR_LEN(header_str.s);
                            sapi_header_op(SAPI_HEADER_ADD, &header_line);
                            smart_str_free(&header_str);
                        }
                    } ZEND_HASH_FOREACH_END();
                } else if (Z_TYPE_P(val) == IS_STRING) {
                    smart_str header_str = {0};
                    const char *name = ZSTR_VAL(key);
                    size_t name_len = ZSTR_LEN(key);
                    
                    smart_str_appendl(&header_str, name, name_len);
                    smart_str_appends(&header_str, ": ");
                    smart_str_appendl(&header_str, Z_STRVAL_P(val), Z_STRLEN_P(val));
                    smart_str_0(&header_str);
                    
                    header_line.line = ZSTR_VAL(header_str.s);
                    header_line.line_len = ZSTR_LEN(header_str.s);
                    sapi_header_op(SAPI_HEADER_REPLACE, &header_line);
                    smart_str_free(&header_str);
                }
            } ZEND_HASH_FOREACH_END();
        }
    }
}

static void signalforge_response_send_body(signalforge_response_object *intern)
{
    if (Z_TYPE(intern->zv_body) == IS_OBJECT && intern->body_is_stream) {
        zval contents_zv;
        zend_call_method(Z_OBJ(intern->zv_body), Z_OBJCE(intern->zv_body), NULL, "getcontents", sizeof("getcontents")-1, &contents_zv, 0, NULL, NULL);
        if (Z_TYPE(contents_zv) == IS_STRING) {
            php_write(Z_STRVAL(contents_zv), Z_STRLEN(contents_zv));
        }
        zval_ptr_dtor(&contents_zv);
    }
}

/* ============================================================================
 * OUTPUT METHODS
 * ============================================================================ */

/* {{{ send() */
PHP_METHOD(Signalforge_Http_Response, send)
{
    signalforge_response_object *intern = Z_SIGNALFORGE_RESPONSE_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();
    
    /* Send headers */
    signalforge_response_send_headers(intern);
    
    /* Send body */
    signalforge_response_send_body(intern);
}
/* }}} */

/* {{{ sendHeaders() */
PHP_METHOD(Signalforge_Http_Response, sendHeaders)
{
    signalforge_response_object *intern = Z_SIGNALFORGE_RESPONSE_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();
    
    signalforge_response_send_headers(intern);
}
/* }}} */

/* {{{ sendBody() */
PHP_METHOD(Signalforge_Http_Response, sendBody)
{
    signalforge_response_object *intern = Z_SIGNALFORGE_RESPONSE_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();
    
    signalforge_response_send_body(intern);
}
/* }}} */

/* {{{ __toString() */
PHP_METHOD(Signalforge_Http_Response, __toString)
{
    signalforge_response_object *intern = Z_SIGNALFORGE_RESPONSE_P(ZEND_THIS);
    smart_str str = {0};
    zend_string *status_line;
    zend_string *headers_str;
    zend_string *body_str;
    
    ZEND_PARSE_PARAMETERS_NONE();
    
    /* Status line */
    const char *reason = signalforge_get_reason_phrase(intern->status_code);
    smart_str_append_printf(&str, "HTTP/%s %ld %s\r\n",
        intern->protocol_version ? ZSTR_VAL(intern->protocol_version) : "1.1",
        intern->status_code, reason);
    
    /* Headers */
    headers_str = signalforge_serialize_headers(intern->ht_headers);
    smart_str_appendl(&str, ZSTR_VAL(headers_str), ZSTR_LEN(headers_str));
    zend_string_release(headers_str);
    
    /* Blank line */
    smart_str_appends(&str, "\r\n");
    
    /* Body */
    if (Z_TYPE(intern->zv_body) == IS_OBJECT && intern->body_is_stream) {
        zval contents_zv;
        zend_call_method(Z_OBJ(intern->zv_body), Z_OBJCE(intern->zv_body), NULL, "getcontents", sizeof("getcontents")-1, &contents_zv, 0, NULL, NULL);
        if (Z_TYPE(contents_zv) == IS_STRING) {
            smart_str_appendl(&str, Z_STRVAL(contents_zv), Z_STRLEN(contents_zv));
        }
        zval_ptr_dtor(&contents_zv);
    }
    
    smart_str_0(&str);
    RETURN_STR(str.s);
}
/* }}} */

/* ============================================================================
 * ARGINFO DEFINITIONS
 * ============================================================================ */

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_response_create, 0, 0, Signalforge\\NativeHttp\\Response, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, status, IS_LONG, 0, "200")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, headers, IS_ARRAY, 1, "[]")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, body, IS_MIXED, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_response_getStatusCode, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_response_withStatus, 0, 1, Signalforge\\NativeHttp\\Response, 0)
    ZEND_ARG_TYPE_INFO(0, code, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, reasonPhrase, IS_STRING, 1, "")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_response_getReasonPhrase, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_response_getProtocolVersion, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_response_withProtocolVersion, 0, 1, Signalforge\\NativeHttp\\Response, 0)
    ZEND_ARG_TYPE_INFO(0, version, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_response_getHeaders, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_response_hasHeader, 0, 1, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_response_getHeader, 0, 1, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_response_getHeaderLine, 0, 1, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_response_withHeader, 0, 2, Signalforge\\NativeHttp\\Response, 0)
    ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_response_withAddedHeader, 0, 2, Signalforge\\NativeHttp\\Response, 0)
    ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_response_withoutHeader, 0, 1, Signalforge\\NativeHttp\\Response, 0)
    ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_response_getBody, 0, 0, Psr\\Http\\Message\\StreamInterface, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_response_withBody, 0, 1, Signalforge\\NativeHttp\\Response, 0)
    ZEND_ARG_OBJ_INFO(0, body, Psr\\Http\\Message\\StreamInterface, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_response_json, 0, 1, Signalforge\\NativeHttp\\Response, 0)
    ZEND_ARG_TYPE_INFO(0, data, IS_MIXED, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, status, IS_LONG, 0, "200")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_response_text, 0, 1, Signalforge\\NativeHttp\\Response, 0)
    ZEND_ARG_TYPE_INFO(0, text, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, status, IS_LONG, 0, "200")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_response_html, 0, 1, Signalforge\\NativeHttp\\Response, 0)
    ZEND_ARG_TYPE_INFO(0, html, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, status, IS_LONG, 0, "200")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_response_redirect, 0, 1, Signalforge\\NativeHttp\\Response, 0)
    ZEND_ARG_TYPE_INFO(0, url, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, status, IS_LONG, 0, "302")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_response_send, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_response_sendHeaders, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_response_sendBody, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_response___toString, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

/* ============================================================================
 * METHOD REGISTRATION
 * ============================================================================ */

static const zend_function_entry signalforge_response_methods[] = {
    /* Factory */
    PHP_ME(Signalforge_Http_Response, create, arginfo_response_create, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    
    /* ResponseInterface */
    PHP_ME(Signalforge_Http_Response, getStatusCode, arginfo_response_getStatusCode, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Response, withStatus, arginfo_response_withStatus, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Response, getReasonPhrase, arginfo_response_getReasonPhrase, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Response, getProtocolVersion, arginfo_response_getProtocolVersion, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Response, withProtocolVersion, arginfo_response_withProtocolVersion, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Response, getHeaders, arginfo_response_getHeaders, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Response, hasHeader, arginfo_response_hasHeader, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Response, getHeader, arginfo_response_getHeader, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Response, getHeaderLine, arginfo_response_getHeaderLine, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Response, withHeader, arginfo_response_withHeader, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Response, withAddedHeader, arginfo_response_withAddedHeader, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Response, withoutHeader, arginfo_response_withoutHeader, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Response, getBody, arginfo_response_getBody, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Response, withBody, arginfo_response_withBody, ZEND_ACC_PUBLIC)
    
    /* Factory methods */
    PHP_ME(Signalforge_Http_Response, json, arginfo_response_json, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(Signalforge_Http_Response, text, arginfo_response_text, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(Signalforge_Http_Response, html, arginfo_response_html, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(Signalforge_Http_Response, redirect, arginfo_response_redirect, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    
    /* Output methods */
    PHP_ME(Signalforge_Http_Response, send, arginfo_response_send, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Response, sendHeaders, arginfo_response_sendHeaders, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Response, sendBody, arginfo_response_sendBody, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Response, __toString, arginfo_response___toString, ZEND_ACC_PUBLIC)
    
    PHP_FE_END
};

void signalforge_response_register_class(void)
{
    zend_class_entry ce;

    INIT_NS_CLASS_ENTRY(ce, "Signalforge\\NativeHttp", "Response", signalforge_response_methods);
    signalforge_response_ce = zend_register_internal_class(&ce);
    signalforge_response_ce->ce_flags |= ZEND_ACC_FINAL;
    signalforge_response_ce->create_object = signalforge_response_create_object;

    /* Copy object handlers */
    memcpy(&signalforge_response_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    signalforge_response_object_handlers.offset = XtOffsetOf(signalforge_response_object, std);
    signalforge_response_object_handlers.free_obj = signalforge_response_free_object;

}

