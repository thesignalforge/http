/*
 * uploadedfile.h
 *
 * Signalforge HTTP UploadedFile class internal header
 */

#ifndef SIGNALFORGE_UPLOADEDFILE_H
#define SIGNALFORGE_UPLOADEDFILE_H

#include "php_signalforge_http.h"
#include "stream.h"

extern zend_class_entry *signalforge_uploadedfile_ce;

/* ============================================================================
 * INTERNAL OBJECT STRUCTURE
 * ============================================================================ */

typedef struct _signalforge_uploadedfile_object {
    /* File information from $_FILES or streamforge */
    zend_string *tmp_name;          // Temporary file path (OWNED)
    zend_string *client_filename;   // Original filename (OWNED, may be NULL)
    zend_string *client_media_type; // MIME type (OWNED, may be NULL)
    zend_long size;                 // File size in bytes
    zend_long error;                // UPLOAD_ERR_* constant

    /* Stream (lazy-loaded) */
    zval zv_stream;                 // StreamInterface object (lazy-loaded)
    bool stream_loaded;        // Flag to track if stream was created

    /* Streamforge integration */
    bool from_streamforge;     // True if file came from streamforge proxy
    int streamforge_index;          // Index in globals temp_paths array (-1 if N/A)

    /* Standard zend_object MUST be last member */
    zend_object std;
} signalforge_uploadedfile_object;

/* ============================================================================
 * OBJECT HELPERS
 * ============================================================================ */

/* Convert zend_object* to our internal struct */
static inline signalforge_uploadedfile_object *signalforge_uploadedfile_from_obj(zend_object *obj)
{
    return (signalforge_uploadedfile_object *)((char *)(obj) - XtOffsetOf(signalforge_uploadedfile_object, std));
}

/* Convenience macro for use in methods */
#define Z_SIGNALFORGE_UPLOADEDFILE_P(zv) signalforge_uploadedfile_from_obj(Z_OBJ_P(zv))

/* ============================================================================
 * INITIALIZATION FUNCTIONS
 * ============================================================================ */

void signalforge_uploadedfile_register_class(void);

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

signalforge_uploadedfile_object *signalforge_uploadedfile_from_files_array(zval *file_data, zval *return_value);

/* Create UploadedFile from streamforge HTTP_X_UPLOAD_* headers */
signalforge_uploadedfile_object *signalforge_uploadedfile_from_streamforge(
    HashTable *server_ht, int index, zval *return_value);

/* Mark streamforge upload as moved (prevents RSHUTDOWN cleanup) */
void signalforge_uploadedfile_mark_moved(signalforge_uploadedfile_object *intern);

#endif /* SIGNALFORGE_UPLOADEDFILE_H */

