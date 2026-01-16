/*
 * uploadedfile.c
 *
 * Signalforge HTTP UploadedFile Class Implementation
 *
 * This file implements the Signalforge\Http\UploadedFile class, providing
 * PSR-7 UploadedFileInterface compliance.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_signalforge_http.h"
#include "psr7_interfaces.h"
#include "uploadedfile.h"
#include "stream.h"
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

zend_class_entry *signalforge_uploadedfile_ce = NULL;
static zend_object_handlers signalforge_uploadedfile_object_handlers;

/* Forward declaration */
extern zend_class_entry *signalforge_stream_ce;

/* SPL Exception classes */
extern PHPAPI zend_class_entry *spl_ce_InvalidArgumentException;
extern PHPAPI zend_class_entry *spl_ce_RuntimeException;

/* ============================================================================
 * OBJECT HANDLERS
 * ============================================================================ */

static zend_object *signalforge_uploadedfile_create_object(zend_class_entry *ce)
{
    signalforge_uploadedfile_object *intern;

    intern = zend_object_alloc(sizeof(signalforge_uploadedfile_object), ce);

    /* Initialize object state */
    intern->tmp_name = NULL;
    intern->client_filename = NULL;
    intern->client_media_type = NULL;
    ZVAL_UNDEF(&intern->zv_stream);

    /* Initialize upload state */
    intern->size = 0;
    intern->error = 0;
    intern->stream_loaded = 0;

    /* Initialize streamforge state */
    intern->from_streamforge = 0;
    intern->streamforge_index = -1;

    /* Set up Zend object infrastructure */
    zend_object_std_init(&intern->std, ce);
    object_properties_init(&intern->std, ce);
    intern->std.handlers = &signalforge_uploadedfile_object_handlers;

    return &intern->std;
}

static void signalforge_uploadedfile_free_object(zend_object *object)
{
    signalforge_uploadedfile_object *intern = signalforge_uploadedfile_from_obj(object);

    /* Clean up owned resources */
    if (intern->tmp_name) {
        zend_string_release(intern->tmp_name);
    }
    if (intern->client_filename) {
        zend_string_release(intern->client_filename);
    }
    if (intern->client_media_type) {
        zend_string_release(intern->client_media_type);
    }

    /* Clean up cached stream */
    if (!Z_ISUNDEF(intern->zv_stream)) {
        zval_ptr_dtor(&intern->zv_stream);
    }

    /* Clean up Zend object */
    zend_object_std_dtor(&intern->std);
}

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

/* Validate that cached stream is still valid for this uploaded file instance.
 *
 * This function performs lazy cleanup of invalidated caches to prevent
 * reference counting issues and crashes during cache invalidation.
 */
static int signalforge_validate_cached_stream(signalforge_uploadedfile_object *intern)
{
    /* Check if cache is properly loaded for this instance */
    if (!intern->stream_loaded) {
        /* Clean up any leftover zval from invalidation */
        if (!Z_ISUNDEF(intern->zv_stream)) {
            zval_ptr_dtor(&intern->zv_stream);
            ZVAL_UNDEF(&intern->zv_stream);
        }
        return 0;
    }

    /* Check if cached stream object exists and is valid */
    if (Z_ISUNDEF(intern->zv_stream) || Z_TYPE(intern->zv_stream) != IS_OBJECT) {
        /* Clean up invalid zval */
        if (!Z_ISUNDEF(intern->zv_stream)) {
            zval_ptr_dtor(&intern->zv_stream);
            ZVAL_UNDEF(&intern->zv_stream);
        }
        intern->stream_loaded = 0;
        return 0;
    }

    /* Cache is valid */
    return 1;
}

/* Invalidate stream cache for this instance.
 *
 * This marks the cache as invalid without immediately destroying the zval,
 * preventing reference counting issues. Cleanup happens lazily during validation.
 */
static void signalforge_invalidate_stream_cache(signalforge_uploadedfile_object *intern)
{
    /* Mark cache as invalid - don't destroy immediately to avoid reference issues */
    intern->stream_loaded = 0;
    /* Keep the zval for now, validation will clean it up if accessed again */
}

/**
 * Create UploadedFile from $_FILES array entry
 */
signalforge_uploadedfile_object *signalforge_uploadedfile_from_files_array(zval *file_data, zval *return_value)
{
    signalforge_uploadedfile_object *intern;
    zval *tmp_name_zv, *name_zv, *type_zv, *size_zv, *error_zv;

    /* Validate input is an array */
    if (Z_TYPE_P(file_data) != IS_ARRAY) {
        /* Handle non-array input gracefully by creating empty object */
        object_init_ex(return_value, signalforge_uploadedfile_ce);
        intern = Z_SIGNALFORGE_UPLOADEDFILE_P(return_value);
        intern->tmp_name = NULL;
        intern->client_filename = NULL;
        intern->client_media_type = NULL;
        intern->size = 0;
        intern->error = 4; /* UPLOAD_ERR_NO_FILE */
        return intern;
    }

    /* Create new instance using proper object initialization */
    object_init_ex(return_value, signalforge_uploadedfile_ce);
    intern = Z_SIGNALFORGE_UPLOADEDFILE_P(return_value);

    /* Extract tmp_name */
    tmp_name_zv = zend_hash_str_find(Z_ARRVAL_P(file_data), "tmp_name", sizeof("tmp_name") - 1);
    if (tmp_name_zv && Z_TYPE_P(tmp_name_zv) == IS_STRING) {
        intern->tmp_name = zend_string_copy(Z_STR_P(tmp_name_zv));
    } else {
        intern->tmp_name = NULL;
    }

    /* Extract name (client filename) */
    name_zv = zend_hash_str_find(Z_ARRVAL_P(file_data), "name", sizeof("name") - 1);
    if (name_zv && Z_TYPE_P(name_zv) == IS_STRING) {
        intern->client_filename = zend_string_copy(Z_STR_P(name_zv));
    } else {
        intern->client_filename = NULL;
    }

    /* Extract type (client media type) */
    type_zv = zend_hash_str_find(Z_ARRVAL_P(file_data), "type", sizeof("type") - 1);
    if (type_zv && Z_TYPE_P(type_zv) == IS_STRING) {
        intern->client_media_type = zend_string_copy(Z_STR_P(type_zv));
    } else {
        intern->client_media_type = NULL;
    }

    /* Extract size (use ZEND_STRTOL for safe string-to-long conversion) */
    size_zv = zend_hash_str_find(Z_ARRVAL_P(file_data), "size", sizeof("size") - 1);
    if (size_zv) {
        if (Z_TYPE_P(size_zv) == IS_LONG) {
            intern->size = Z_LVAL_P(size_zv);
        } else if (Z_TYPE_P(size_zv) == IS_STRING) {
            char *endptr = NULL;
            zend_long parsed_size = ZEND_STRTOL(Z_STRVAL_P(size_zv), &endptr, 10);
            if (endptr != NULL && *endptr == '\0' && parsed_size >= 0) {
                intern->size = parsed_size;
            } else {
                intern->size = 0;
            }
        }
    } else {
        intern->size = 0;
    }

    /* Extract error (validate error codes 0-8) */
    error_zv = zend_hash_str_find(Z_ARRVAL_P(file_data), "error", sizeof("error") - 1);
    if (error_zv) {
        if (Z_TYPE_P(error_zv) == IS_LONG) {
            intern->error = Z_LVAL_P(error_zv);
        } else if (Z_TYPE_P(error_zv) == IS_STRING) {
            char *endptr = NULL;
            zend_long parsed_error = ZEND_STRTOL(Z_STRVAL_P(error_zv), &endptr, 10);
            if (endptr != NULL && *endptr == '\0' && parsed_error >= 0 && parsed_error <= 8) {
                intern->error = parsed_error;
            } else {
                intern->error = 4; /* UPLOAD_ERR_NO_FILE */
            }
        }
    } else {
        intern->error = 0; /* UPLOAD_ERR_OK */
    }

    return intern;
}

/* ============================================================================
 * PSR-17 Factory Method
 * ============================================================================ */

/**
 * Create a new UploadedFile instance.
 *
 * PSR-17 factory method for creating UploadedFile instances programmatically.
 *
 * @param StreamInterface $stream The underlying stream representing the uploaded file content
 * @param int|null $size The size of the file in bytes
 * @param int $error The PHP file upload error constant
 * @param string|null $clientFilename The filename as provided by the client
 * @param string|null $clientMediaType The media type as provided by the client
 * @return UploadedFile New uploaded file instance
 */
PHP_METHOD(Signalforge_Http_UploadedFile, create)
{
    signalforge_uploadedfile_object *intern;
    zval *stream_param;
    zend_long size = 0;
    zend_bool size_is_null = 1;
    zend_long error = 0; /* UPLOAD_ERR_OK */
    zend_string *client_filename = NULL;
    zend_string *client_media_type = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 5)
        Z_PARAM_OBJECT_OF_CLASS(stream_param, signalforge_stream_ce)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG_OR_NULL(size, size_is_null)
        Z_PARAM_LONG(error)
        Z_PARAM_STR_OR_NULL(client_filename)
        Z_PARAM_STR_OR_NULL(client_media_type)
    ZEND_PARSE_PARAMETERS_END();

    /* Create new instance */
    object_init_ex(return_value, signalforge_uploadedfile_ce);
    intern = Z_SIGNALFORGE_UPLOADEDFILE_P(return_value);

    /* Store the stream reference */
    ZVAL_COPY(&intern->zv_stream, stream_param);
    intern->stream_loaded = 1;

    /* Set size */
    if (!size_is_null) {
        intern->size = size;
    } else {
        /* Try to get size from stream */
        signalforge_stream_object *stream_intern = Z_SIGNALFORGE_STREAM_P(stream_param);
        if (stream_intern->size >= 0) {
            intern->size = stream_intern->size;
        } else {
            intern->size = 0;
        }
    }

    /* Set error */
    intern->error = error;

    /* Set client filename */
    if (client_filename) {
        intern->client_filename = zend_string_copy(client_filename);
    } else {
        intern->client_filename = NULL;
    }

    /* Set client media type */
    if (client_media_type) {
        intern->client_media_type = zend_string_copy(client_media_type);
    } else {
        intern->client_media_type = NULL;
    }

    /* No tmp_name for programmatically created files */
    intern->tmp_name = NULL;
}
/* }}} */

/* ============================================================================
 * PSR-7 UploadedFileInterface Methods
 * ============================================================================ */

/**
 * Get a StreamInterface representation of the uploaded file.
 *
 * This method provides lazy-loaded access to the uploaded file content through
 * a PSR-7 StreamInterface. The stream is cached after first access for performance.
 * If the file cannot be opened or has upload errors, a RuntimeException is thrown.
 *
 * @return StreamInterface Stream representation of the uploaded file
 * @throws RuntimeException When file cannot be opened or upload has errors
 */
PHP_METHOD(Signalforge_Http_UploadedFile, getStream)
{
    signalforge_uploadedfile_object *intern = Z_SIGNALFORGE_UPLOADEDFILE_P(ZEND_THIS);
    php_stream *stream;
    zval resource_zv;

    ZEND_PARSE_PARAMETERS_NONE();

    /* Check error first */
    if (intern->error != 0) { /* UPLOAD_ERR_OK = 0 */
        zend_throw_exception_ex(zend_ce_exception, 0,
            "Cannot get stream: file upload error " ZEND_LONG_FMT, intern->error);
        RETURN_THROWS();
    }

    /* Safe stream caching with per-instance isolation.
     *
     * This caching mechanism ensures that:
     * 1. Each UploadedFile instance has its own isolated cache
     * 2. Cached streams are validated before use
     * 3. Cache is invalidated when files are moved (moveTo)
     * 4. No state leakage between different UploadedFile instances
     * 5. Proper cleanup prevents memory leaks and crashes
     */
    if (intern->stream_loaded && !Z_ISUNDEF(intern->zv_stream)) {
        /* Validate cached stream is still valid for this instance */
        if (signalforge_validate_cached_stream(intern)) {
            /* Return cached stream with proper reference counting */
            RETVAL_ZVAL(&intern->zv_stream, 1, 0);
            return;
        } else {
            /* Invalidate corrupted cache */
            signalforge_invalidate_stream_cache(intern);
        }
    }

    /* Check tmp_name exists and is non-empty */
    if (!intern->tmp_name || ZSTR_LEN(intern->tmp_name) == 0) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Temporary file path is not available", 0);
        RETURN_THROWS();
    }

    /* Validate file path for security */
    if (strstr(ZSTR_VAL(intern->tmp_name), "..") != NULL) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Invalid file path: contains directory traversal", 0);
        RETURN_THROWS();
    }

    /* Open file stream - this will fail if file doesn't exist or isn't readable */
    stream = php_stream_open_wrapper(ZSTR_VAL(intern->tmp_name), "rb", 0, NULL);
    if (!stream) {
        /* PSR-7 requires RuntimeException when stream cannot be created */
        zend_throw_exception(spl_ce_RuntimeException,
            "Unable to open uploaded file for reading", 0);
        RETURN_THROWS();
    }

    /* Create resource from stream */
    php_stream_to_zval(stream, &resource_zv);

    /* Create Stream instance directly */
    signalforge_stream_object *stream_intern;
    zval stream_zv;
    object_init_ex(&stream_zv, signalforge_stream_ce);
    stream_intern = Z_SIGNALFORGE_STREAM_P(&stream_zv);

    /* Initialize object fields */
    stream_intern->string_data = NULL;
    stream_intern->ht_metadata = NULL;
    stream_intern->metadata_loaded = 0;

    /* Store resource */
    ZVAL_COPY(&stream_intern->zv_resource, &resource_zv);

    /* Initialize with safe defaults */
    stream_intern->position = 0;
    stream_intern->size = -1;
    stream_intern->seekable = 1;
    stream_intern->readable = 1;
    stream_intern->writable = 0;

    /* Cache the created stream for future access */
    ZVAL_COPY(&intern->zv_stream, &stream_zv);
    intern->stream_loaded = 1;

    /* Return the stream - RETVAL_ZVAL with dtor=1 already destroys stream_zv */
    RETVAL_ZVAL(&stream_zv, 1, 1);

    zval_ptr_dtor(&resource_zv);
    /* Successfully created stream - return it */
}
/* }}} */

/**
 * Move the uploaded file to a new location.
 *
 * Atomically moves the uploaded file from its temporary location to the specified
 * target path using rename(). The target directory must be writable and the path
 * must not contain directory traversal sequences (..).
 *
 * After successful move, the UploadedFile becomes unusable for further operations
 * as required by PSR-7. Any cached stream is invalidated.
 *
 * @param string $targetPath Absolute path where to move the uploaded file
 * @throws InvalidArgumentException When target path is invalid or empty
 * @throws RuntimeException When move operation fails or file has upload errors
 */
PHP_METHOD(Signalforge_Http_UploadedFile, moveTo)
{
    signalforge_uploadedfile_object *intern;
    zend_string *target_path;

    intern = Z_SIGNALFORGE_UPLOADEDFILE_P(ZEND_THIS);

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(target_path)
    ZEND_PARSE_PARAMETERS_END();

    /* Check error first */
    if (intern->error != 0) { /* UPLOAD_ERR_OK = 0 */
        zend_throw_exception(spl_ce_RuntimeException,
            "Cannot move file: file upload error", 0);
        RETURN_THROWS();
    }

    /* Validate target path */
    if (ZSTR_LEN(target_path) == 0) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Target path cannot be empty", 0);
        RETURN_THROWS();
    }

    /* Validate target path for security */
    if (strstr(ZSTR_VAL(target_path), "..") != NULL) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Invalid target path: contains directory traversal", 0);
        RETURN_THROWS();
    }

    /* Check if target directory is writable - safely */
    char *target_dir = NULL;
    char *last_slash = NULL;

    target_dir = estrndup(ZSTR_VAL(target_path), ZSTR_LEN(target_path));
    if (target_dir) {
        last_slash = strrchr(target_dir, '/');
        if (last_slash) {
            *last_slash = '\0';
            if (access(target_dir, W_OK) != 0) {
                efree(target_dir);
                zend_throw_exception(spl_ce_RuntimeException,
                    "Target directory is not writable", 0);
                RETURN_THROWS();
            }
        }
        efree(target_dir);
    }

    /* Check tmp_name */
    if (!intern->tmp_name || ZSTR_LEN(intern->tmp_name) == 0) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Temporary file path is not available", 0);
        RETURN_THROWS();
    }

    /* Move file using rename() - atomic operation */
    if (rename(ZSTR_VAL(intern->tmp_name), ZSTR_VAL(target_path)) != 0) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Failed to move uploaded file", 0);
        RETURN_THROWS();
    }

    /* Mark streamforge temp file as moved (prevents RSHUTDOWN cleanup) */
    signalforge_uploadedfile_mark_moved(intern);

    /* Update tmp_name to new location */
    if (intern->tmp_name) {
        zend_string_release(intern->tmp_name);
    }
    intern->tmp_name = zend_string_copy(target_path);

    /* Invalidate cached stream since file location changed */
    signalforge_invalidate_stream_cache(intern);
}
/* }}} */

/**
 * Get the size of the uploaded file in bytes.
 *
 * @return int|null File size in bytes, or null if unknown
 */
PHP_METHOD(Signalforge_Http_UploadedFile, getSize)
{
    signalforge_uploadedfile_object *intern = Z_SIGNALFORGE_UPLOADEDFILE_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();
    RETURN_LONG(intern->size);
}
/* }}} */

/**
 * Get the error code associated with the file upload.
 *
 * Returns one of PHP's UPLOAD_ERR_* constants indicating the status
 * of the file upload operation.
 *
 * @return int Upload error code (UPLOAD_ERR_* constant)
 */
PHP_METHOD(Signalforge_Http_UploadedFile, getError)
{
    signalforge_uploadedfile_object *intern = Z_SIGNALFORGE_UPLOADEDFILE_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();
    RETURN_LONG(intern->error);
}
/* }}} */

/**
 * Get the original filename sent by the client.
 *
 * @return string|null Original filename, or null if not provided
 */
PHP_METHOD(Signalforge_Http_UploadedFile, getClientFilename)
{
    signalforge_uploadedfile_object *intern = Z_SIGNALFORGE_UPLOADEDFILE_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();
    
    if (intern->client_filename) {
        RETURN_STR(zend_string_copy(intern->client_filename));
    }
    RETURN_NULL();
}
/* }}} */

/**
 * Get the media type sent by the client.
 *
 * @return string|null MIME type, or null if not provided
 */
PHP_METHOD(Signalforge_Http_UploadedFile, getClientMediaType)
{
    signalforge_uploadedfile_object *intern = Z_SIGNALFORGE_UPLOADEDFILE_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();
    
    if (intern->client_media_type) {
        RETURN_STR(zend_string_copy(intern->client_media_type));
    }
    RETURN_NULL();
}
/* }}} */

/* ============================================================================
 * ARGINFO DEFINITIONS
 * ============================================================================ */

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_uploadedfile_create, 0, 1, Signalforge\\NativeHttp\\UploadedFile, 0)
    ZEND_ARG_OBJ_INFO(0, stream, Signalforge\\NativeHttp\\Stream, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, size, IS_LONG, 1, "null")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, error, IS_LONG, 0, "0")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, clientFilename, IS_STRING, 1, "null")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, clientMediaType, IS_STRING, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_uploadedfile_getStream, 0, 0, Psr\\Http\\Message\\StreamInterface, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_uploadedfile_moveTo, 0, 1, IS_VOID, 0)
    ZEND_ARG_TYPE_INFO(0, targetPath, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_uploadedfile_getSize, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_uploadedfile_getError, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_uploadedfile_getClientFilename, 0, 0, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_uploadedfile_getClientMediaType, 0, 0, IS_STRING, 1)
ZEND_END_ARG_INFO()

/* ============================================================================
 * METHOD REGISTRATION
 * ============================================================================ */

static const zend_function_entry signalforge_uploadedfile_methods[] = {
    PHP_ME(Signalforge_Http_UploadedFile, create, arginfo_uploadedfile_create, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(Signalforge_Http_UploadedFile, getStream, arginfo_uploadedfile_getStream, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_UploadedFile, moveTo, arginfo_uploadedfile_moveTo, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_UploadedFile, getSize, arginfo_uploadedfile_getSize, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_UploadedFile, getError, arginfo_uploadedfile_getError, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_UploadedFile, getClientFilename, arginfo_uploadedfile_getClientFilename, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_UploadedFile, getClientMediaType, arginfo_uploadedfile_getClientMediaType, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

/* ============================================================================
 * CLASS REGISTRATION
 * ============================================================================ */

void signalforge_uploadedfile_register_class(void)
{
    zend_class_entry ce;

    INIT_NS_CLASS_ENTRY(ce, "Signalforge\\NativeHttp", "UploadedFile", signalforge_uploadedfile_methods);
    signalforge_uploadedfile_ce = zend_register_internal_class(&ce);
    signalforge_uploadedfile_ce->ce_flags |= ZEND_ACC_FINAL;

    /* Implement PSR-7 UploadedFileInterface */
    if (psr7_uploadedfile_interface_ce) {
        zend_class_implements(signalforge_uploadedfile_ce, 1, psr7_uploadedfile_interface_ce);
    }

    /* Copy object handlers */
    memcpy(&signalforge_uploadedfile_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

    /* Set custom handlers */
    signalforge_uploadedfile_object_handlers.free_obj = signalforge_uploadedfile_free_object;
    signalforge_uploadedfile_object_handlers.offset = XtOffsetOf(signalforge_uploadedfile_object, std);

    /* Set custom object creation function */
    signalforge_uploadedfile_ce->create_object = signalforge_uploadedfile_create_object;

}

/* ============================================================================
 * STREAMFORGE INTEGRATION
 *
 * When the streamforge proxy handles multipart uploads, it writes files to disk
 * and passes metadata via HTTP_X_UPLOAD_* headers instead of populating $_FILES.
 * These functions create UploadedFile objects from that metadata.
 * ============================================================================ */

/**
 * Create UploadedFile from streamforge HTTP_X_UPLOAD_* headers
 *
 * Streamforge sends these headers for each uploaded file:
 *   HTTP_X_UPLOAD_N_NAME      - Form field name
 *   HTTP_X_UPLOAD_N_FILENAME  - Original client filename
 *   HTTP_X_UPLOAD_N_PATH      - Temp file path on disk
 *   HTTP_X_UPLOAD_N_SIZE      - File size in bytes
 *   HTTP_X_UPLOAD_N_TYPE      - MIME type (optional)
 *
 * @param server_ht  $_SERVER hashtable
 * @param index      Upload index (0, 1, 2, ...)
 * @param return_value  zval to initialize with UploadedFile object
 * @return Internal object pointer, or NULL on error
 */
signalforge_uploadedfile_object *signalforge_uploadedfile_from_streamforge(
    HashTable *server_ht, int index, zval *return_value)
{
    signalforge_uploadedfile_object *intern;
    char key[64];
    zval *val;

    /* Create new instance */
    object_init_ex(return_value, signalforge_uploadedfile_ce);
    intern = Z_SIGNALFORGE_UPLOADEDFILE_P(return_value);

    /* Mark as from streamforge */
    intern->from_streamforge = 1;
    intern->error = 0; /* UPLOAD_ERR_OK - streamforge already validated */

    /* Get temp file path (required) */
    snprintf(key, sizeof(key), "HTTP_X_UPLOAD_%d_PATH", index);
    val = zend_hash_str_find(server_ht, key, strlen(key));
    if (val && Z_TYPE_P(val) == IS_STRING && Z_STRLEN_P(val) > 0) {
        intern->tmp_name = zend_string_copy(Z_STR_P(val));

        /* Register for cleanup tracking */
        if (SIGNALFORGE_HTTP_G(streamforge_upload_count) < SIGNALFORGE_MAX_STREAMFORGE_UPLOADS) {
            int idx = SIGNALFORGE_HTTP_G(streamforge_upload_count)++;
            SIGNALFORGE_HTTP_G(streamforge_temp_paths)[idx] = estrdup(Z_STRVAL_P(val));
            SIGNALFORGE_HTTP_G(streamforge_temp_moved)[idx] = 0;
            intern->streamforge_index = idx;
        }
    } else {
        /* No path - upload error */
        intern->tmp_name = NULL;
        intern->error = 4; /* UPLOAD_ERR_NO_FILE */
        return intern;
    }

    /* Get original filename */
    snprintf(key, sizeof(key), "HTTP_X_UPLOAD_%d_FILENAME", index);
    val = zend_hash_str_find(server_ht, key, strlen(key));
    if (val && Z_TYPE_P(val) == IS_STRING) {
        intern->client_filename = zend_string_copy(Z_STR_P(val));
    } else {
        intern->client_filename = NULL;
    }

    /* Get MIME type */
    snprintf(key, sizeof(key), "HTTP_X_UPLOAD_%d_TYPE", index);
    val = zend_hash_str_find(server_ht, key, strlen(key));
    if (val && Z_TYPE_P(val) == IS_STRING) {
        intern->client_media_type = zend_string_copy(Z_STR_P(val));
    } else {
        intern->client_media_type = NULL;
    }

    /* Get file size */
    snprintf(key, sizeof(key), "HTTP_X_UPLOAD_%d_SIZE", index);
    val = zend_hash_str_find(server_ht, key, strlen(key));
    if (val) {
        if (Z_TYPE_P(val) == IS_LONG) {
            intern->size = Z_LVAL_P(val);
        } else if (Z_TYPE_P(val) == IS_STRING) {
            char *endptr = NULL;
            zend_long parsed = ZEND_STRTOL(Z_STRVAL_P(val), &endptr, 10);
            if (endptr && *endptr == '\0' && parsed >= 0) {
                intern->size = parsed;
            }
        }
    }

    return intern;
}

/**
 * Mark a streamforge upload as moved
 *
 * Called from moveTo() when file is successfully moved. This prevents
 * RSHUTDOWN from trying to clean up a file that no longer exists at
 * its original temp location.
 */
void signalforge_uploadedfile_mark_moved(signalforge_uploadedfile_object *intern)
{
    if (intern->from_streamforge && intern->streamforge_index >= 0 &&
        intern->streamforge_index < SIGNALFORGE_MAX_STREAMFORGE_UPLOADS) {
        SIGNALFORGE_HTTP_G(streamforge_temp_moved)[intern->streamforge_index] = 1;
    }
}

