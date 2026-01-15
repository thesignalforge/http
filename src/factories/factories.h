/*
 * factories.h
 *
 * Signalforge HTTP PSR-17 Factory Classes
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

#ifndef SIGNALFORGE_FACTORIES_H
#define SIGNALFORGE_FACTORIES_H

#include "php_signalforge_http.h"

/* ============================================================================
 * CLASS ENTRIES
 * ============================================================================ */

extern zend_class_entry *signalforge_request_factory_ce;
extern zend_class_entry *signalforge_response_factory_ce;
extern zend_class_entry *signalforge_stream_factory_ce;
extern zend_class_entry *signalforge_uri_factory_ce;
extern zend_class_entry *signalforge_uploaded_file_factory_ce;
extern zend_class_entry *signalforge_server_request_factory_ce;

/* ============================================================================
 * REGISTRATION
 * ============================================================================ */

void signalforge_factories_register_classes(void);

/* ============================================================================
 * INTERNAL HELPERS
 *
 * These functions create PSR-7 objects for internal use (client, etc.)
 * They bypass the normal constructor paths for efficiency.
 * ============================================================================ */

/**
 * Create a Request for client-side use.
 * This creates a minimal request with method and URI set.
 * Use with* methods to add headers and body.
 *
 * @param method HTTP method (e.g., "GET", "POST")
 * @param method_len Length of method string
 * @param uri_obj Uri object (already created)
 * @return Request object
 */
zend_object *signalforge_create_client_request(
    const char *method,
    size_t method_len,
    zend_object *uri_obj
);

/**
 * Create a Response with status code and optional reason phrase.
 *
 * @param status_code HTTP status code
 * @param reason_phrase Optional reason phrase (NULL for default)
 * @param reason_len Length of reason phrase (0 if NULL)
 * @return Response object
 */
zend_object *signalforge_create_response(
    zend_long status_code,
    const char *reason_phrase,
    size_t reason_len
);

/**
 * Create a Stream from string content.
 *
 * @param content Content string (can be empty)
 * @param content_len Length of content
 * @return Stream object
 */
zend_object *signalforge_create_stream_from_string(
    const char *content,
    size_t content_len
);

/**
 * Create a Stream from file path.
 *
 * @param filename Path to file
 * @param filename_len Length of path
 * @param mode File mode (e.g., "r", "w")
 * @param mode_len Length of mode
 * @return Stream object
 */
zend_object *signalforge_create_stream_from_file(
    const char *filename,
    size_t filename_len,
    const char *mode,
    size_t mode_len
);

/**
 * Create a Stream from PHP resource.
 *
 * @param resource PHP stream resource zval
 * @return Stream object
 */
zend_object *signalforge_create_stream_from_resource(zval *resource);

#endif /* SIGNALFORGE_FACTORIES_H */
