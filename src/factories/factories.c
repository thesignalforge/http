/*
 * factories.c
 *
 * Signalforge HTTP PSR-17 Factory Classes Implementation
 *
 * Implements:
 * - RequestFactory (Psr\Http\Message\RequestFactoryInterface)
 * - ResponseFactory (Psr\Http\Message\ResponseFactoryInterface)
 * - StreamFactory (Psr\Http\Message\StreamFactoryInterface)
 * - UriFactory (Psr\Http\Message\UriFactoryInterface)
 * - UploadedFileFactory (Psr\Http\Message\UploadedFileFactoryInterface)
 * - ServerRequestFactory (Psr\Http\Message\ServerRequestFactoryInterface)
 *
 * Copyright (c) 2026 Signalforge
 * License: MIT
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_signalforge_http.h"
#include "factories.h"
#include "../psr7_interfaces.h"
#include "../request.h"
#include "../response.h"
#include "../stream.h"
#include "../uri.h"
#include "../uploadedfile.h"
#include "rfc1867.h"  /* For UPLOAD_ERR_OK */
#include "zend_smart_str.h"
#include <ctype.h>

/* ============================================================================
 * EXTERNAL CLASS ENTRIES
 * ============================================================================ */

extern zend_class_entry *signalforge_request_ce;
extern zend_class_entry *signalforge_response_ce;
extern zend_class_entry *signalforge_stream_ce;
extern zend_class_entry *signalforge_uri_ce;
extern zend_class_entry *signalforge_uploadedfile_ce;

/* PSR-17 interface entries */
extern zend_class_entry *psr17_request_factory_interface_ce;
extern zend_class_entry *psr17_response_factory_interface_ce;
extern zend_class_entry *psr17_stream_factory_interface_ce;
extern zend_class_entry *psr17_uri_factory_interface_ce;
extern zend_class_entry *psr17_uploaded_file_factory_interface_ce;
extern zend_class_entry *psr17_server_request_factory_interface_ce;

/* SPL Exceptions */
extern PHPAPI zend_class_entry *spl_ce_InvalidArgumentException;
extern PHPAPI zend_class_entry *spl_ce_RuntimeException;

/* ============================================================================
 * CLASS ENTRIES
 * ============================================================================ */

zend_class_entry *signalforge_request_factory_ce = NULL;
zend_class_entry *signalforge_response_factory_ce = NULL;
zend_class_entry *signalforge_stream_factory_ce = NULL;
zend_class_entry *signalforge_uri_factory_ce = NULL;
zend_class_entry *signalforge_uploaded_file_factory_ce = NULL;
zend_class_entry *signalforge_server_request_factory_ce = NULL;

/* ============================================================================
 * INTERNAL HELPER: CREATE URI FROM STRING
 * ============================================================================ */

/* Forward declaration from uri.c */
extern zend_object *signalforge_uri_create_from_string(const char *uri, size_t len);

/* ============================================================================
 * INTERNAL HELPER: CREATE CLIENT-SIDE REQUEST
 *
 * This creates a Request object suitable for outgoing HTTP requests (client use).
 * Unlike the server-side Request which reads from superglobals, this creates
 * a minimal request with just method and URI that can be modified with with*().
 * ============================================================================ */

zend_object *signalforge_create_client_request(
    const char *method,
    size_t method_len,
    zend_object *uri_obj
)
{
    zval request_zv;
    signalforge_request_object *intern;

    /* Create new Request instance */
    object_init_ex(&request_zv, signalforge_request_ce);
    intern = Z_SIGNALFORGE_REQUEST_P(&request_zv);

    /* Set method (uppercase) */
    zend_string *method_str = zend_string_alloc(method_len, 0);
    for (size_t i = 0; i < method_len; i++) {
        ZSTR_VAL(method_str)[i] = toupper((unsigned char)method[i]);
    }
    ZSTR_VAL(method_str)[method_len] = '\0';
    ZVAL_STR(&intern->zv_method, method_str);
    intern->flags |= SF_REQ_FLAG_METHOD_RESOLVED;

    /* Convert URI object to string and store as zv_uri
     * The Request::getUri() method expects zv_uri to be a STRING containing
     * the full URI (with scheme://). When it sees "://", it creates a Uri
     * object from it directly.
     */
    signalforge_uri_object *uri_intern = signalforge_uri_from_obj(uri_obj);
    smart_str uri_str = {0};

    /* Build URI string from Uri object components */
    if (uri_intern->scheme && ZSTR_LEN(uri_intern->scheme) > 0) {
        smart_str_append(&uri_str, uri_intern->scheme);
        smart_str_appendl(&uri_str, "://", 3);
    }

    /* Add authority (userinfo@host:port) */
    if (uri_intern->user && ZSTR_LEN(uri_intern->user) > 0) {
        smart_str_append(&uri_str, uri_intern->user);
        if (uri_intern->pass && ZSTR_LEN(uri_intern->pass) > 0) {
            smart_str_appendc(&uri_str, ':');
            smart_str_append(&uri_str, uri_intern->pass);
        }
        smart_str_appendc(&uri_str, '@');
    }

    if (uri_intern->host && ZSTR_LEN(uri_intern->host) > 0) {
        smart_str_append(&uri_str, uri_intern->host);
    }

    if (uri_intern->port != SIGNALFORGE_PORT_UNSET &&
        !signalforge_is_standard_port(uri_intern->scheme, uri_intern->port)) {
        smart_str_appendc(&uri_str, ':');
        smart_str_append_long(&uri_str, uri_intern->port);
    }

    /* Add path */
    if (uri_intern->path && ZSTR_LEN(uri_intern->path) > 0) {
        /* Ensure path starts with / if we have host */
        if (uri_intern->host && ZSTR_LEN(uri_intern->host) > 0 &&
            ZSTR_VAL(uri_intern->path)[0] != '/') {
            smart_str_appendc(&uri_str, '/');
        }
        smart_str_append(&uri_str, uri_intern->path);
    } else if (uri_intern->host && ZSTR_LEN(uri_intern->host) > 0) {
        /* Empty path with host - add / */
        smart_str_appendc(&uri_str, '/');
    }

    /* Add query */
    if (uri_intern->query && ZSTR_LEN(uri_intern->query) > 0) {
        smart_str_appendc(&uri_str, '?');
        smart_str_append(&uri_str, uri_intern->query);
    }

    /* Add fragment */
    if (uri_intern->fragment && ZSTR_LEN(uri_intern->fragment) > 0) {
        smart_str_appendc(&uri_str, '#');
        smart_str_append(&uri_str, uri_intern->fragment);
    }

    smart_str_0(&uri_str);

    /* Store URI string */
    if (uri_str.s) {
        ZVAL_STR(&intern->zv_uri, uri_str.s);
        intern->request_uri = Z_STRVAL(intern->zv_uri);
        intern->request_uri_len = Z_STRLEN(intern->zv_uri);
    } else {
        ZVAL_EMPTY_STRING(&intern->zv_uri);
        intern->request_uri = Z_STRVAL(intern->zv_uri);
        intern->request_uri_len = 0;
    }

    /* Initialize empty headers if not already done */
    if (!intern->ht_headers) {
        ALLOC_HASHTABLE(intern->ht_headers);
        zend_hash_init(intern->ht_headers, 16, NULL, ZVAL_PTR_DTOR, 0);
    }
    intern->flags |= SF_REQ_FLAG_HEADERS_EXTRACTED;

    /* Initialize empty attributes if not already done */
    if (!intern->ht_attributes) {
        ALLOC_HASHTABLE(intern->ht_attributes);
        zend_hash_init(intern->ht_attributes, 8, NULL, ZVAL_PTR_DTOR, 0);
    }

    /* Set protocol version */
    if (!intern->protocol_version) {
        intern->protocol_version = zend_string_init("1.1", 3, 0);
    }

    /* Create empty body Stream for client-side request
     * The getBody() method expects zv_body to be a Stream object,
     * not a raw string. Create an empty Stream using our helper. */
    zend_object *body_stream = signalforge_create_stream_from_string("", 0);
    ZVAL_OBJ(&intern->zv_body, body_stream);
    intern->flags |= SF_REQ_FLAG_BODY_READ;

    return Z_OBJ(request_zv);
}

/* ============================================================================
 * INTERNAL HELPER: CREATE RESPONSE
 * ============================================================================ */

zend_object *signalforge_create_response(
    zend_long status_code,
    const char *reason_phrase,
    size_t reason_len
)
{
    zval response_zv;
    signalforge_response_object *intern;

    /* Create new Response instance */
    object_init_ex(&response_zv, signalforge_response_ce);
    intern = Z_SIGNALFORGE_RESPONSE_P(&response_zv);

    /* Set status code */
    intern->status_code = status_code;

    /* Set reason phrase if provided */
    if (reason_phrase && reason_len > 0) {
        if (intern->reason_phrase) {
            zend_string_release(intern->reason_phrase);
        }
        intern->reason_phrase = zend_string_init(reason_phrase, reason_len, 0);
    }

    return Z_OBJ(response_zv);
}

/* ============================================================================
 * INTERNAL HELPER: CREATE STREAM FROM STRING
 * ============================================================================ */

zend_object *signalforge_create_stream_from_string(
    const char *content,
    size_t content_len
)
{
    zval stream_zv;
    signalforge_stream_object *intern;

    /* Create new Stream instance */
    object_init_ex(&stream_zv, signalforge_stream_ce);
    intern = Z_SIGNALFORGE_STREAM_P(&stream_zv);

    /* Create string data */
    intern->string_data = zend_string_init(content, content_len, 0);
    intern->position = 0;
    intern->size = content_len;
    intern->readable = 1;
    intern->writable = 0;
    intern->seekable = 1;

    return Z_OBJ(stream_zv);
}

/* ============================================================================
 * INTERNAL HELPER: CREATE STREAM FROM FILE
 * ============================================================================ */

zend_object *signalforge_create_stream_from_file(
    const char *filename,
    size_t filename_len,
    const char *mode,
    size_t mode_len
)
{
    zval stream_zv;
    signalforge_stream_object *intern;
    php_stream *stream;
    (void)filename_len;  /* Used implicitly by php_stream_open_wrapper */

    /* Open file stream */
    stream = php_stream_open_wrapper((char *)filename, (char *)mode, 0, NULL);
    if (!stream) {
        return NULL;
    }

    /* Create new Stream instance */
    object_init_ex(&stream_zv, signalforge_stream_ce);
    intern = Z_SIGNALFORGE_STREAM_P(&stream_zv);

    /* Create resource from php_stream */
    php_stream_to_zval(stream, &intern->zv_resource);

    /* Get position */
    intern->position = php_stream_tell(stream);

    /* Determine capabilities based on mode */
    intern->readable = (strchr(mode, 'r') != NULL || strchr(mode, '+') != NULL);
    intern->writable = (strchr(mode, 'w') != NULL || strchr(mode, 'a') != NULL ||
                       strchr(mode, 'x') != NULL || strchr(mode, 'c') != NULL ||
                       strchr(mode, '+') != NULL);
    intern->seekable = 1;

    /* Get size */
    php_stream_statbuf ssb;
    if (php_stream_stat(stream, &ssb) == 0) {
        intern->size = ssb.sb.st_size;
    } else {
        intern->size = 0;
    }

    return Z_OBJ(stream_zv);
}

/* ============================================================================
 * INTERNAL HELPER: CREATE STREAM FROM RESOURCE
 * ============================================================================ */

zend_object *signalforge_create_stream_from_resource(zval *resource)
{
    zval stream_zv;
    signalforge_stream_object *intern;
    php_stream *stream;

    /* Validate resource */
    if (Z_TYPE_P(resource) != IS_RESOURCE) {
        return NULL;
    }

    /* Get php_stream from resource */
    if (zend_fetch_resource2(Z_RES_P(resource), "stream", php_file_le_stream(), php_file_le_pstream()) == NULL) {
        return NULL;
    }
    php_stream_from_zval_no_verify(stream, resource);
    if (!stream) {
        return NULL;
    }

    /* Create new Stream instance */
    object_init_ex(&stream_zv, signalforge_stream_ce);
    intern = Z_SIGNALFORGE_STREAM_P(&stream_zv);

    /* Store resource reference */
    ZVAL_COPY(&intern->zv_resource, resource);
    intern->position = php_stream_tell(stream);

    /* Determine capabilities */
    php_stream_statbuf ssb;
    if (php_stream_stat(stream, &ssb) == 0) {
        intern->size = ssb.sb.st_size;
        intern->seekable = (ssb.sb.st_mode & S_IFREG) ? 1 : 0;
    } else {
        intern->size = 0;
        intern->seekable = 1;  /* Assume seekable for memory streams */
    }

    /* Check mode for read/write */
    intern->readable = stream->flags & PHP_STREAM_FLAG_NO_FCLOSE ? 1 : 1;  /* Assume readable */
    intern->writable = (stream->ops == &php_stream_memory_ops ||
                       stream->ops == &php_stream_temp_ops) ? 1 : 0;

    return Z_OBJ(stream_zv);
}

/* ============================================================================
 * URI FACTORY
 * ============================================================================ */

/* {{{ UriFactory::createUri(string $uri = ''): UriInterface */
PHP_METHOD(Signalforge_Http_UriFactory, createUri)
{
    zend_string *uri_str = NULL;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR(uri_str)
    ZEND_PARSE_PARAMETERS_END();

    if (!uri_str || ZSTR_LEN(uri_str) == 0) {
        /* Create empty URI */
        object_init_ex(return_value, signalforge_uri_ce);
    } else {
        /* Parse URI string */
        zend_object *uri_obj = signalforge_uri_create_from_string(ZSTR_VAL(uri_str), ZSTR_LEN(uri_str));
        ZVAL_OBJ(return_value, uri_obj);
    }
}
/* }}} */

/* ============================================================================
 * REQUEST FACTORY
 * ============================================================================ */

/* {{{ RequestFactory::createRequest(string $method, $uri): RequestInterface */
PHP_METHOD(Signalforge_Http_RequestFactory, createRequest)
{
    zend_string *method;
    zval *uri_arg;
    zend_object *uri_obj;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(method)
        Z_PARAM_ZVAL(uri_arg)
    ZEND_PARSE_PARAMETERS_END();

    /* Handle URI argument - can be string or UriInterface */
    if (Z_TYPE_P(uri_arg) == IS_STRING) {
        /* Create Uri from string */
        uri_obj = signalforge_uri_create_from_string(Z_STRVAL_P(uri_arg), Z_STRLEN_P(uri_arg));
    } else if (Z_TYPE_P(uri_arg) == IS_OBJECT) {
        /* Verify it's a UriInterface */
        if (!instanceof_function(Z_OBJCE_P(uri_arg), psr7_uri_interface_ce)) {
            zend_throw_exception(spl_ce_InvalidArgumentException,
                "URI must be a string or UriInterface instance", 0);
            RETURN_THROWS();
        }
        uri_obj = Z_OBJ_P(uri_arg);
        GC_ADDREF(uri_obj);
    } else {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "URI must be a string or UriInterface instance", 0);
        RETURN_THROWS();
    }

    /* Create client request */
    zend_object *request_obj = signalforge_create_client_request(
        ZSTR_VAL(method), ZSTR_LEN(method), uri_obj
    );

    /* Release extra reference from uri_obj (client_request added its own) */
    GC_DELREF(uri_obj);

    ZVAL_OBJ(return_value, request_obj);
}
/* }}} */

/* ============================================================================
 * RESPONSE FACTORY
 * ============================================================================ */

/* {{{ ResponseFactory::createResponse(int $code = 200, string $reasonPhrase = ''): ResponseInterface */
PHP_METHOD(Signalforge_Http_ResponseFactory, createResponse)
{
    zend_long code = 200;
    zend_string *reason = NULL;

    ZEND_PARSE_PARAMETERS_START(0, 2)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(code)
        Z_PARAM_STR(reason)
    ZEND_PARSE_PARAMETERS_END();

    /* Validate status code */
    if (code < 100 || code > 599) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Status code must be between 100 and 599", 0);
        RETURN_THROWS();
    }

    /* Create response */
    zend_object *response_obj = signalforge_create_response(
        code,
        reason ? ZSTR_VAL(reason) : NULL,
        reason ? ZSTR_LEN(reason) : 0
    );

    ZVAL_OBJ(return_value, response_obj);
}
/* }}} */

/* ============================================================================
 * STREAM FACTORY
 * ============================================================================ */

/* {{{ StreamFactory::createStream(string $content = ''): StreamInterface */
PHP_METHOD(Signalforge_Http_StreamFactory, createStream)
{
    zend_string *content = NULL;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR(content)
    ZEND_PARSE_PARAMETERS_END();

    zend_object *stream_obj = signalforge_create_stream_from_string(
        content ? ZSTR_VAL(content) : "",
        content ? ZSTR_LEN(content) : 0
    );

    ZVAL_OBJ(return_value, stream_obj);
}
/* }}} */

/* {{{ StreamFactory::createStreamFromFile(string $filename, string $mode = 'r'): StreamInterface */
PHP_METHOD(Signalforge_Http_StreamFactory, createStreamFromFile)
{
    zend_string *filename;
    zend_string *mode = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(filename)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR(mode)
    ZEND_PARSE_PARAMETERS_END();

    const char *mode_str = mode ? ZSTR_VAL(mode) : "r";
    size_t mode_len = mode ? ZSTR_LEN(mode) : 1;

    zend_object *stream_obj = signalforge_create_stream_from_file(
        ZSTR_VAL(filename), ZSTR_LEN(filename),
        mode_str, mode_len
    );

    if (!stream_obj) {
        zend_throw_exception(spl_ce_RuntimeException,
            "Failed to open file", 0);
        RETURN_THROWS();
    }

    ZVAL_OBJ(return_value, stream_obj);
}
/* }}} */

/* {{{ StreamFactory::createStreamFromResource($resource): StreamInterface */
PHP_METHOD(Signalforge_Http_StreamFactory, createStreamFromResource)
{
    zval *resource;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(resource)
    ZEND_PARSE_PARAMETERS_END();

    if (Z_TYPE_P(resource) != IS_RESOURCE) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Parameter must be a resource", 0);
        RETURN_THROWS();
    }

    zend_object *stream_obj = signalforge_create_stream_from_resource(resource);

    if (!stream_obj) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Resource is not a valid stream", 0);
        RETURN_THROWS();
    }

    ZVAL_OBJ(return_value, stream_obj);
}
/* }}} */

/* ============================================================================
 * SERVER REQUEST FACTORY
 * ============================================================================ */

/* {{{ ServerRequestFactory::createServerRequest(string $method, $uri, array $serverParams = []): ServerRequestInterface */
PHP_METHOD(Signalforge_Http_ServerRequestFactory, createServerRequest)
{
    zend_string *method;
    zval *uri_arg;
    zval *server_params = NULL;
    zend_object *uri_obj;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STR(method)
        Z_PARAM_ZVAL(uri_arg)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY(server_params)
    ZEND_PARSE_PARAMETERS_END();

    /* Handle URI argument - can be string or UriInterface */
    if (Z_TYPE_P(uri_arg) == IS_STRING) {
        uri_obj = signalforge_uri_create_from_string(Z_STRVAL_P(uri_arg), Z_STRLEN_P(uri_arg));
    } else if (Z_TYPE_P(uri_arg) == IS_OBJECT) {
        if (!instanceof_function(Z_OBJCE_P(uri_arg), psr7_uri_interface_ce)) {
            zend_throw_exception(spl_ce_InvalidArgumentException,
                "URI must be a string or UriInterface instance", 0);
            RETURN_THROWS();
        }
        uri_obj = Z_OBJ_P(uri_arg);
        GC_ADDREF(uri_obj);
    } else {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "URI must be a string or UriInterface instance", 0);
        RETURN_THROWS();
    }

    /* Create request (use same helper as RequestFactory) */
    zend_object *request_obj = signalforge_create_client_request(
        ZSTR_VAL(method), ZSTR_LEN(method), uri_obj
    );

    /* Release extra reference */
    GC_DELREF(uri_obj);

    /* If server params provided, store them */
    if (server_params && Z_TYPE_P(server_params) == IS_ARRAY) {
        signalforge_request_object *intern = signalforge_request_from_obj(request_obj);
        /* Store as reference to server params array */
        ZVAL_COPY(&intern->zv_server, server_params);
    }

    ZVAL_OBJ(return_value, request_obj);
}
/* }}} */

/* ============================================================================
 * UPLOADED FILE FACTORY
 * ============================================================================ */

/* {{{ UploadedFileFactory::createUploadedFile(...): UploadedFileInterface */
PHP_METHOD(Signalforge_Http_UploadedFileFactory, createUploadedFile)
{
    zval *stream;
    zend_long size = 0;
    zend_bool size_null = 1;
    zend_long error = PHP_UPLOAD_ERROR_OK;
    zend_string *client_filename = NULL;
    zend_string *client_media_type = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 5)
        Z_PARAM_ZVAL(stream)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG_OR_NULL(size, size_null)
        Z_PARAM_LONG(error)
        Z_PARAM_STR_OR_NULL(client_filename)
        Z_PARAM_STR_OR_NULL(client_media_type)
    ZEND_PARSE_PARAMETERS_END();

    /* Verify stream is a StreamInterface */
    if (Z_TYPE_P(stream) != IS_OBJECT ||
        !instanceof_function(Z_OBJCE_P(stream), psr7_stream_interface_ce)) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Stream must be a StreamInterface instance", 0);
        RETURN_THROWS();
    }

    /* Create new UploadedFile */
    object_init_ex(return_value, signalforge_uploadedfile_ce);

    /* Get the internal object and initialize it */
    /* Note: UploadedFile needs special initialization for factory use */
    /* For now, we use reflection-like approach through PHP */

    /* Store stream reference */
    zval *stream_prop;
    stream_prop = zend_read_property(signalforge_uploadedfile_ce, Z_OBJ_P(return_value),
                                     "stream", sizeof("stream")-1, 1, NULL);
    (void)stream_prop;

    /* This is a simplified implementation - full implementation would
       directly manipulate the uploadedfile_object structure */
    zend_throw_exception(spl_ce_RuntimeException,
        "UploadedFileFactory::createUploadedFile() not yet fully implemented", 0);
    RETURN_THROWS();
}
/* }}} */

/* ============================================================================
 * ARGINFO
 * ============================================================================ */

/* UriFactory */
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_uri_factory_createUri, 0, 0, Psr\\Http\\Message\\UriInterface, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, uri, IS_STRING, 0, "''")
ZEND_END_ARG_INFO()

/* RequestFactory */
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_request_factory_createRequest, 0, 2, Psr\\Http\\Message\\RequestInterface, 0)
    ZEND_ARG_TYPE_INFO(0, method, IS_STRING, 0)
    ZEND_ARG_INFO(0, uri)
ZEND_END_ARG_INFO()

/* ResponseFactory */
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_response_factory_createResponse, 0, 0, Psr\\Http\\Message\\ResponseInterface, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, code, IS_LONG, 0, "200")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, reasonPhrase, IS_STRING, 0, "''")
ZEND_END_ARG_INFO()

/* StreamFactory */
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_stream_factory_createStream, 0, 0, Psr\\Http\\Message\\StreamInterface, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, content, IS_STRING, 0, "''")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_stream_factory_createStreamFromFile, 0, 1, Psr\\Http\\Message\\StreamInterface, 0)
    ZEND_ARG_TYPE_INFO(0, filename, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, mode, IS_STRING, 0, "'r'")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_stream_factory_createStreamFromResource, 0, 1, Psr\\Http\\Message\\StreamInterface, 0)
    ZEND_ARG_INFO(0, resource)
ZEND_END_ARG_INFO()

/* ServerRequestFactory */
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_server_request_factory_createServerRequest, 0, 2, Psr\\Http\\Message\\ServerRequestInterface, 0)
    ZEND_ARG_TYPE_INFO(0, method, IS_STRING, 0)
    ZEND_ARG_INFO(0, uri)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, serverParams, IS_ARRAY, 0, "[]")
ZEND_END_ARG_INFO()

/* UploadedFileFactory */
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_uploaded_file_factory_createUploadedFile, 0, 1, Psr\\Http\\Message\\UploadedFileInterface, 0)
    ZEND_ARG_OBJ_INFO(0, stream, Psr\\Http\\Message\\StreamInterface, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, size, IS_LONG, 1, "null")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, error, IS_LONG, 0, "UPLOAD_ERR_OK")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, clientFilename, IS_STRING, 1, "null")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, clientMediaType, IS_STRING, 1, "null")
ZEND_END_ARG_INFO()

/* ============================================================================
 * METHOD TABLES
 * ============================================================================ */

static const zend_function_entry signalforge_uri_factory_methods[] = {
    PHP_ME(Signalforge_Http_UriFactory, createUri, arginfo_uri_factory_createUri, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

static const zend_function_entry signalforge_request_factory_methods[] = {
    PHP_ME(Signalforge_Http_RequestFactory, createRequest, arginfo_request_factory_createRequest, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

static const zend_function_entry signalforge_response_factory_methods[] = {
    PHP_ME(Signalforge_Http_ResponseFactory, createResponse, arginfo_response_factory_createResponse, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

static const zend_function_entry signalforge_stream_factory_methods[] = {
    PHP_ME(Signalforge_Http_StreamFactory, createStream, arginfo_stream_factory_createStream, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_StreamFactory, createStreamFromFile, arginfo_stream_factory_createStreamFromFile, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_StreamFactory, createStreamFromResource, arginfo_stream_factory_createStreamFromResource, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

static const zend_function_entry signalforge_server_request_factory_methods[] = {
    PHP_ME(Signalforge_Http_ServerRequestFactory, createServerRequest, arginfo_server_request_factory_createServerRequest, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

static const zend_function_entry signalforge_uploaded_file_factory_methods[] = {
    PHP_ME(Signalforge_Http_UploadedFileFactory, createUploadedFile, arginfo_uploaded_file_factory_createUploadedFile, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

/* ============================================================================
 * CLASS REGISTRATION
 * ============================================================================ */

void signalforge_factories_register_classes(void)
{
    zend_class_entry ce;

    /* UriFactory */
    INIT_NS_CLASS_ENTRY(ce, "Signalforge\\NativeHttp", "UriFactory", signalforge_uri_factory_methods);
    signalforge_uri_factory_ce = zend_register_internal_class(&ce);
    signalforge_uri_factory_ce->ce_flags |= ZEND_ACC_FINAL;
    zend_class_implements(signalforge_uri_factory_ce, 1, psr17_uri_factory_interface_ce);

    /* RequestFactory */
    INIT_NS_CLASS_ENTRY(ce, "Signalforge\\NativeHttp", "RequestFactory", signalforge_request_factory_methods);
    signalforge_request_factory_ce = zend_register_internal_class(&ce);
    signalforge_request_factory_ce->ce_flags |= ZEND_ACC_FINAL;
    zend_class_implements(signalforge_request_factory_ce, 1, psr17_request_factory_interface_ce);

    /* ResponseFactory */
    INIT_NS_CLASS_ENTRY(ce, "Signalforge\\NativeHttp", "ResponseFactory", signalforge_response_factory_methods);
    signalforge_response_factory_ce = zend_register_internal_class(&ce);
    signalforge_response_factory_ce->ce_flags |= ZEND_ACC_FINAL;
    zend_class_implements(signalforge_response_factory_ce, 1, psr17_response_factory_interface_ce);

    /* StreamFactory */
    INIT_NS_CLASS_ENTRY(ce, "Signalforge\\NativeHttp", "StreamFactory", signalforge_stream_factory_methods);
    signalforge_stream_factory_ce = zend_register_internal_class(&ce);
    signalforge_stream_factory_ce->ce_flags |= ZEND_ACC_FINAL;
    zend_class_implements(signalforge_stream_factory_ce, 1, psr17_stream_factory_interface_ce);

    /* ServerRequestFactory */
    INIT_NS_CLASS_ENTRY(ce, "Signalforge\\NativeHttp", "ServerRequestFactory", signalforge_server_request_factory_methods);
    signalforge_server_request_factory_ce = zend_register_internal_class(&ce);
    signalforge_server_request_factory_ce->ce_flags |= ZEND_ACC_FINAL;
    zend_class_implements(signalforge_server_request_factory_ce, 1, psr17_server_request_factory_interface_ce);

    /* UploadedFileFactory */
    INIT_NS_CLASS_ENTRY(ce, "Signalforge\\NativeHttp", "UploadedFileFactory", signalforge_uploaded_file_factory_methods);
    signalforge_uploaded_file_factory_ce = zend_register_internal_class(&ce);
    signalforge_uploaded_file_factory_ce->ce_flags |= ZEND_ACC_FINAL;
    zend_class_implements(signalforge_uploaded_file_factory_ce, 1, psr17_uploaded_file_factory_interface_ce);
}
