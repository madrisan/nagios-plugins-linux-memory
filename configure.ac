dnl Process this file with autoconf to produce a configure script.

m4_define(PROG_VERSION, 1)
m4_define(PROG_NAME, [nagios-plugins-memory])
m4_define(PROG_BUGREPORT, [davide.madrisan@gmail.com])

AC_INIT([PROG_NAME],[PROG_VERSION],[PROG_BUGREPORT],[PROG_NAME])
AC_PREREQ([2.59])

AC_CONFIG_SRCDIR([check_memory.c])
AC_CONFIG_HEADERS(config.h:config.hin)

AM_INIT_AUTOMAKE([gnu dist-bzip2])

AM_MAINTAINER_MODE
AC_CANONICAL_HOST

dnl Checks for programs
AC_PROG_CC
AC_PROG_GCC_TRADITIONAL

dnl Checks for header files
AC_HEADER_STDC

AC_PREFIX_DEFAULT(/usr/local/nagios)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_PROTOTYPES

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
