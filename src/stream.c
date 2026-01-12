/*
 * stream.c
 *
 * Signalforge HTTP Stream Class Implementation
 *
 * This file implements the Signalforge\Http\Stream class, providing
 * PSR-7 StreamInterface with zero-copy optimizations.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_signalforge_http.h"
#include "psr7_interfaces.h"
#include "stream.h"
#include "zend_smart_str.h"
#include <sys/stat.h>

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

zend_class_entry *signalforge_stream_ce = NULL;
static zend_object_handlers signalforge_stream_object_handlers;

/* SPL Exception classes (available in PHP 8.3) */
extern PHPAPI zend_class_entry *spl_ce_InvalidArgumentException;
extern PHPAPI zend_class_entry *spl_ce_RuntimeException;

/* ============================================================================
 * OBJECT HANDLERS
 * ============================================================================ */

static zend_object *signalforge_stream_create_object(zend_class_entry *ce)
{
    signalforge_stream_object *intern;

    intern = zend_object_alloc(sizeof(signalforge_stream_object), ce);

    /* Initialize all pointers to NULL */
    ZVAL_UNDEF(&intern->zv_resource);
    intern->string_data = NULL;
    intern->ht_metadata = NULL;

    /* Initialize values */
    intern->position = 0;
    intern->size = 0;
    intern->readable = 0;
    intern->writable = 0;
    intern->seekable = 0;
    intern->metadata_loaded = 0;

    /* Initialize standard object */
    zend_object_std_init(&intern->std, ce);
    object_properties_init(&intern->std, ce);
    intern->std.handlers = &signalforge_stream_object_handlers;

    return &intern->std;
}

static void signalforge_stream_free_object(zend_object *object)
{
    signalforge_stream_object *intern = signalforge_stream_from_obj(object);

    /* Release resource */
    zval_ptr_dtor(&intern->zv_resource);

    /* Release string data (zero-copy reference) */
    if (intern->string_data) {
        zend_string_release(intern->string_data);
    }

    /* Free metadata HashTable */
    if (intern->ht_metadata) {
        zend_hash_destroy(intern->ht_metadata);
        FREE_HASHTABLE(intern->ht_metadata);
    }

    /* Destroy standard object properties */
    zend_object_std_dtor(&intern->std);
}

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

/* Inlined for maximum performance */
#define signalforge_get_php_stream(intern) \
    ((intern)->string_data ? NULL : \
     (Z_TYPE((intern)->zv_resource) == IS_RESOURCE ? \
      ({ php_stream *stream = NULL; \
         if (zend_fetch_resource2(Z_RES((intern)->zv_resource), "stream", php_file_le_stream(), php_file_le_pstream()) != NULL) { \
           php_stream_from_zval_no_verify(stream, &(intern)->zv_resource); \
         } \
         stream; }) : \
      NULL))

/* Inlined for maximum performance */
#define signalforge_stream_is_closed(intern) \
    (!((intern)->readable) && !((intern)->writable) && !((intern)->seekable) && \
     Z_ISUNDEF((intern)->zv_resource) && !((intern)->string_data))

#define signalforge_stream_is_detached(intern) \
    (Z_ISUNDEF((intern)->zv_resource) && !((intern)->string_data))

/*
 * Load stream metadata lazily into the hashtable cache.
 *
 * For resource-based streams, extracts metadata via stream_get_meta_data().
 * For string-based streams, synthesizes compatible metadata (size, mode).
 * Called on first getMetadata() access to avoid upfront cost.
 *
 * @param intern Stream object to load metadata for
 */
void signalforge_load_metadata(signalforge_stream_object *intern)
{
    if (intern->metadata_loaded) {
        return;
    }

    intern->metadata_loaded = 1;

    if (!intern->ht_metadata) {
        ALLOC_HASHTABLE(intern->ht_metadata);
        zend_hash_init(intern->ht_metadata, 8, NULL, ZVAL_PTR_DTOR, 0);
    }

    /* String-based stream metadata */
    if (intern->string_data) {
        zval size_zv, mode_zv;
        ZVAL_LONG(&size_zv, ZSTR_LEN(intern->string_data));
        /* Use interned strings for common metadata keys to avoid allocations */
        zend_string *size_key = zend_string_init_interned("size", sizeof("size")-1, 1);
        zend_hash_add(intern->ht_metadata, size_key, &size_zv);
        zend_string_release(size_key);

        /* Add mode for compatibility with tests */
        ZVAL_STRING(&mode_zv, "rb");
        zend_string *mode_key = zend_string_init_interned("mode", sizeof("mode")-1, 1);
        zend_hash_add(intern->ht_metadata, mode_key, &mode_zv);
        zend_string_release(mode_key);

        return;
    }

    /* Resource-based stream metadata */
    if (Z_TYPE(intern->zv_resource) == IS_RESOURCE) {
        /* Call PHP's stream_get_meta_data() function */
        zval function_name, retval, params[1];
        int result;

        ZVAL_STRING(&function_name, "stream_get_meta_data");
        ZVAL_COPY(&params[0], &intern->zv_resource);

        result = call_user_function(CG(function_table), NULL, &function_name, &retval, 1, params);

        if (result == SUCCESS && Z_TYPE(retval) == IS_ARRAY) {
            /* Copy all metadata from PHP's function */
            zend_hash_copy(intern->ht_metadata, Z_ARRVAL(retval), (copy_ctor_func_t)zval_add_ref);
        } else {
            /* Fallback: add basic metadata */
            zval mode_zv;
            ZVAL_STRING(&mode_zv, "r+");
            zend_string *mode_key = zend_string_init_interned("mode", sizeof("mode")-1, 1);
            zend_hash_update(intern->ht_metadata, mode_key, &mode_zv);
            zend_string_release(mode_key);
        }

        zval_ptr_dtor(&retval);
        zval_ptr_dtor(&function_name);
        zval_ptr_dtor(&params[0]);
    }
}

/* ============================================================================
 * PSR-7 METHOD IMPLEMENTATIONS
 * ============================================================================ */

/**
 * Read up to $length bytes from the stream.
 *
 * Advances the stream position by the number of bytes read. If fewer bytes
 * are available than requested, returns all remaining data. If the stream
 * is not readable, throws a RuntimeException.
 *
 * @param int $length Maximum number of bytes to read
 * @return string Data read from stream
 * @throws RuntimeException When stream is not readable
 */
PHP_METHOD(Signalforge_Http_Stream, read)
{
    signalforge_stream_object *intern;
    zend_long length;
    zend_string *result;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(length)
    ZEND_PARSE_PARAMETERS_END();

    if (length < 0) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Length must be non-negative", 0);
        RETURN_THROWS();
    }

    intern = Z_SIGNALFORGE_STREAM_P(ZEND_THIS);

    if (signalforge_stream_is_closed(intern) || signalforge_stream_is_detached(intern)) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Stream is closed or detached", 0);
        RETURN_THROWS();
    }

    if (!intern->readable) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Stream is not readable", 0);
        RETURN_THROWS();
    }

    /* String-based stream */
    if (intern->string_data) {
        size_t available = ZSTR_LEN(intern->string_data) - intern->position;
        size_t read_len = (size_t)length < available ? (size_t)length : available;

        if (read_len == 0) {
            RETURN_EMPTY_STRING();
        }

        result = zend_string_init(ZSTR_VAL(intern->string_data) + intern->position, read_len, 0);
        intern->position += read_len;
        RETURN_STR(result);
    }

    /* Resource-based stream */
    php_stream *stream = signalforge_get_php_stream(intern);
    if (!stream) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Stream resource is invalid", 0);
        RETURN_THROWS();
    }

    if (length == 0) {
        RETURN_EMPTY_STRING();
    }
    
    result = zend_string_alloc(length, 0);
    ssize_t read_bytes = php_stream_read(stream, ZSTR_VAL(result), (size_t)length);
    
    if (read_bytes > 0) {
        ZSTR_LEN(result) = read_bytes;
        ZSTR_VAL(result)[read_bytes] = '\0';
        intern->position = php_stream_tell(stream);
        RETURN_STR(result);
    }
    
    zend_string_release(result);
    RETURN_EMPTY_STRING();
}
/* }}} */

/* {{{ getContents() */
/**
 * Get remaining contents of stream as string.
 *
 * Reads all remaining data from current position to end of stream.
 * Uses optimized php_stream_copy_to_mem for file streams when possible.
 * If the stream is not readable, throws a RuntimeException.
 *
 * @return string Remaining stream contents
 * @throws RuntimeException When stream is not readable
 */
PHP_METHOD(Signalforge_Http_Stream, getContents)
{
    signalforge_stream_object *intern;
    zend_string *result;

    ZEND_PARSE_PARAMETERS_NONE();

    intern = Z_SIGNALFORGE_STREAM_P(ZEND_THIS);

    if (signalforge_stream_is_closed(intern) || signalforge_stream_is_detached(intern)) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Stream is closed or detached", 0);
        RETURN_THROWS();
    }

    if (!intern->readable) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Stream is not readable", 0);
        RETURN_THROWS();
    }

    /* String-based stream */
    if (intern->string_data) {
        size_t available = ZSTR_LEN(intern->string_data) - intern->position;
        if (available == 0) {
            RETURN_EMPTY_STRING();
        }

        /* Zero-copy optimization: return original string (zend_string_copy increments refcount) */
        if (intern->position == 0 && available == ZSTR_LEN(intern->string_data)) {
            intern->position = ZSTR_LEN(intern->string_data);
            RETURN_STR(zend_string_copy(intern->string_data));
        }

        /* Otherwise, create substring */
        result = zend_string_init(ZSTR_VAL(intern->string_data) + intern->position, available, 0);
        intern->position = ZSTR_LEN(intern->string_data);
        RETURN_STR(result);
    }

    /* Resource-based stream */
    php_stream *stream = signalforge_get_php_stream(intern);
    if (!stream) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Stream resource is invalid", 0);
        RETURN_THROWS();
    }

    /* Use php_stream_copy_to_mem for better performance than manual buffering */
    zend_string *contents = php_stream_copy_to_mem(stream, PHP_STREAM_COPY_ALL, 0);
    if (contents) {
        /* Update position to end of stream */
        intern->position = php_stream_tell(stream);
        RETURN_STR(contents);
    }

    /* Stream copy failed, try manual reading as fallback */
    smart_str buf = {0};
    char read_buf[65536]; /* Use larger buffer for better performance */
    ssize_t read_len;

    while ((read_len = php_stream_read(stream, read_buf, sizeof(read_buf))) > 0) {
        smart_str_appendl(&buf, read_buf, read_len);
    }

    smart_str_0(&buf);
    intern->position = php_stream_tell(stream);

    if (buf.s && ZSTR_LEN(buf.s) > 0) {
        RETURN_STR(buf.s);
    }

    if (buf.s) {
        zend_string_release(buf.s);
    }
    RETURN_EMPTY_STRING();
}
/* }}} */

/* {{{ eof() */
PHP_METHOD(Signalforge_Http_Stream, eof)
{
    signalforge_stream_object *intern;

    ZEND_PARSE_PARAMETERS_NONE();

    intern = Z_SIGNALFORGE_STREAM_P(ZEND_THIS);

    if (signalforge_stream_is_closed(intern) || signalforge_stream_is_detached(intern)) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Stream is closed or detached", 0);
        RETURN_THROWS();
    }

    /* String-based stream */
    if (intern->string_data) {
        RETURN_BOOL(intern->position >= ZSTR_LEN(intern->string_data));
    }

    /* Resource-based stream */
    php_stream *stream = signalforge_get_php_stream(intern);
    if (!stream) {
        RETURN_TRUE; /* Invalid stream = EOF */
    }

    RETURN_BOOL(php_stream_eof(stream));
}
/* }}} */

/* {{{ write($string) */
/**
 * Write data to the stream.
 *
 * Writes the given data to the stream at current position and advances
 * the position by the number of bytes written. If the stream is not
 * writable, throws a RuntimeException.
 *
 * @param string $string Data to write
 * @return int Number of bytes written
 * @throws RuntimeException When stream is not writable
 */
PHP_METHOD(Signalforge_Http_Stream, write)
{
    signalforge_stream_object *intern;
    zend_string *data;
    size_t written;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(data)
    ZEND_PARSE_PARAMETERS_END();

    intern = Z_SIGNALFORGE_STREAM_P(ZEND_THIS);

    if (signalforge_stream_is_closed(intern) || signalforge_stream_is_detached(intern)) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Stream is closed or detached", 0);
        RETURN_THROWS();
    }

    if (!intern->writable) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Stream is not writable", 0);
        RETURN_THROWS();
    }

    /* String-based streams are not writable */
    if (intern->string_data) {
        zend_throw_exception(spl_ce_RuntimeException,
            "String-based streams are not writable", 0);
        RETURN_THROWS();
    }

    /* Resource-based stream */
    php_stream *stream = signalforge_get_php_stream(intern);
    if (!stream) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Stream resource is invalid", 0);
        RETURN_THROWS();
    }

    written = php_stream_write(stream, ZSTR_VAL(data), ZSTR_LEN(data));
    intern->position = php_stream_tell(stream);
    
    RETURN_LONG(written);
}
/* }}} */

/* {{{ seek($offset, $whence = SEEK_SET) */
/**
 * Seek to a position in the stream.
 *
 * Changes the stream position according to the given offset and whence.
 * SEEK_SET: absolute position, SEEK_CUR: relative to current, SEEK_END: relative to end.
 * If the stream is not seekable, throws a RuntimeException.
 *
 * @param int $offset Position offset
 * @param int $whence SEEK_SET, SEEK_CUR, or SEEK_END
 * @throws RuntimeException When stream is not seekable
 */
PHP_METHOD(Signalforge_Http_Stream, seek)
{
    signalforge_stream_object *intern;
    zend_long offset;
    zend_long whence = SEEK_SET;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_LONG(offset)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(whence)
    ZEND_PARSE_PARAMETERS_END();

    intern = Z_SIGNALFORGE_STREAM_P(ZEND_THIS);

    /* Validate whence parameter */
    if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Invalid whence value", 0);
        RETURN_THROWS();
    }

    if (signalforge_stream_is_closed(intern) || signalforge_stream_is_detached(intern)) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Stream is closed or detached", 0);
        RETURN_THROWS();
    }

    if (!intern->seekable) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Stream is not seekable", 0);
        RETURN_THROWS();
    }

    /* String-based stream */
    if (intern->string_data) {
        zend_long new_position;
        switch (whence) {
            case SEEK_SET:
                new_position = offset;
                break;
            case SEEK_CUR:
                /* Bounds check to prevent integer overflow/underflow */
                if (offset > 0 && intern->position > ZEND_LONG_MAX - offset) {
                    zend_throw_exception(spl_ce_RuntimeException,
                        "Seek position overflow", 0);
                    RETURN_THROWS();
                }
                if (offset < 0 && intern->position < ZEND_LONG_MIN - offset) {
                    zend_throw_exception(spl_ce_RuntimeException,
                        "Seek position underflow", 0);
                    RETURN_THROWS();
                }
                new_position = intern->position + offset;
                break;
            case SEEK_END:
                /* Bounds check to prevent integer overflow */
                if (offset > 0 && (zend_long)ZSTR_LEN(intern->string_data) > ZEND_LONG_MAX - offset) {
                    zend_throw_exception(spl_ce_RuntimeException,
                        "Seek position overflow", 0);
                    RETURN_THROWS();
                }
                new_position = ZSTR_LEN(intern->string_data) + offset;
                break;
            default:
                zend_throw_exception(spl_ce_InvalidArgumentException,
                    "Invalid whence value", 0);
                RETURN_THROWS();
        }

        /* Allow seeking beyond end (test expects this behavior) */
        if (new_position < 0) {
            zend_throw_exception(spl_ce_RuntimeException,
                "Seek position out of range", 0);
            RETURN_THROWS();
        }

        intern->position = new_position;
        return;
    }

    /* Resource-based stream */
    php_stream *stream = signalforge_get_php_stream(intern);
    if (!stream) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Stream resource is invalid", 0);
        RETURN_THROWS();
    }

    if (php_stream_seek(stream, offset, (int)whence) != 0) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Seek failed", 0);
        RETURN_THROWS();
    }
    
    intern->position = php_stream_tell(stream);
}
/* }}} */

/* {{{ tell() */
PHP_METHOD(Signalforge_Http_Stream, tell)
{
    signalforge_stream_object *intern;

    ZEND_PARSE_PARAMETERS_NONE();

    intern = Z_SIGNALFORGE_STREAM_P(ZEND_THIS);

    if (signalforge_stream_is_closed(intern) || signalforge_stream_is_detached(intern)) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Stream is closed or detached", 0);
        RETURN_THROWS();
    }

    /* String-based stream */
    if (intern->string_data) {
        RETURN_LONG(intern->position);
    }

    /* Resource-based stream */
    php_stream *stream = signalforge_get_php_stream(intern);
    if (!stream) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Stream resource is invalid", 0);
        RETURN_THROWS();
    }

    RETURN_LONG(php_stream_tell(stream));
}
/* }}} */

/* {{{ rewind() */
PHP_METHOD(Signalforge_Http_Stream, rewind)
{
    signalforge_stream_object *intern;

    ZEND_PARSE_PARAMETERS_NONE();

    intern = Z_SIGNALFORGE_STREAM_P(ZEND_THIS);

    if (signalforge_stream_is_closed(intern) || signalforge_stream_is_detached(intern)) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Stream is closed or detached", 0);
        RETURN_THROWS();
    }

    if (!intern->seekable) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Stream is not seekable", 0);
        RETURN_THROWS();
    }

    /* String-based stream */
    if (intern->string_data) {
        intern->position = 0;
        return;
    }

    /* Resource-based stream */
    php_stream *stream = signalforge_get_php_stream(intern);
    if (stream) {
        php_stream_rewind(stream);
        intern->position = php_stream_tell(stream);
    }
}
/* }}} */

/* {{{ isReadable() */
PHP_METHOD(Signalforge_Http_Stream, isReadable)
{
    signalforge_stream_object *intern = Z_SIGNALFORGE_STREAM_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();

    if (signalforge_stream_is_closed(intern) || signalforge_stream_is_detached(intern)) {
        RETURN_FALSE;
    }

    RETURN_BOOL(intern->readable);
}
/* }}} */

/* {{{ isWritable() */
PHP_METHOD(Signalforge_Http_Stream, isWritable)
{
    signalforge_stream_object *    intern = Z_SIGNALFORGE_STREAM_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();

    if (signalforge_stream_is_closed(intern) || signalforge_stream_is_detached(intern)) {
        RETURN_FALSE;
    }

    /* String-based streams are not writable */
    if (intern->string_data) {
        RETURN_FALSE;
    }

    /* For resource-based streams, return the cached writable flag */
    /* This is set correctly during stream creation (fromFile/fromResource) */
    RETURN_BOOL(intern->writable);
}
/* }}} */

/* {{{ isSeekable() */
PHP_METHOD(Signalforge_Http_Stream, isSeekable)
{
    signalforge_stream_object *intern = Z_SIGNALFORGE_STREAM_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();

    if (signalforge_stream_is_closed(intern) || signalforge_stream_is_detached(intern)) {
        RETURN_FALSE;
    }

    RETURN_BOOL(intern->seekable);
}
/* }}} */

/* {{{ getSize() */
PHP_METHOD(Signalforge_Http_Stream, getSize)
{
    signalforge_stream_object *intern = Z_SIGNALFORGE_STREAM_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();
    
    /* Check if stream is closed or detached */
    if (signalforge_stream_is_closed(intern) || signalforge_stream_is_detached(intern)) {
        RETURN_NULL();
    }

    /* String-based stream */
    if (intern->string_data) {
        RETURN_LONG(ZSTR_LEN(intern->string_data));
    }

    /* Resource-based stream */
    php_stream *stream = signalforge_get_php_stream(intern);
    if (stream) {
        php_stream_statbuf ssb;
        if (php_stream_stat(stream, &ssb) == 0) {
            RETURN_LONG(ssb.sb.st_size);
        }
    }

    RETURN_NULL();
}
/* }}} */

/* {{{ getMetadata($key = null) */
PHP_METHOD(Signalforge_Http_Stream, getMetadata)
{
    signalforge_stream_object *intern = Z_SIGNALFORGE_STREAM_P(ZEND_THIS);
    zend_string *key = NULL;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(key)
    ZEND_PARSE_PARAMETERS_END();

    /* Check if stream is closed or detached */
    if (signalforge_stream_is_closed(intern) || signalforge_stream_is_detached(intern)) {
        RETURN_NULL();
    }

    signalforge_load_metadata(intern);

    if (!intern->ht_metadata) {
        if (key) {
            RETURN_NULL();
        }
        array_init(return_value);
        return;
    }
    
    if (key) {
        zval *val = zend_hash_find(intern->ht_metadata, key);
        if (val) {
            RETURN_ZVAL(val, 1, 0);
        }
        RETURN_NULL();
    }
    
    /* Return all metadata */
    array_init(return_value);
    zend_hash_copy(Z_ARRVAL_P(return_value), intern->ht_metadata, zval_add_ref);
}
/* }}} */

/* {{{ close() */
PHP_METHOD(Signalforge_Http_Stream, close)
{
    signalforge_stream_object *intern = Z_SIGNALFORGE_STREAM_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();
    
    /* String-based stream - just mark as closed */
    if (intern->string_data) {
        intern->readable = 0;
        intern->writable = 0;
        intern->seekable = 0;
        /* Clear string data to mark as closed */
        zend_string_release(intern->string_data);
        intern->string_data = NULL;
        return;
    }
    
    /* Resource-based stream */
    if (Z_TYPE(intern->zv_resource) == IS_RESOURCE) {
        php_stream *stream = signalforge_get_php_stream(intern);
        if (stream) {
            php_stream_close(stream);
        }
        zval_ptr_dtor(&intern->zv_resource);
        ZVAL_UNDEF(&intern->zv_resource);
    }
    
    intern->readable = 0;
    intern->writable = 0;
    intern->seekable = 0;
}
/* }}} */

/* {{{ detach() */
PHP_METHOD(Signalforge_Http_Stream, detach)
{
    signalforge_stream_object *intern = Z_SIGNALFORGE_STREAM_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();
    
    /* String-based stream - return NULL */
    if (intern->string_data) {
        RETURN_NULL();
    }
    
    /* Resource-based stream - return resource and clear */
    if (Z_TYPE(intern->zv_resource) == IS_RESOURCE) {
        /* Try to get stream without generating notices */
        php_stream *stream = NULL;
        zend_error_handling error_handling;

        zend_replace_error_handling(EH_NORMAL, NULL, &error_handling);
        php_stream_from_zval_no_verify(stream, &intern->zv_resource);
        /* Clear any exception that was set */
        if (EG(exception)) {
            zend_clear_exception();
        }
        zend_restore_error_handling(&error_handling);

        if (stream != NULL) {
            /* Resource is valid - return it */
            zval resource_copy;
            ZVAL_COPY(&resource_copy, &intern->zv_resource);
            zval_ptr_dtor(&intern->zv_resource);
            ZVAL_UNDEF(&intern->zv_resource);
            intern->readable = 0;
            intern->writable = 0;
            intern->seekable = 0;

            /* Clear cached metadata */
            if (intern->ht_metadata) {
                zend_hash_destroy(intern->ht_metadata);
                FREE_HASHTABLE(intern->ht_metadata);
                intern->ht_metadata = NULL;
            }
            intern->metadata_loaded = 0;

            RETURN_ZVAL(&resource_copy, 0, 0);
        }
        /* Resource is invalid (closed) - clear and return NULL */
        /* Clear the zval without calling dtor on invalid resource */
        ZVAL_UNDEF(&intern->zv_resource);
        intern->readable = 0;
        intern->writable = 0;
        intern->seekable = 0;

        /* Clear cached metadata */
        if (intern->ht_metadata) {
            zend_hash_destroy(intern->ht_metadata);
            FREE_HASHTABLE(intern->ht_metadata);
            intern->ht_metadata = NULL;
        }
        intern->metadata_loaded = 0;

        RETURN_NULL();
    }
    
    /* No resource or resource already detached - return NULL */
    RETURN_NULL();
}
/* }}} */

/* {{{ __toString() */
/**
 * Get string representation of stream contents.
 *
 * Returns the entire stream contents as a string. For seekable streams,
 * temporarily seeks to beginning, reads all content, then restores position.
 * For non-seekable streams, reads from current position to end.
 *
 * @return string Stream contents as string
 */
PHP_METHOD(Signalforge_Http_Stream, __toString)
{
    signalforge_stream_object *intern = Z_SIGNALFORGE_STREAM_P(ZEND_THIS);
    zend_long saved_position;

    ZEND_PARSE_PARAMETERS_NONE();

    /* Check if stream is closed or detached - PSR-7 requires returning empty string */
    if (signalforge_stream_is_closed(intern) || signalforge_stream_is_detached(intern)) {
        RETURN_EMPTY_STRING();
    }

    /* Check if readable - PSR-7 requires returning empty string on error */
    if (!intern->readable) {
        RETURN_EMPTY_STRING();
    }

    /* String-based stream: return with proper refcount (zend_string_copy) */
    if (intern->string_data) {
        RETURN_STR(zend_string_copy(intern->string_data));
    }

    /* For resource-based streams */
    php_stream *stream = signalforge_get_php_stream(intern);
    if (!stream) {
        RETURN_EMPTY_STRING();
    }

    /* Save position */
    saved_position = php_stream_tell(stream);

    /* Rewind to start */
    if (php_stream_seek(stream, 0, SEEK_SET) != 0) {
        RETURN_EMPTY_STRING();
    }

    /* Read all contents */
    zend_string *contents = php_stream_copy_to_mem(stream, PHP_STREAM_COPY_ALL, 0);

    /* Restore position (ignore errors per PSR-7) */
    php_stream_seek(stream, saved_position, SEEK_SET);
    
    if (contents) {
        RETURN_STR(contents);
    }
    
    RETURN_EMPTY_STRING();
}
/* }}} */

/* ============================================================================
 * FACTORY METHODS
 * ============================================================================ */

/* {{{ fromString($string) */
/**
 * Create a stream from a string.
 *
 * Creates a readable, seekable stream containing the given string data.
 * The string is stored as a zero-copy reference to avoid duplication.
 *
 * @param string $string String data for the stream
 * @return StreamInterface New stream instance
 */
PHP_METHOD(Signalforge_Http_Stream, fromString)
{
    zend_string *string;
    signalforge_stream_object *intern;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(string)
    ZEND_PARSE_PARAMETERS_END();

    /* Create new instance */
    object_init_ex(return_value, signalforge_stream_ce);
    intern = Z_SIGNALFORGE_STREAM_P(return_value);

    /* Store string reference (TRUE zero-copy - no data duplication).
     * WHY: zend_string_addref() only increments reference count without copying
     * the underlying char buffer. This eliminates memory allocation and data
     * copying for string streams, providing massive performance gains for
     * large strings while maintaining memory safety through Zend's refcounting.
     */
    zend_string_addref(string);
    intern->string_data = string;
    intern->position = 0;
    intern->size = ZSTR_LEN(string);
    intern->readable = 1;
    intern->writable = 0;
    intern->seekable = 1;
}
/* }}} */

/* {{{ fromResource($resource) */
/**
 * Create a stream from a PHP resource.
 *
 * Wraps an existing PHP stream resource (file handle, etc.) in a StreamInterface.
 * The resource must be a valid stream resource. Read/write/seek capabilities
 * are determined from the underlying resource.
 *
 * @param resource $resource PHP stream resource
 * @return StreamInterface New stream instance
 * @throws InvalidArgumentException When resource is not a valid stream
 */
PHP_METHOD(Signalforge_Http_Stream, fromResource)
{
    zval *resource;
    signalforge_stream_object *intern;
    php_stream *stream;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(resource)
    ZEND_PARSE_PARAMETERS_END();

    /* Validate that resource is actually a resource */
    if (Z_TYPE_P(resource) != IS_RESOURCE) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Parameter must be a resource", 0);
        RETURN_THROWS();
    }

    /* Verify it's a stream resource */
    /* Check if resource is valid before attempting to get stream */
    if (zend_fetch_resource2(Z_RES_P(resource), "stream", php_file_le_stream(), php_file_le_pstream()) == NULL) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Resource is not a valid stream", 0);
        RETURN_THROWS();
    }
    php_stream_from_zval_no_verify(stream, resource);
    if (!stream) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Resource is not a valid stream", 0);
        RETURN_THROWS();
    }
    
    /* Create new instance */
    object_init_ex(return_value, signalforge_stream_ce);
    intern = Z_SIGNALFORGE_STREAM_P(return_value);
    
    /* Store resource */
    ZVAL_COPY(&intern->zv_resource, resource);
    intern->position = php_stream_tell(stream);
    
    /* Determine capabilities and size */
    php_stream_statbuf ssb;
    zend_bool is_seekable = 0;
    if (php_stream_stat(stream, &ssb) == 0) {
        intern->size = ssb.sb.st_size;
        /* Regular files are seekable */
        is_seekable = (ssb.sb.st_mode & S_IFREG) ? 1 : 0;
    } else {
        /* For php://memory and other non-file streams, determine size by seeking to end */
        zend_long current_pos = php_stream_tell(stream);
        if (php_stream_seek(stream, 0, SEEK_END) == 0) {
            intern->size = php_stream_tell(stream);
            is_seekable = 1;
            php_stream_seek(stream, current_pos, SEEK_SET);
        } else {
            /* If can't seek, size is unknown */
            intern->size = -1;
        }
    }
    intern->seekable = is_seekable;
    
    /* Determine capabilities from stream mode.
     * WHY: We query stream_get_meta_data() to get the mode string, then parse
     * it to determine read/write capabilities. This is the only reliable way
     * to detect capabilities since the mode is specified when opening the stream.
     */
    intern->readable = 1; /* Assume readable by default */
    intern->writable = 0; /* Default to not writable, will be detected below */

    /* Get mode from stream metadata to determine writable capability */
    {
        zval function_name, retval, params[1];
        ZVAL_STRING(&function_name, "stream_get_meta_data");
        ZVAL_COPY(&params[0], &intern->zv_resource);

        if (call_user_function(CG(function_table), NULL, &function_name, &retval, 1, params) == SUCCESS
            && Z_TYPE(retval) == IS_ARRAY) {
            zval *mode_zv = zend_hash_str_find(Z_ARRVAL(retval), "mode", sizeof("mode")-1);
            if (mode_zv && Z_TYPE_P(mode_zv) == IS_STRING) {
                const char *mode_str = Z_STRVAL_P(mode_zv);
                /* Check if mode allows writing: w, a, x, c, or + modifier */
                intern->writable = (strchr(mode_str, '+') != NULL ||
                                   strchr(mode_str, 'w') != NULL ||
                                   strchr(mode_str, 'a') != NULL ||
                                   strchr(mode_str, 'x') != NULL ||
                                   strchr(mode_str, 'c') != NULL) ? 1 : 0;
                /* Check if mode allows reading: r, or + modifier */
                intern->readable = (strchr(mode_str, 'r') != NULL ||
                                   strchr(mode_str, '+') != NULL) ? 1 : 0;
            }
        }

        zval_ptr_dtor(&retval);
        zval_ptr_dtor(&function_name);
        zval_ptr_dtor(&params[0]);
    }
}
/* }}} */

/* {{{ fromFile($path, $mode = 'r') */
/**
 * Create a stream from a file path.
 *
 * Opens the specified file with the given mode and wraps it in a StreamInterface.
 * If no mode is specified, defaults to 'r' (read-only). The file must be accessible
 * with the given mode or an exception is thrown.
 *
 * @param string $filename Path to file
 * @param string $mode File access mode (default: 'r')
 * @return StreamInterface New stream instance
 * @throws RuntimeException When file cannot be opened
 */
PHP_METHOD(Signalforge_Http_Stream, fromFile)
{
    zend_string *path;
    zend_string *mode = NULL;
    zend_bool mode_allocated = 0;
    signalforge_stream_object *intern;
    php_stream *stream;
    zval resource_zv;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(path)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR(mode)
    ZEND_PARSE_PARAMETERS_END();

    /* If no mode provided, default to "r" */
    if (!mode) {
        mode = zend_string_init("r", 1, 0);
        mode_allocated = 1;
    }

    /* Open file stream */
    stream = php_stream_open_wrapper(ZSTR_VAL(path), ZSTR_VAL(mode), 0, NULL);
    if (!stream) {
        if (mode_allocated) {
            zend_string_release(mode);
        }
        zend_throw_exception(spl_ce_RuntimeException,
            "Failed to open file", 0);
        RETURN_THROWS();
    }

    /* Create resource from stream */
    php_stream_to_zval(stream, &resource_zv);

    /* Create Stream instance directly */
    object_init_ex(return_value, signalforge_stream_ce);
    intern = Z_SIGNALFORGE_STREAM_P(return_value);

    /* Store resource */
    ZVAL_COPY(&intern->zv_resource, &resource_zv);

    /* Determine capabilities based on file mode.
     * WHY: Parse the mode string to determine read/write capabilities.
     * For reading: modes containing 'r' or '+' allow reading.
     * For writing: modes containing 'w', 'a', 'x', 'c', or '+' allow writing.
     */
    {
        const char *mode_str = ZSTR_VAL(mode);
        /* Check if mode allows writing (+, w, a, x, c) */
        intern->writable = (strchr(mode_str, '+') != NULL ||
                           strchr(mode_str, 'w') != NULL ||
                           strchr(mode_str, 'a') != NULL ||
                           strchr(mode_str, 'x') != NULL ||
                           strchr(mode_str, 'c') != NULL) ? 1 : 0;
        /* Check if mode allows reading (r or +) */
        intern->readable = (strchr(mode_str, 'r') != NULL ||
                           strchr(mode_str, '+') != NULL) ? 1 : 0;
    }

    /* Initialize other properties */
    intern->position = php_stream_tell(stream);
    intern->size = -1; /* Will be determined lazily */
    intern->seekable = 1; /* Assume seekable for files */
    intern->string_data = NULL;
    intern->ht_metadata = NULL;
    intern->metadata_loaded = 0;

    /* Clean up */
    if (mode_allocated) {
        zend_string_release(mode);
    }
    zval_ptr_dtor(&resource_zv);
}
/* }}} */

/* ============================================================================
 * ARGINFO DEFINITIONS
 * ============================================================================ */

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_stream_fromString, 0, 1, Signalforge\\NativeHttp\\Stream, 0)
    ZEND_ARG_TYPE_INFO(0, string, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_stream_fromResource, 0, 1, Signalforge\\NativeHttp\\Stream, 0)
    /* PHP 8.4+ does not allow IS_RESOURCE in zend_type - use untyped parameter.
     * Runtime validation still enforces resource type in fromResource() implementation. */
    ZEND_ARG_INFO(0, resource)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_stream_fromFile, 0, 1, Signalforge\\NativeHttp\\Stream, 0)
    ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, mode, IS_STRING, 0, "'r'")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_stream_read, 0, 1, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, length, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_stream_getContents, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_stream_eof, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_stream_write, 0, 1, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, string, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_stream_seek, 0, 1, IS_VOID, 0)
    ZEND_ARG_TYPE_INFO(0, offset, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, whence, IS_LONG, 0, "SEEK_SET")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_stream_tell, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_stream_rewind, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_stream_isReadable, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_stream_isWritable, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_stream_isSeekable, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_stream_getSize, 0, 0, IS_LONG, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_stream_getMetadata, 0, 0, IS_MIXED, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, key, IS_STRING, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_stream_close, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

/* PHP 8.4+ does not allow IS_RESOURCE in zend_type.
 * PSR-7 StreamInterface::detach() returns resource|null, but we cannot express this
 * in arginfo. Return type validation happens at runtime. */
ZEND_BEGIN_ARG_INFO_EX(arginfo_stream_detach, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_stream___toString, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

/* ============================================================================
 * METHOD REGISTRATION
 * ============================================================================ */

static const zend_function_entry signalforge_stream_methods[] = {
    /* Factory methods */
    PHP_ME(Signalforge_Http_Stream, fromString, arginfo_stream_fromString, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(Signalforge_Http_Stream, fromResource, arginfo_stream_fromResource, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(Signalforge_Http_Stream, fromFile, arginfo_stream_fromFile, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    
    /* StreamInterface */
    PHP_ME(Signalforge_Http_Stream, read, arginfo_stream_read, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Stream, getContents, arginfo_stream_getContents, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Stream, eof, arginfo_stream_eof, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Stream, write, arginfo_stream_write, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Stream, seek, arginfo_stream_seek, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Stream, tell, arginfo_stream_tell, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Stream, rewind, arginfo_stream_rewind, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Stream, isReadable, arginfo_stream_isReadable, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Stream, isWritable, arginfo_stream_isWritable, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Stream, isSeekable, arginfo_stream_isSeekable, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Stream, getSize, arginfo_stream_getSize, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Stream, getMetadata, arginfo_stream_getMetadata, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Stream, close, arginfo_stream_close, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Stream, detach, arginfo_stream_detach, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Stream, __toString, arginfo_stream___toString, ZEND_ACC_PUBLIC)
    
    PHP_FE_END
};

void signalforge_stream_register_class(void)
{
    zend_class_entry ce;

    INIT_NS_CLASS_ENTRY(ce, "Signalforge\\NativeHttp", "Stream", signalforge_stream_methods);
    signalforge_stream_ce = zend_register_internal_class(&ce);
    signalforge_stream_ce->ce_flags |= ZEND_ACC_FINAL;
    signalforge_stream_ce->create_object = signalforge_stream_create_object;

    /* Implement PSR-7 StreamInterface */
    if (psr7_stream_interface_ce) {
        zend_class_implements(signalforge_stream_ce, 1, psr7_stream_interface_ce);
    }

    /* Copy object handlers */
    memcpy(&signalforge_stream_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    signalforge_stream_object_handlers.offset = XtOffsetOf(signalforge_stream_object, std);
    signalforge_stream_object_handlers.free_obj = signalforge_stream_free_object;

}
