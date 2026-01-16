/*
 * psr7_interfaces.c
 *
 * PSR-7, PSR-17, and PSR-18 Interface Implementation for Signalforge HTTP Extension
 *
 * This file implements PSR interfaces on the native classes for maximum
 * performance and to reduce dependencies on external PHP packages.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_signalforge_http.h"
#include "psr7_interfaces.h"

/* ============================================================================
 * PSR-7 INTERFACE CLASS ENTRIES (HTTP Message)
 * ============================================================================ */

zend_class_entry *psr7_message_interface_ce = NULL;
zend_class_entry *psr7_request_interface_ce = NULL;
zend_class_entry *psr7_response_interface_ce = NULL;
zend_class_entry *psr7_stream_interface_ce = NULL;
zend_class_entry *psr7_uploadedfile_interface_ce = NULL;
zend_class_entry *psr7_serverrequest_interface_ce = NULL;
zend_class_entry *psr7_uri_interface_ce = NULL;

/* ============================================================================
 * PSR-17 INTERFACE CLASS ENTRIES (HTTP Factories)
 * ============================================================================ */

zend_class_entry *psr17_request_factory_interface_ce = NULL;
zend_class_entry *psr17_response_factory_interface_ce = NULL;
zend_class_entry *psr17_stream_factory_interface_ce = NULL;
zend_class_entry *psr17_uploaded_file_factory_interface_ce = NULL;
zend_class_entry *psr17_uri_factory_interface_ce = NULL;
zend_class_entry *psr17_server_request_factory_interface_ce = NULL;

/* ============================================================================
 * PSR-18 INTERFACE CLASS ENTRIES (HTTP Client)
 * ============================================================================ */

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT
zend_class_entry *psr18_client_interface_ce = NULL;
zend_class_entry *psr18_client_exception_interface_ce = NULL;
zend_class_entry *psr18_network_exception_interface_ce = NULL;
zend_class_entry *psr18_request_exception_interface_ce = NULL;
#endif

/* ============================================================================
 * PSR-7 INTERFACE REGISTRATION
 * ============================================================================ */

void signalforge_register_psr7_interfaces(void)
{
    zend_class_entry ce;

    /* Register MessageInterface */
    INIT_NS_CLASS_ENTRY(ce, "Psr\\Http\\Message", "MessageInterface", NULL);
    psr7_message_interface_ce = zend_register_internal_interface(&ce);

    /* Register RequestInterface (extends MessageInterface) */
    INIT_NS_CLASS_ENTRY(ce, "Psr\\Http\\Message", "RequestInterface", NULL);
    psr7_request_interface_ce = zend_register_internal_interface(&ce);
    zend_class_implements(psr7_request_interface_ce, 1, psr7_message_interface_ce);

    /* Register ResponseInterface (extends MessageInterface) */
    INIT_NS_CLASS_ENTRY(ce, "Psr\\Http\\Message", "ResponseInterface", NULL);
    psr7_response_interface_ce = zend_register_internal_interface(&ce);
    zend_class_implements(psr7_response_interface_ce, 1, psr7_message_interface_ce);

    /* Register StreamInterface */
    INIT_NS_CLASS_ENTRY(ce, "Psr\\Http\\Message", "StreamInterface", NULL);
    psr7_stream_interface_ce = zend_register_internal_interface(&ce);

    /* Register UploadedFileInterface */
    INIT_NS_CLASS_ENTRY(ce, "Psr\\Http\\Message", "UploadedFileInterface", NULL);
    psr7_uploadedfile_interface_ce = zend_register_internal_interface(&ce);

    /* Register ServerRequestInterface (extends RequestInterface) */
    INIT_NS_CLASS_ENTRY(ce, "Psr\\Http\\Message", "ServerRequestInterface", NULL);
    psr7_serverrequest_interface_ce = zend_register_internal_interface(&ce);
    zend_class_implements(psr7_serverrequest_interface_ce, 1, psr7_request_interface_ce);

    /* Register UriInterface */
    INIT_NS_CLASS_ENTRY(ce, "Psr\\Http\\Message", "UriInterface", NULL);
    psr7_uri_interface_ce = zend_register_internal_interface(&ce);
}

/* ============================================================================
 * PSR-17 INTERFACE REGISTRATION (HTTP Factories)
 * ============================================================================ */

void signalforge_register_psr17_interfaces(void)
{
    zend_class_entry ce;

    /* Register RequestFactoryInterface */
    INIT_NS_CLASS_ENTRY(ce, "Psr\\Http\\Message", "RequestFactoryInterface", NULL);
    psr17_request_factory_interface_ce = zend_register_internal_interface(&ce);

    /* Register ResponseFactoryInterface */
    INIT_NS_CLASS_ENTRY(ce, "Psr\\Http\\Message", "ResponseFactoryInterface", NULL);
    psr17_response_factory_interface_ce = zend_register_internal_interface(&ce);

    /* Register StreamFactoryInterface */
    INIT_NS_CLASS_ENTRY(ce, "Psr\\Http\\Message", "StreamFactoryInterface", NULL);
    psr17_stream_factory_interface_ce = zend_register_internal_interface(&ce);

    /* Register UploadedFileFactoryInterface */
    INIT_NS_CLASS_ENTRY(ce, "Psr\\Http\\Message", "UploadedFileFactoryInterface", NULL);
    psr17_uploaded_file_factory_interface_ce = zend_register_internal_interface(&ce);

    /* Register UriFactoryInterface */
    INIT_NS_CLASS_ENTRY(ce, "Psr\\Http\\Message", "UriFactoryInterface", NULL);
    psr17_uri_factory_interface_ce = zend_register_internal_interface(&ce);

    /* Register ServerRequestFactoryInterface */
    INIT_NS_CLASS_ENTRY(ce, "Psr\\Http\\Message", "ServerRequestFactoryInterface", NULL);
    psr17_server_request_factory_interface_ce = zend_register_internal_interface(&ce);
}

/* ============================================================================
 * PSR-18 INTERFACE REGISTRATION (HTTP Client)
 * ============================================================================ */

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT

void signalforge_register_psr18_interfaces(void)
{
    zend_class_entry ce;

    /* Register ClientInterface */
    INIT_NS_CLASS_ENTRY(ce, "Psr\\Http\\Client", "ClientInterface", NULL);
    psr18_client_interface_ce = zend_register_internal_interface(&ce);

    /* Register ClientExceptionInterface (extends Throwable) */
    INIT_NS_CLASS_ENTRY(ce, "Psr\\Http\\Client", "ClientExceptionInterface", NULL);
    psr18_client_exception_interface_ce = zend_register_internal_interface(&ce);

    /* Register NetworkExceptionInterface (extends ClientExceptionInterface) */
    INIT_NS_CLASS_ENTRY(ce, "Psr\\Http\\Client", "NetworkExceptionInterface", NULL);
    psr18_network_exception_interface_ce = zend_register_internal_interface(&ce);
    zend_class_implements(psr18_network_exception_interface_ce, 1, psr18_client_exception_interface_ce);

    /* Register RequestExceptionInterface (extends ClientExceptionInterface) */
    INIT_NS_CLASS_ENTRY(ce, "Psr\\Http\\Client", "RequestExceptionInterface", NULL);
    psr18_request_exception_interface_ce = zend_register_internal_interface(&ce);
    zend_class_implements(psr18_request_exception_interface_ce, 1, psr18_client_exception_interface_ce);
}

#endif /* HAVE_SIGNALFORGE_HTTP_CLIENT */
