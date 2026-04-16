/*
 * libFuzzer harness for signalforge_parse_uri().
 *
 * Purpose
 * -------
 * Fuzz the RFC 3986 URI parser that backs Signalforge\Http\Uri. The parser
 * is a pure C function that takes a (const char *, size_t) and fills a struct
 * of zend_string pointers. We exercise every field of the result to ensure
 * ASan catches any out-of-bounds reads or use-after-free on the stored
 * components.
 *
 * Build with: make
 * Run with:   make run   (or ./harness corpus/ -max_total_time=300)
 */

#include "../../fuzz-support/php_stubs.h"
#include <string.h>
#include <stdint.h>
#include <stddef.h>

/* ============================================================================
 * Declarations from uri_parser_extracted.c
 * ============================================================================ */

#define SIGNALFORGE_PORT_UNSET -1

typedef struct _signalforge_uri_object {
	zend_string *scheme;
	zend_string *user;
	zend_string *pass;
	zend_string *host;
	zend_long    port;
	zend_string *path;
	zend_string *query;
	zend_string *fragment;
} signalforge_uri_object;

int signalforge_parse_uri(const char *uri, size_t len, signalforge_uri_object *result);
void signalforge_uri_free_parts(signalforge_uri_object *obj);

/* ============================================================================
 * Touch every byte of a zend_string to detect OOB reads under ASan.
 * The volatile sink prevents the compiler from optimising the reads away.
 * ============================================================================ */

static volatile unsigned char g_sink;

static void touch_zstr(zend_string *zs)
{
	if (!zs) return;
	for (size_t i = 0; i < ZSTR_LEN(zs); i++) {
		g_sink ^= (unsigned char)ZSTR_VAL(zs)[i];
	}
}

/* ============================================================================
 * libFuzzer entry point
 * ============================================================================ */

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	/* The URI parser works on C strings (uses strtol which needs NUL).
	 * Create a NUL-terminated copy so we don't read past the fuzz input. */
	char *buf = (char *)malloc(size + 1);
	if (!buf) return 0;
	memcpy(buf, data, size);
	buf[size] = '\0';

	/* Zero-initialise so the parser's defensive "release old value" checks
	 * don't dereference garbage pointers on the first call. */
	signalforge_uri_object obj;
	memset(&obj, 0, sizeof(obj));
	obj.port = SIGNALFORGE_PORT_UNSET;

	(void)signalforge_parse_uri(buf, size, &obj);

	/* Touch every field regardless of return code. If the parser wrote
	 * partial results before returning FAILURE, we want ASan to catch
	 * any dangling pointers or short allocations. */
	touch_zstr(obj.scheme);
	touch_zstr(obj.user);
	touch_zstr(obj.pass);
	touch_zstr(obj.host);
	touch_zstr(obj.path);
	touch_zstr(obj.query);
	touch_zstr(obj.fragment);

	/* Verify port is in expected range (UBSan will catch signed overflow
	 * if strtol produces something unexpected). */
	g_sink ^= (unsigned char)(obj.port & 0xFF);

	signalforge_uri_free_parts(&obj);
	free(buf);

	return 0;
}
