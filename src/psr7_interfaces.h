/*
 * psr7_interfaces.h
 *
 * PSR-7 Interface Definitions Header
 */

#ifndef SIGNALFORGE_PSR7_INTERFACES_H
#define SIGNALFORGE_PSR7_INTERFACES_H

#include "php_signalforge_http.h"

/* ============================================================================
 * PSR-7 INTERFACE CLASS ENTRIES
 * ============================================================================ */

extern zend_class_entry *psr7_message_interface_ce;
extern zend_class_entry *psr7_request_interface_ce;
extern zend_class_entry *psr7_response_interface_ce;
extern zend_class_entry *psr7_stream_interface_ce;
extern zend_class_entry *psr7_uploadedfile_interface_ce;
extern zend_class_entry *psr7_serverrequest_interface_ce;

/* ============================================================================
 * INTERFACE REGISTRATION FUNCTION
 * ============================================================================ */

void signalforge_register_psr7_interfaces(void);

#endif /* SIGNALFORGE_PSR7_INTERFACES_H */
