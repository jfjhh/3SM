#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([3SM], [1.0.1], [https://github.com/jfjhh/3SM/issues])
AC_CONFIG_SRCDIR([3sm.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_ARG_VAR([PM], [Used to suspend the system.])
AC_CHECK_PROG([PM], [pm-is-supported], [/usr/bin/pm-is-supported], [pm-is-supported])

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h stdio.h string.h unistd.h ctype.h sys/types.h sys/wait.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS([memset])

AC_OUTPUT([Makefile])

