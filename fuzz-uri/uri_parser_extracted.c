/*
 * URI parser extracted from src/uri.c for standalone fuzzing.
 *
 * Why extract?
 * ------------
 * src/uri.c includes uri.h -> php_signalforge_http.h -> php.h, which pulls
 * in the entire Zend Engine, plus PSR-7 interfaces and SPL exceptions.
 * The URI parser itself is pure C: it takes a (const char *, size_t) and
 * fills a struct of zend_string pointers. Nothing else from the runtime
 * is touched.
 *
 * This file is a verbatim copy of signalforge_parse_uri() and its helper
 * functions from src/uri.c. Keep in sync when the upstream implementation
 * changes - the fuzz harness is only useful if it exercises the same code
 * the production extension runs.
 *
 * The signalforge_uri_object struct is duplicated here (minus the trailing
 * zend_object member, which is PHP object machinery) so the extracted
 * code compiles against the fuzz-support shim without any PHP headers.
 */

#include "../../fuzz-support/php_stubs.h"
#include <ctype.h>

/* ============================================================================
 * Zend macros not covered by the shim
 * ============================================================================ */

/* ZEND_STRTOL is just strtol on POSIX (strtoll on Win64). */
#ifndef ZEND_STRTOL
#define ZEND_STRTOL strtol
#endif

/* ============================================================================
 * Struct definition — mirrors src/uri.h minus the zend_object tail.
 * ============================================================================ */

#define SIGNALFORGE_PORT_UNSET  -1

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

/* ============================================================================
 * Forward declarations
 * ============================================================================ */

int signalforge_parse_uri(const char *uri, size_t len, signalforge_uri_object *result);
void signalforge_uri_free_parts(signalforge_uri_object *obj);

/* ============================================================================
 * Begin verbatim copy from src/uri.c
 * ============================================================================ */

/* Helper: Convert string to lowercase in-place */
static void str_tolower(char *str, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		str[i] = tolower((unsigned char)str[i]);
	}
}

/*
 * Helper: Create a lowercase zend_string from source data.
 *
 * Optimized single-allocation path: allocates the zend_string directly
 * and lowercases in-place, avoiding the emalloc+zend_string_init+efree
 * pattern that would require two allocations.
 *
 * @param src Source string data (not null-terminated required)
 * @param len Length of source data
 * @return New zend_string with lowercase content (caller owns)
 */
static zend_string *zend_string_init_lowercase(const char *src, size_t len)
{
	zend_string *result = zend_string_alloc(len, 0);
	memcpy(ZSTR_VAL(result), src, len);
	ZSTR_VAL(result)[len] = '\0';
	str_tolower(ZSTR_VAL(result), len);
	return result;
}

/* Helper: Check if character is unreserved per RFC 3986 */
static inline int is_unreserved(unsigned char c)
{
	return isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~';
}

/* Helper: Check if character is sub-delimiter per RFC 3986 */
static inline int is_subdelim(unsigned char c)
{
	return c == '!' || c == '$' || c == '&' || c == '\'' ||
	       c == '(' || c == ')' || c == '*' || c == '+' ||
	       c == ',' || c == ';' || c == '=';
}

/* Parse URI into components. Returns SUCCESS or FAILURE */
int signalforge_parse_uri(const char *uri, size_t len, signalforge_uri_object *result)
{
	const char *p = uri;
	const char *end = uri + len;
	const char *scheme_end = NULL;
	const char *authority_start = NULL;
	const char *authority_end = NULL;
	const char *path_start = NULL;
	const char *query_start = NULL;
	const char *fragment_start = NULL;

	/* Find scheme (ends at first ':') */
	for (const char *s = p; s < end; s++) {
		if (*s == ':') {
			/* Validate scheme: must start with alpha, contain alpha/digit/+/-/. */
			if (s > p && isalpha((unsigned char)*p)) {
				int valid = 1;
				for (const char *c = p + 1; c < s; c++) {
					if (!isalnum((unsigned char)*c) && *c != '+' && *c != '-' && *c != '.') {
						valid = 0;
						break;
					}
				}
				if (valid) {
					scheme_end = s;
					p = s + 1;
				}
			}
			break;
		}
		/* No valid scheme character */
		if (!isalnum((unsigned char)*s) && *s != '+' && *s != '-' && *s != '.') {
			break;
		}
	}

	/* Check for authority (starts with "//") */
	if (p + 1 < end && p[0] == '/' && p[1] == '/') {
		p += 2;
		authority_start = p;

		/* Find end of authority (next / ? or #) */
		authority_end = p;
		while (authority_end < end && *authority_end != '/' && *authority_end != '?' && *authority_end != '#') {
			authority_end++;
		}
		p = authority_end;
	}

	/* Path starts here */
	path_start = p;

	/* Find query (starts with ?) */
	while (p < end && *p != '?' && *p != '#') {
		p++;
	}
	const char *path_end = p;

	if (p < end && *p == '?') {
		p++;
		query_start = p;
		while (p < end && *p != '#') {
			p++;
		}
	}
	const char *query_end = p;

	/* Find fragment (starts with #) */
	if (p < end && *p == '#') {
		p++;
		fragment_start = p;
	}

	/* Store scheme (lowercase) - single allocation path */
	if (result->scheme) {
		zend_string_release(result->scheme);
	}
	if (scheme_end) {
		size_t scheme_len = scheme_end - uri;
		result->scheme = zend_string_init_lowercase(uri, scheme_len);
	} else {
		result->scheme = NULL;  /* Getters return empty string for NULL */
	}

	/* Parse authority if present */
	if (authority_start && authority_end > authority_start) {
		const char *auth = authority_start;
		size_t auth_len = authority_end - authority_start;

		/* Check for userinfo (contains @) */
		const char *at = NULL;
		for (const char *c = auth; c < authority_end; c++) {
			if (*c == '@') {
				at = c;
				break;
			}
		}

		if (at) {
			/* Parse userinfo: user[:pass] */
			const char *colon = NULL;
			for (const char *c = auth; c < at; c++) {
				if (*c == ':') {
					colon = c;
					break;
				}
			}

			/* Release existing user/pass if present (defensive) */
			if (result->user) {
				zend_string_release(result->user);
			}
			if (result->pass) {
				zend_string_release(result->pass);
			}

			if (colon) {
				result->user = zend_string_init(auth, colon - auth, 0);
				result->pass = zend_string_init(colon + 1, at - colon - 1, 0);
			} else {
				result->user = zend_string_init(auth, at - auth, 0);
				result->pass = NULL;
			}
			auth = at + 1;
		} else {
			/* Release existing user/pass if present (defensive) */
			if (result->user) {
				zend_string_release(result->user);
			}
			if (result->pass) {
				zend_string_release(result->pass);
			}
			result->user = NULL;
			result->pass = NULL;
		}

		/* Parse host[:port] */
		const char *host_start = auth;
		const char *host_end = authority_end;
		const char *port_start = NULL;

		/* Handle IPv6 addresses: [::1] */
		if (host_start < host_end && *host_start == '[') {
			/* IPv6 literal */
			const char *bracket = NULL;
			for (const char *c = host_start + 1; c < host_end; c++) {
				if (*c == ']') {
					bracket = c;
					break;
				}
			}
			if (bracket) {
				host_end = bracket + 1;
				if (host_end < authority_end && *host_end == ':') {
					port_start = host_end + 1;
				}
			} else {
				/* Unclosed '[' — reject as invalid IPv6 rather than
				 * silently storing "[::1" as the host. (audit H-H-ipv6) */
				result->host = ZSTR_EMPTY_ALLOC();
				result->port = SIGNALFORGE_PORT_UNSET;
				host_start = host_end; /* skip host assignment below */
			}
		} else {
			/* Regular host - find last colon for port */
			for (const char *c = authority_end - 1; c >= host_start; c--) {
				if (*c == ':') {
					port_start = c + 1;
					host_end = c;
					break;
				}
			}
		}

		/* Store host (lowercase) - single allocation path */
		if (result->host) {
			zend_string_release(result->host);
		}
		if (host_end > host_start) {
			size_t host_len = host_end - host_start;
			result->host = zend_string_init_lowercase(host_start, host_len);
		} else {
			result->host = NULL;  /* Getters return empty string for NULL */
		}

		/* Parse port */
		if (port_start && port_start < authority_end) {
			char *port_end_ptr;
			zend_long port_val = ZEND_STRTOL(port_start, &port_end_ptr, 10);
			if (port_end_ptr > port_start && port_val >= 0 && port_val <= 65535) {
				result->port = port_val;
			} else {
				result->port = SIGNALFORGE_PORT_UNSET;
			}
		} else {
			result->port = SIGNALFORGE_PORT_UNSET;
		}
	} else {
		/* No authority - defensive release */
		if (result->user) {
			zend_string_release(result->user);
		}
		if (result->pass) {
			zend_string_release(result->pass);
		}
		if (result->host) {
			zend_string_release(result->host);
		}
		result->user = NULL;
		result->pass = NULL;
		result->host = NULL;  /* Getters return empty string for NULL */
		result->port = SIGNALFORGE_PORT_UNSET;
	}

	/* Store path */
	if (result->path) {
		zend_string_release(result->path);
	}
	if (path_end > path_start) {
		result->path = zend_string_init(path_start, path_end - path_start, 0);
	} else {
		result->path = NULL  /* Getters return empty string for NULL */;
	}

	/* Store query (without ?) */
	if (result->query) {
		zend_string_release(result->query);
	}
	if (query_start && query_end > query_start) {
		result->query = zend_string_init(query_start, query_end - query_start, 0);
	} else {
		result->query = NULL  /* Getters return empty string for NULL */;
	}

	/* Store fragment (without #) */
	if (result->fragment) {
		zend_string_release(result->fragment);
	}
	if (fragment_start && end > fragment_start) {
		result->fragment = zend_string_init(fragment_start, end - fragment_start, 0);
	} else {
		result->fragment = NULL  /* Getters return empty string for NULL */;
	}

	return SUCCESS;
}

/* ---- End verbatim copy from src/uri.c ---- */

/* ============================================================================
 * Cleanup helper — mirrors signalforge_uri_free_obj() logic without the
 * zend_object_std_dtor() call (no zend_object in our extracted struct).
 * ============================================================================ */

void signalforge_uri_free_parts(signalforge_uri_object *obj)
{
	if (!obj) return;
	if (obj->scheme)   zend_string_release(obj->scheme);
	if (obj->user)     zend_string_release(obj->user);
	if (obj->pass)     zend_string_release(obj->pass);
	if (obj->host)     zend_string_release(obj->host);
	if (obj->path)     zend_string_release(obj->path);
	if (obj->query)    zend_string_release(obj->query);
	if (obj->fragment) zend_string_release(obj->fragment);
}
