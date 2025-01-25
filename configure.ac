#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.72])
AC_INIT([chip8-emu], [1.2.0], [https://github.com/miloje357/chip8-emu])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])


# Checks for programs.
AC_PROG_CC

# Checks for libraries.
PKG_CHECK_MODULES([NCURSES], [ncurses])
AC_CHECK_LIB([ncurses], [initscr])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
