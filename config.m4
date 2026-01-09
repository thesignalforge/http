dnl
dnl Signalforge HTTP Extension
dnl config.m4 - Build configuration
dnl
dnl Copyright (c) 2024 Signalforge
dnl License: MIT
dnl

PHP_ARG_ENABLE([signalforge_http],
  [whether to enable signalforge_http support],
  [AS_HELP_STRING([--enable-signalforge_http],
    [Enable signalforge_http support])],
  [no])

if test "$PHP_SIGNALFORGE_HTTP" != "no"; then

  dnl Check for required headers
  AC_CHECK_HEADERS([stdint.h], [], [
    AC_MSG_ERROR([stdint.h not found])
  ])

  dnl Source files
  PHP_NEW_EXTENSION(signalforge_http,
    signalforge_http.c \
    src/psr7_interfaces.c \
    src/request.c \
    src/response.c \
    src/stream.c \
    src/uploadedfile.c,
    $ext_shared,,
    dnl Add ZTS-specific flags only when ZTS is enabled
    m4_ifdef([ZTS], [-DZEND_ENABLE_STATIC_TSRMLS_CACHE=1], []))

  dnl Add header files
  PHP_ADD_BUILD_DIR($ext_builddir)
  PHP_ADD_BUILD_DIR($ext_builddir/src)
  PHP_ADD_INCLUDE($ext_srcdir)
  PHP_ADD_INCLUDE($ext_srcdir/src)

  dnl Install headers for potential use by other extensions
  PHP_INSTALL_HEADERS([ext/signalforge_http], [php_signalforge_http.h src/request.h src/response.h src/stream.h src/uploadedfile.h])

fi
