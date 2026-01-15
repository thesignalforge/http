/*
 * psr7_interfaces.h
 *
 * PSR-7, PSR-17, and PSR-18 Interface Definitions Header
 */

#ifndef SIGNALFORGE_PSR7_INTERFACES_H
#define SIGNALFORGE_PSR7_INTERFACES_H

#include "php_signalforge_http.h"

/* ============================================================================
 * PSR-7 INTERFACE CLASS ENTRIES (HTTP Message)
 * ============================================================================ */

extern zend_class_entry *psr7_message_interface_ce;
extern zend_class_entry *psr7_request_interface_ce;
extern zend_class_entry *psr7_response_interface_ce;
extern zend_class_entry *psr7_stream_interface_ce;
extern zend_class_entry *psr7_uploadedfile_interface_ce;
extern zend_class_entry *psr7_serverrequest_interface_ce;
extern zend_class_entry *psr7_uri_interface_ce;

/* ============================================================================
 * PSR-17 INTERFACE CLASS ENTRIES (HTTP Factories)
 * ============================================================================ */

extern zend_class_entry *psr17_request_factory_interface_ce;
extern zend_class_entry *psr17_response_factory_interface_ce;
extern zend_class_entry *psr17_stream_factory_interface_ce;
extern zend_class_entry *psr17_uploaded_file_factory_interface_ce;
extern zend_class_entry *psr17_uri_factory_interface_ce;
extern zend_class_entry *psr17_server_request_factory_interface_ce;

/* ============================================================================
 * PSR-18 INTERFACE CLASS ENTRIES (HTTP Client)
 * ============================================================================ */

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT
extern zend_class_entry *psr18_client_interface_ce;
extern zend_class_entry *psr18_client_exception_interface_ce;
extern zend_class_entry *psr18_network_exception_interface_ce;
extern zend_class_entry *psr18_request_exception_interface_ce;
#endif

/* ============================================================================
 * INTERFACE REGISTRATION FUNCTIONS
 * ============================================================================ */

void signalforge_register_psr7_interfaces(void);
void signalforge_register_psr17_interfaces(void);

#ifdef HAVE_SIGNALFORGE_HTTP_CLIENT
void signalforge_register_psr18_interfaces(void);
#endif

#endif /* SIGNALFORGE_PSR7_INTERFACES_H */
