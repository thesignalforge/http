/*
 * uri.c
 *
 * Signalforge HTTP Uri class implementation
 * Implements PSR-7 UriInterface with RFC 3986 compliant parsing
 *
 * Copyright (c) 2024 Signalforge
 * License: MIT
 */

#include "uri.h"
#include "psr7_interfaces.h"
#include "ext/spl/spl_exceptions.h"
#include "zend_smart_str.h"
#include <ctype.h>

/* ============================================================================
 * CLASS ENTRY AND HANDLERS
 * ============================================================================ */

zend_class_entry *signalforge_uri_ce = NULL;
static zend_object_handlers signalforge_uri_handlers;

/* ============================================================================
 * RFC 3986 URI PARSER
 * ============================================================================ */

/*
 * RFC 3986 URI syntax:
 * URI = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
 * hier-part = "//" authority path-abempty / path-absolute / path-rootless / path-empty
 * authority = [ userinfo "@" ] host [ ":" port ]
 * userinfo = *( unreserved / pct-encoded / sub-delims / ":" )
 */

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

/* ============================================================================
 * OBJECT HANDLERS
 * ============================================================================ */

static void signalforge_uri_free_obj(zend_object *object)
{
    signalforge_uri_object *intern = signalforge_uri_from_obj(object);

    if (intern->scheme) {
        zend_string_release(intern->scheme);
    }
    if (intern->user) {
        zend_string_release(intern->user);
    }
    if (intern->pass) {
        zend_string_release(intern->pass);
    }
    if (intern->host) {
        zend_string_release(intern->host);
    }
    if (intern->path) {
        zend_string_release(intern->path);
    }
    if (intern->query) {
        zend_string_release(intern->query);
    }
    if (intern->fragment) {
        zend_string_release(intern->fragment);
    }

    zend_object_std_dtor(&intern->std);
}

static zend_object *signalforge_uri_create_object(zend_class_entry *ce)
{
    signalforge_uri_object *intern = zend_object_alloc(sizeof(signalforge_uri_object), ce);

    /* Initialize all fields to NULL/unset - no allocations needed */
    intern->scheme = NULL;
    intern->user = NULL;
    intern->pass = NULL;
    intern->host = NULL;
    intern->port = SIGNALFORGE_PORT_UNSET;
    intern->path = NULL;
    intern->query = NULL;
    intern->fragment = NULL;

    zend_object_std_init(&intern->std, ce);
    object_properties_init(&intern->std, ce);
    intern->std.handlers = &signalforge_uri_handlers;

    return &intern->std;
}

static zend_object *signalforge_uri_clone_obj(zend_object *object)
{
    signalforge_uri_object *old_intern = signalforge_uri_from_obj(object);
    zend_object *new_obj = signalforge_uri_create_object(object->ce);
    signalforge_uri_object *new_intern = signalforge_uri_from_obj(new_obj);

    /* Copy all fields - no release needed, create_object initializes to NULL */
    new_intern->scheme = old_intern->scheme ? zend_string_copy(old_intern->scheme) : NULL;
    new_intern->user = old_intern->user ? zend_string_copy(old_intern->user) : NULL;
    new_intern->pass = old_intern->pass ? zend_string_copy(old_intern->pass) : NULL;
    new_intern->host = old_intern->host ? zend_string_copy(old_intern->host) : NULL;
    new_intern->port = old_intern->port;
    new_intern->path = old_intern->path ? zend_string_copy(old_intern->path) : NULL;
    new_intern->query = old_intern->query ? zend_string_copy(old_intern->query) : NULL;
    new_intern->fragment = old_intern->fragment ? zend_string_copy(old_intern->fragment) : NULL;

    zend_objects_clone_members(new_obj, object);

    return new_obj;
}

/* Clone helper for with* methods */
signalforge_uri_object *signalforge_uri_clone(signalforge_uri_object *src, zval *return_value)
{
    object_init_ex(return_value, signalforge_uri_ce);
    signalforge_uri_object *dst = Z_SIGNALFORGE_URI_P(return_value);

    /* Copy from source - no release needed, create_object initializes to NULL */
    dst->scheme = src->scheme ? zend_string_copy(src->scheme) : NULL;
    dst->user = src->user ? zend_string_copy(src->user) : NULL;
    dst->pass = src->pass ? zend_string_copy(src->pass) : NULL;
    dst->host = src->host ? zend_string_copy(src->host) : NULL;
    dst->port = src->port;
    dst->path = src->path ? zend_string_copy(src->path) : NULL;
    dst->query = src->query ? zend_string_copy(src->query) : NULL;
    dst->fragment = src->fragment ? zend_string_copy(src->fragment) : NULL;

    return dst;
}

/* ============================================================================
 * PHP METHODS - GETTERS
 * ============================================================================ */

/* {{{ getScheme(): string */
PHP_METHOD(Signalforge_Http_Uri, getScheme)
{
    signalforge_uri_object *intern = Z_SIGNALFORGE_URI_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();

    if (intern->scheme) {
        RETURN_STR_COPY(intern->scheme);
    }
    RETURN_EMPTY_STRING();
}
/* }}} */

/* {{{ getUserInfo(): string */
PHP_METHOD(Signalforge_Http_Uri, getUserInfo)
{
    signalforge_uri_object *intern = Z_SIGNALFORGE_URI_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();

    if (!intern->user || ZSTR_LEN(intern->user) == 0) {
        RETURN_EMPTY_STRING();
    }

    if (intern->pass && ZSTR_LEN(intern->pass) > 0) {
        /* user:pass */
        size_t len = ZSTR_LEN(intern->user) + 1 + ZSTR_LEN(intern->pass);
        zend_string *result = zend_string_alloc(len, 0);
        memcpy(ZSTR_VAL(result), ZSTR_VAL(intern->user), ZSTR_LEN(intern->user));
        ZSTR_VAL(result)[ZSTR_LEN(intern->user)] = ':';
        memcpy(ZSTR_VAL(result) + ZSTR_LEN(intern->user) + 1, ZSTR_VAL(intern->pass), ZSTR_LEN(intern->pass));
        ZSTR_VAL(result)[len] = '\0';
        RETURN_NEW_STR(result);
    }

    RETURN_STR_COPY(intern->user);
}
/* }}} */

/* {{{ getHost(): string */
PHP_METHOD(Signalforge_Http_Uri, getHost)
{
    signalforge_uri_object *intern = Z_SIGNALFORGE_URI_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();

    if (intern->host) {
        RETURN_STR_COPY(intern->host);
    }
    RETURN_EMPTY_STRING();
}
/* }}} */

/* {{{ getPort(): ?int */
PHP_METHOD(Signalforge_Http_Uri, getPort)
{
    signalforge_uri_object *intern = Z_SIGNALFORGE_URI_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();

    /* Return null if port is not set or is standard for the scheme */
    if (intern->port == SIGNALFORGE_PORT_UNSET ||
        signalforge_is_standard_port(intern->scheme, intern->port)) {
        RETURN_NULL();
    }

    RETURN_LONG(intern->port);
}
/* }}} */

/* {{{ getPath(): string */
PHP_METHOD(Signalforge_Http_Uri, getPath)
{
    signalforge_uri_object *intern = Z_SIGNALFORGE_URI_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();

    if (intern->path) {
        RETURN_STR_COPY(intern->path);
    }
    RETURN_EMPTY_STRING();
}
/* }}} */

/* {{{ getQuery(): string */
PHP_METHOD(Signalforge_Http_Uri, getQuery)
{
    signalforge_uri_object *intern = Z_SIGNALFORGE_URI_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();

    if (intern->query) {
        RETURN_STR_COPY(intern->query);
    }
    RETURN_EMPTY_STRING();
}
/* }}} */

/* {{{ getFragment(): string */
PHP_METHOD(Signalforge_Http_Uri, getFragment)
{
    signalforge_uri_object *intern = Z_SIGNALFORGE_URI_P(ZEND_THIS);
    ZEND_PARSE_PARAMETERS_NONE();

    if (intern->fragment) {
        RETURN_STR_COPY(intern->fragment);
    }
    RETURN_EMPTY_STRING();
}
/* }}} */

/* {{{ getAuthority(): string */
PHP_METHOD(Signalforge_Http_Uri, getAuthority)
{
    signalforge_uri_object *intern = Z_SIGNALFORGE_URI_P(ZEND_THIS);
    smart_str buf = {0};
    ZEND_PARSE_PARAMETERS_NONE();

    /* No host = no authority */
    if (!intern->host || ZSTR_LEN(intern->host) == 0) {
        RETURN_EMPTY_STRING();
    }

    /* Add userinfo if present */
    if (intern->user && ZSTR_LEN(intern->user) > 0) {
        smart_str_append(&buf, intern->user);
        if (intern->pass && ZSTR_LEN(intern->pass) > 0) {
            smart_str_appendc(&buf, ':');
            smart_str_append(&buf, intern->pass);
        }
        smart_str_appendc(&buf, '@');
    }

    /* Add host */
    smart_str_append(&buf, intern->host);

    /* Add port if non-standard */
    if (intern->port != SIGNALFORGE_PORT_UNSET &&
        !signalforge_is_standard_port(intern->scheme, intern->port)) {
        smart_str_appendc(&buf, ':');
        smart_str_append_long(&buf, intern->port);
    }

    smart_str_0(&buf);
    if (buf.s) {
        RETURN_NEW_STR(buf.s);
    }
    RETURN_EMPTY_STRING();
}
/* }}} */

/* {{{ __toString(): string */
PHP_METHOD(Signalforge_Http_Uri, __toString)
{
    signalforge_uri_object *intern = Z_SIGNALFORGE_URI_P(ZEND_THIS);
    smart_str buf = {0};
    ZEND_PARSE_PARAMETERS_NONE();

    /* RFC 3986 Section 5.3: Component Recomposition */

    /* Scheme */
    if (intern->scheme && ZSTR_LEN(intern->scheme) > 0) {
        smart_str_append(&buf, intern->scheme);
        smart_str_appendc(&buf, ':');
    }

    /* Authority */
    if (intern->host && ZSTR_LEN(intern->host) > 0) {
        smart_str_appendl(&buf, "//", 2);

        /* Userinfo */
        if (intern->user && ZSTR_LEN(intern->user) > 0) {
            smart_str_append(&buf, intern->user);
            if (intern->pass && ZSTR_LEN(intern->pass) > 0) {
                smart_str_appendc(&buf, ':');
                smart_str_append(&buf, intern->pass);
            }
            smart_str_appendc(&buf, '@');
        }

        /* Host */
        smart_str_append(&buf, intern->host);

        /* Port */
        if (intern->port != SIGNALFORGE_PORT_UNSET &&
            !signalforge_is_standard_port(intern->scheme, intern->port)) {
            smart_str_appendc(&buf, ':');
            smart_str_append_long(&buf, intern->port);
        }
    }

    /* Path */
    if (intern->path && ZSTR_LEN(intern->path) > 0) {
        /* If authority present, path must start with / or be empty */
        if (intern->host && ZSTR_LEN(intern->host) > 0 &&
            ZSTR_VAL(intern->path)[0] != '/') {
            smart_str_appendc(&buf, '/');
        }
        smart_str_append(&buf, intern->path);
    }

    /* Query */
    if (intern->query && ZSTR_LEN(intern->query) > 0) {
        smart_str_appendc(&buf, '?');
        smart_str_append(&buf, intern->query);
    }

    /* Fragment */
    if (intern->fragment && ZSTR_LEN(intern->fragment) > 0) {
        smart_str_appendc(&buf, '#');
        smart_str_append(&buf, intern->fragment);
    }

    smart_str_0(&buf);
    if (buf.s) {
        RETURN_NEW_STR(buf.s);
    }
    RETURN_EMPTY_STRING();
}
/* }}} */

/* ============================================================================
 * PHP METHODS - IMMUTABLE MODIFIERS
 * ============================================================================ */

/* {{{ withScheme(string $scheme): static */
PHP_METHOD(Signalforge_Http_Uri, withScheme)
{
    zend_string *scheme;
    signalforge_uri_object *src, *dst;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(scheme)
    ZEND_PARSE_PARAMETERS_END();

    src = Z_SIGNALFORGE_URI_P(ZEND_THIS);
    dst = signalforge_uri_clone(src, return_value);

    /* Release old and set new */
    if (dst->scheme) {
        zend_string_release(dst->scheme);
    }

    /* Lowercase the scheme - single allocation path */
    if (ZSTR_LEN(scheme) > 0) {
        dst->scheme = zend_string_init_lowercase(ZSTR_VAL(scheme), ZSTR_LEN(scheme));
    } else {
        dst->scheme = NULL  /* Getters return empty string for NULL */;
    }
}
/* }}} */

/* {{{ withUserInfo(string $user, ?string $password = null): static */
PHP_METHOD(Signalforge_Http_Uri, withUserInfo)
{
    zend_string *user;
    zend_string *password = NULL;
    signalforge_uri_object *src, *dst;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(user)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(password)
    ZEND_PARSE_PARAMETERS_END();

    src = Z_SIGNALFORGE_URI_P(ZEND_THIS);
    dst = signalforge_uri_clone(src, return_value);

    /* Release old */
    if (dst->user) {
        zend_string_release(dst->user);
        dst->user = NULL;
    }
    if (dst->pass) {
        zend_string_release(dst->pass);
        dst->pass = NULL;
    }

    /* Set new */
    if (ZSTR_LEN(user) > 0) {
        dst->user = zend_string_copy(user);
        if (password && ZSTR_LEN(password) > 0) {
            dst->pass = zend_string_copy(password);
        }
    }
}
/* }}} */

/* {{{ withHost(string $host): static */
PHP_METHOD(Signalforge_Http_Uri, withHost)
{
    zend_string *host;
    signalforge_uri_object *src, *dst;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(host)
    ZEND_PARSE_PARAMETERS_END();

    src = Z_SIGNALFORGE_URI_P(ZEND_THIS);
    dst = signalforge_uri_clone(src, return_value);

    /* Release old */
    if (dst->host) {
        zend_string_release(dst->host);
    }

    /* Lowercase the host - single allocation path */
    if (ZSTR_LEN(host) > 0) {
        dst->host = zend_string_init_lowercase(ZSTR_VAL(host), ZSTR_LEN(host));
    } else {
        dst->host = NULL  /* Getters return empty string for NULL */;
    }
}
/* }}} */

/* {{{ withPort(?int $port): static */
PHP_METHOD(Signalforge_Http_Uri, withPort)
{
    zend_long port;
    bool port_is_null = 0;
    signalforge_uri_object *src, *dst;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG_OR_NULL(port, port_is_null)
    ZEND_PARSE_PARAMETERS_END();

    if (!port_is_null && (port < 0 || port > 65535)) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Port must be between 0 and 65535", 0);
        RETURN_THROWS();
    }

    src = Z_SIGNALFORGE_URI_P(ZEND_THIS);
    dst = signalforge_uri_clone(src, return_value);

    dst->port = port_is_null ? SIGNALFORGE_PORT_UNSET : port;
}
/* }}} */

/* {{{ withPath(string $path): static */
PHP_METHOD(Signalforge_Http_Uri, withPath)
{
    zend_string *path;
    signalforge_uri_object *src, *dst;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(path)
    ZEND_PARSE_PARAMETERS_END();

    /* Validate: path cannot contain query or fragment */
    if (memchr(ZSTR_VAL(path), '?', ZSTR_LEN(path)) ||
        memchr(ZSTR_VAL(path), '#', ZSTR_LEN(path))) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Path cannot contain query string or fragment", 0);
        RETURN_THROWS();
    }

    src = Z_SIGNALFORGE_URI_P(ZEND_THIS);
    dst = signalforge_uri_clone(src, return_value);

    if (dst->path) {
        zend_string_release(dst->path);
    }
    dst->path = zend_string_copy(path);
}
/* }}} */

/* {{{ withQuery(string $query): static */
PHP_METHOD(Signalforge_Http_Uri, withQuery)
{
    zend_string *query;
    signalforge_uri_object *src, *dst;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(query)
    ZEND_PARSE_PARAMETERS_END();

    /* Validate: query cannot contain fragment */
    if (memchr(ZSTR_VAL(query), '#', ZSTR_LEN(query))) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Query cannot contain fragment", 0);
        RETURN_THROWS();
    }

    src = Z_SIGNALFORGE_URI_P(ZEND_THIS);
    dst = signalforge_uri_clone(src, return_value);

    if (dst->query) {
        zend_string_release(dst->query);
    }

    /* Strip leading ? if present */
    if (ZSTR_LEN(query) > 0 && ZSTR_VAL(query)[0] == '?') {
        dst->query = zend_string_init(ZSTR_VAL(query) + 1, ZSTR_LEN(query) - 1, 0);
    } else {
        dst->query = zend_string_copy(query);
    }
}
/* }}} */

/* {{{ withFragment(string $fragment): static */
PHP_METHOD(Signalforge_Http_Uri, withFragment)
{
    zend_string *fragment;
    signalforge_uri_object *src, *dst;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(fragment)
    ZEND_PARSE_PARAMETERS_END();

    src = Z_SIGNALFORGE_URI_P(ZEND_THIS);
    dst = signalforge_uri_clone(src, return_value);

    if (dst->fragment) {
        zend_string_release(dst->fragment);
    }

    /* Strip leading # if present */
    if (ZSTR_LEN(fragment) > 0 && ZSTR_VAL(fragment)[0] == '#') {
        dst->fragment = zend_string_init(ZSTR_VAL(fragment) + 1, ZSTR_LEN(fragment) - 1, 0);
    } else {
        dst->fragment = zend_string_copy(fragment);
    }
}
/* }}} */

/* ============================================================================
 * PHP METHODS - FACTORY
 * ============================================================================ */

/* {{{ fromString(string $uri): static */
PHP_METHOD(Signalforge_Http_Uri, fromString)
{
    zend_string *uri_str;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(uri_str)
    ZEND_PARSE_PARAMETERS_END();

    object_init_ex(return_value, signalforge_uri_ce);
    signalforge_uri_object *intern = Z_SIGNALFORGE_URI_P(return_value);

    /* Release defaults */
    if (intern->scheme) zend_string_release(intern->scheme);
    if (intern->user) zend_string_release(intern->user);
    if (intern->pass) zend_string_release(intern->pass);
    if (intern->host) zend_string_release(intern->host);
    if (intern->path) zend_string_release(intern->path);
    if (intern->query) zend_string_release(intern->query);
    if (intern->fragment) zend_string_release(intern->fragment);

    intern->scheme = NULL;
    intern->user = NULL;
    intern->pass = NULL;
    intern->host = NULL;
    intern->path = NULL;
    intern->query = NULL;
    intern->fragment = NULL;

    if (signalforge_parse_uri(ZSTR_VAL(uri_str), ZSTR_LEN(uri_str), intern) == FAILURE) {
        zend_throw_exception(spl_ce_InvalidArgumentException,
            "Invalid URI", 0);
        RETURN_THROWS();
    }
}
/* }}} */

/* Create Uri from string for internal use */
zend_object *signalforge_uri_create_from_string(const char *uri, size_t len)
{
    zend_object *obj = signalforge_uri_create_object(signalforge_uri_ce);
    signalforge_uri_object *intern = signalforge_uri_from_obj(obj);

    /* Release defaults */
    if (intern->scheme) zend_string_release(intern->scheme);
    if (intern->user) zend_string_release(intern->user);
    if (intern->pass) zend_string_release(intern->pass);
    if (intern->host) zend_string_release(intern->host);
    if (intern->path) zend_string_release(intern->path);
    if (intern->query) zend_string_release(intern->query);
    if (intern->fragment) zend_string_release(intern->fragment);

    intern->scheme = NULL;
    intern->user = NULL;
    intern->pass = NULL;
    intern->host = NULL;
    intern->path = NULL;
    intern->query = NULL;
    intern->fragment = NULL;

    signalforge_parse_uri(uri, len, intern);

    return obj;
}

/* ============================================================================
 * ARGINFO
 * ============================================================================ */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_uri_getScheme, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_uri_getUserInfo, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_uri_getHost, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_uri_getPort, 0, 0, IS_LONG, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_uri_getPath, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_uri_getQuery, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_uri_getFragment, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_uri_getAuthority, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_uri___toString, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_uri_withScheme, 0, 1, Signalforge\\NativeHttp\\Uri, 0)
    ZEND_ARG_TYPE_INFO(0, scheme, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_uri_withUserInfo, 0, 1, Signalforge\\NativeHttp\\Uri, 0)
    ZEND_ARG_TYPE_INFO(0, user, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, password, IS_STRING, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_uri_withHost, 0, 1, Signalforge\\NativeHttp\\Uri, 0)
    ZEND_ARG_TYPE_INFO(0, host, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_uri_withPort, 0, 1, Signalforge\\NativeHttp\\Uri, 0)
    ZEND_ARG_TYPE_INFO(0, port, IS_LONG, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_uri_withPath, 0, 1, Signalforge\\NativeHttp\\Uri, 0)
    ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_uri_withQuery, 0, 1, Signalforge\\NativeHttp\\Uri, 0)
    ZEND_ARG_TYPE_INFO(0, query, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_uri_withFragment, 0, 1, Signalforge\\NativeHttp\\Uri, 0)
    ZEND_ARG_TYPE_INFO(0, fragment, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_uri_fromString, 0, 1, Signalforge\\NativeHttp\\Uri, 0)
    ZEND_ARG_TYPE_INFO(0, uri, IS_STRING, 0)
ZEND_END_ARG_INFO()

/* ============================================================================
 * METHOD TABLE
 * ============================================================================ */

static const zend_function_entry signalforge_uri_methods[] = {
    /* Getters */
    PHP_ME(Signalforge_Http_Uri, getScheme, arginfo_uri_getScheme, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Uri, getUserInfo, arginfo_uri_getUserInfo, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Uri, getHost, arginfo_uri_getHost, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Uri, getPort, arginfo_uri_getPort, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Uri, getPath, arginfo_uri_getPath, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Uri, getQuery, arginfo_uri_getQuery, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Uri, getFragment, arginfo_uri_getFragment, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Uri, getAuthority, arginfo_uri_getAuthority, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Uri, __toString, arginfo_uri___toString, ZEND_ACC_PUBLIC)

    /* Immutable modifiers */
    PHP_ME(Signalforge_Http_Uri, withScheme, arginfo_uri_withScheme, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Uri, withUserInfo, arginfo_uri_withUserInfo, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Uri, withHost, arginfo_uri_withHost, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Uri, withPort, arginfo_uri_withPort, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Uri, withPath, arginfo_uri_withPath, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Uri, withQuery, arginfo_uri_withQuery, ZEND_ACC_PUBLIC)
    PHP_ME(Signalforge_Http_Uri, withFragment, arginfo_uri_withFragment, ZEND_ACC_PUBLIC)

    /* Factory */
    PHP_ME(Signalforge_Http_Uri, fromString, arginfo_uri_fromString, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)

    PHP_FE_END
};

/* ============================================================================
 * CLASS REGISTRATION
 * ============================================================================ */

void signalforge_uri_register_class(void)
{
    zend_class_entry ce;

    INIT_NS_CLASS_ENTRY(ce, "Signalforge\\NativeHttp", "Uri", signalforge_uri_methods);
    signalforge_uri_ce = zend_register_internal_class(&ce);
    signalforge_uri_ce->create_object = signalforge_uri_create_object;

    /* Set up object handlers */
    memcpy(&signalforge_uri_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    signalforge_uri_handlers.offset = XtOffsetOf(signalforge_uri_object, std);
    signalforge_uri_handlers.free_obj = signalforge_uri_free_obj;
    signalforge_uri_handlers.clone_obj = signalforge_uri_clone_obj;

    /* Mark as final - cannot be extended */
    signalforge_uri_ce->ce_flags |= ZEND_ACC_FINAL;

    /* Implement Stringable interface */
    zend_class_implements(signalforge_uri_ce, 1, zend_ce_stringable);

    /* Implement PSR-7 UriInterface */
    if (psr7_uri_interface_ce) {
        zend_class_implements(signalforge_uri_ce, 1, psr7_uri_interface_ce);
    }
}
