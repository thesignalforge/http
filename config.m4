dnl
dnl Signalforge HTTP Extension
dnl config.m4 - Build configuration
dnl
dnl Copyright (c) 2026 Signalforge
dnl License: MIT
dnl

PHP_ARG_ENABLE([signalforge_http],
  [whether to enable signalforge_http support],
  [AS_HELP_STRING([--enable-signalforge_http],
    [Enable signalforge_http support])],
  [no])

PHP_ARG_ENABLE([signalforge_http_client],
  [whether to enable HTTP client support (PSR-18)],
  [AS_HELP_STRING([--enable-signalforge-http-client],
    [Enable HTTP client support (requires libcurl)])],
  [yes])

PHP_ARG_ENABLE([signalforge_http_client_threads],
  [whether to enable threaded HttpRequestPool (ZTS only)],
  [AS_HELP_STRING([--enable-signalforge-http-client-threads],
    [Enable threaded HttpRequestPool for maximum throughput (requires ZTS, pthread)])],
  [yes])

if test "$PHP_SIGNALFORGE_HTTP" != "no"; then

  dnl Check for required headers
  AC_CHECK_HEADERS([stdint.h], [], [
    AC_MSG_ERROR([stdint.h not found])
  ])

  dnl Base source files (PSR-7 + PSR-17 factories)
  SIGNALFORGE_HTTP_SOURCES="
    signalforge_http.c
    src/psr7_interfaces.c
    src/request.c
    src/response.c
    src/stream.c
    src/uploadedfile.c
    src/uri.c
    src/factories/factories.c
  "

  dnl ============================================================================
  dnl HTTP Client Support (PSR-18) - Works on ALL builds (ZTS and non-ZTS)
  dnl ============================================================================

  SIGNALFORGE_HTTP_CLIENT_ENABLED="no"
  SIGNALFORGE_HTTP_CLIENT_THREADS_ENABLED="no"

  if test "$PHP_SIGNALFORGE_HTTP_CLIENT" != "no"; then

    dnl Check for curl using pkg-config
    PKG_CHECK_MODULES([CURL], [libcurl >= 7.68.0], [
      AC_MSG_CHECKING([for libcurl version])
      CURL_VERSION=`$PKG_CONFIG --modversion libcurl`
      AC_MSG_RESULT([$CURL_VERSION])
      HAVE_CURL="yes"
    ], [
      dnl Fallback to curl-config if pkg-config fails
      AC_PATH_PROG(CURL_CONFIG, curl-config, no)
      if test "$CURL_CONFIG" = "no"; then
        AC_MSG_WARN([curl not found. HTTP client will be disabled.])
        HAVE_CURL="no"
      else
        CURL_CFLAGS=`$CURL_CONFIG --cflags`
        CURL_LIBS=`$CURL_CONFIG --libs`
        CURL_VERSION=`$CURL_CONFIG --version`

        AC_MSG_CHECKING([for libcurl version])
        AC_MSG_RESULT([$CURL_VERSION])

        dnl Check for minimum curl version
        CURL_VERSION_NUM=`$CURL_CONFIG --vernum`
        if test "$CURL_VERSION_NUM" -lt "074400"; then
          AC_MSG_WARN([libcurl 7.68.0 or later is required. HTTP client will be disabled.])
          HAVE_CURL="no"
        else
          HAVE_CURL="yes"
        fi
      fi
    ])

    if test "$HAVE_CURL" = "yes"; then
      PHP_EVAL_INCLINE($CURL_CFLAGS)
      PHP_EVAL_LIBLINE($CURL_LIBS, SIGNALFORGE_HTTP_SHARED_LIBADD)

      dnl Save and update CPPFLAGS for compilation tests
      save_CPPFLAGS="$CPPFLAGS"
      CPPFLAGS="$CPPFLAGS $CURL_CFLAGS"

      dnl Check for curl HTTP/2 support
      AC_MSG_CHECKING([for curl HTTP/2 support])
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
        #include <curl/curl.h>
      ]], [[
        int ver = CURL_HTTP_VERSION_2_0;
        return ver ? 0 : 1;
      ]])], [
        AC_MSG_RESULT([yes])
      ], [
        AC_MSG_WARN([libcurl must be built with HTTP/2 support. HTTP client will be disabled.])
        HAVE_CURL="no"
      ])

      if test "$HAVE_CURL" = "yes"; then
        dnl Check for curl_multi_poll (curl 7.66.0+)
        AC_MSG_CHECKING([for curl_multi_poll])
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
          #include <curl/curl.h>
        ]], [[
          curl_multi_poll(NULL, NULL, 0, 0, NULL);
        ]])], [
          AC_MSG_RESULT([yes])
          AC_DEFINE([HAVE_CURL_MULTI_POLL], [1], [Have curl_multi_poll])
        ], [
          AC_MSG_RESULT([no - will use curl_multi_wait])
        ])

        dnl Check for HTTP/3 support (optional)
        AC_MSG_CHECKING([for curl HTTP/3 support])
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
          #include <curl/curl.h>
        ]], [[
          #if !defined(CURL_HTTP_VERSION_3)
          #error No HTTP/3 support
          #endif
        ]])], [
          AC_MSG_RESULT([yes])
          AC_DEFINE([HAVE_HTTP3], [1], [Have HTTP/3 support])
        ], [
          AC_MSG_RESULT([no])
        ])

        dnl Check for CURLSH (shared handle)
        AC_MSG_CHECKING([for curl share interface])
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
          #include <curl/curl.h>
        ]], [[
          CURLSH *sh = curl_share_init();
          curl_share_cleanup(sh);
        ]])], [
          AC_MSG_RESULT([yes])
        ], [
          AC_MSG_WARN([curl share interface not available. HTTP client will be disabled.])
          HAVE_CURL="no"
        ])
      fi

      dnl Restore CPPFLAGS
      CPPFLAGS="$save_CPPFLAGS"

      if test "$HAVE_CURL" = "yes"; then
        dnl Enable HTTP client (base functionality - works everywhere)
        SIGNALFORGE_HTTP_CLIENT_ENABLED="yes"
        AC_DEFINE([HAVE_SIGNALFORGE_HTTP_CLIENT], [1], [HTTP client enabled])

        dnl Base client source files (synchronous + curl_multi)
        SIGNALFORGE_HTTP_SOURCES="$SIGNALFORGE_HTTP_SOURCES
          src/client/client.c
          src/client/curl_easy.c
          src/client/curl_multi_pool.c
          src/client/share.c
          src/client/request_data.c
          src/client/response_data.c
          src/client/retry.c
        "

        AC_MSG_NOTICE([HTTP client (PSR-18) support enabled])

        dnl ============================================================================
        dnl Threaded HttpRequestPool - Optional, ZTS only
        dnl ============================================================================

        if test "$PHP_SIGNALFORGE_HTTP_CLIENT_THREADS" != "no"; then
          if test "$PHP_THREAD_SAFETY" = "yes"; then
            dnl Check for pthreads
            AC_CHECK_LIB(pthread, pthread_create, [
              HAVE_PTHREAD="yes"
              PHP_ADD_LIBRARY(pthread, 1, SIGNALFORGE_HTTP_SHARED_LIBADD)

              dnl Check for pthread_spinlock (optional optimization)
              AC_MSG_CHECKING([for pthread_spinlock])
              AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
                #include <pthread.h>
              ]], [[
                pthread_spinlock_t lock;
                pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE);
                pthread_spin_destroy(&lock);
              ]])], [
                AC_MSG_RESULT([yes])
                AC_DEFINE([HAVE_PTHREAD_SPINLOCK], [1], [Have pthread spinlock])
              ], [
                AC_MSG_RESULT([no])
              ])

              dnl Enable threaded client
              SIGNALFORGE_HTTP_CLIENT_THREADS_ENABLED="yes"
              AC_DEFINE([HAVE_SIGNALFORGE_HTTP_CLIENT_THREADS], [1], [Threaded HTTP client enabled])

              dnl Add thread-specific source files
              SIGNALFORGE_HTTP_SOURCES="$SIGNALFORGE_HTTP_SOURCES
                src/client/thread_pool.c
                src/client/queue.c
                src/client/curl_worker.c
              "

              AC_MSG_NOTICE([Threaded HttpRequestPool enabled (ZTS + pthread)])
            ], [
              AC_MSG_WARN([pthreads library not found. Threaded mode disabled.])
            ])
          else
            AC_MSG_NOTICE([Threaded HttpRequestPool requires ZTS. Using event-driven mode only.])
          fi
        fi
      fi
    fi
  fi

  if test "$SIGNALFORGE_HTTP_CLIENT_ENABLED" != "yes"; then
    AC_MSG_NOTICE([HTTP client (PSR-18) support disabled])
  fi

  dnl ============================================================================
  dnl Compiler flags
  dnl ============================================================================

  dnl Add math library for retry.c (pow function)
  if test "$SIGNALFORGE_HTTP_CLIENT_ENABLED" = "yes"; then
    PHP_ADD_LIBRARY(m, 1, SIGNALFORGE_HTTP_SHARED_LIBADD)
  fi

  dnl Debug mode flags
  if test "$PHP_DEBUG" = "1"; then
    if test "$SIGNALFORGE_HTTP_CLIENT_ENABLED" = "yes"; then
      CFLAGS="$CFLAGS -DDEBUG_SIGNALFORGE_HTTP_CLIENT"
    fi
  fi

  PHP_SUBST(SIGNALFORGE_HTTP_SHARED_LIBADD)

  dnl ============================================================================
  dnl Extension registration
  dnl ============================================================================

  PHP_NEW_EXTENSION(signalforge_http, $SIGNALFORGE_HTTP_SOURCES, $ext_shared,, $CURL_CFLAGS -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)

  dnl Add build directories
  PHP_ADD_BUILD_DIR($ext_builddir)
  PHP_ADD_BUILD_DIR($ext_builddir/src)
  PHP_ADD_BUILD_DIR($ext_builddir/src/client)
  PHP_ADD_BUILD_DIR($ext_builddir/src/factories)
  PHP_ADD_INCLUDE($ext_srcdir)
  PHP_ADD_INCLUDE($ext_srcdir/src)
  PHP_ADD_INCLUDE($ext_srcdir/src/client)
  PHP_ADD_INCLUDE($ext_srcdir/src/factories)

  dnl Install headers for potential use by other extensions
  PHP_INSTALL_HEADERS([ext/signalforge_http], [php_signalforge_http.h src/request.h src/response.h src/stream.h src/uploadedfile.h src/uri.h])

fi
