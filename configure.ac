#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(ptouch-driver, 1.3, panic@itu.dk)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([rastertoptch.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.
AC_CHECK_LIB([cups], [cupsParseOptions])
AC_CHECK_LIB([cupsimage], [cupsRasterReadHeader])
AC_CHECK_LIB([m], [lrint])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([cups/cups.h cups/raster.h fcntl.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_STDBOOL
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([memset strcasecmp strstr strtol])

AC_CONFIG_FILES([Makefile
                 ptouch-driver.spec
                 ptouch-driver-foomatic.spec])
AC_OUTPUT
