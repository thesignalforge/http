/*
 * psr7_interfaces.c
 *
 * PSR-7 Interface Implementation for Signalforge HTTP Extension
 *
 * This file implements PSR-7 interfaces on the native classes for maximum
 * performance and to reduce dependencies on the PHP wrapper classes.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_signalforge_http.h"
#include "psr7_interfaces.h"

/* ============================================================================
 * PSR-7 INTERFACE CLASS ENTRIES (from external packages)
 * ============================================================================ */

zend_class_entry *psr7_message_interface_ce = NULL;
zend_class_entry *psr7_request_interface_ce = NULL;
zend_class_entry *psr7_response_interface_ce = NULL;
zend_class_entry *psr7_stream_interface_ce = NULL;
zend_class_entry *psr7_uploadedfile_interface_ce = NULL;
zend_class_entry *psr7_serverrequest_interface_ce = NULL;

/* ============================================================================
 * INTERFACE LOOKUP AND IMPLEMENTATION
 * ============================================================================ */

void signalforge_register_psr7_interfaces(void)
{
    /* Define PSR-7 interfaces directly in the extension for maximum performance */
    /* This avoids dependency on external PSR packages */

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
}
