dnl $Id: configure.ac,v 1.15.2.3 2006/03/19 11:26:31 reho Exp $

AC_INIT(amavisd-milter, 1.0.6)
AC_CONFIG_SRCDIR(aclocal/acinclude.m4)
AC_CONFIG_AUX_DIR(aclocal)
AC_CONFIG_MACRO_DIR(aclocal)
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE

AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_CHECK_C_COMPILER
ACX_PTHREAD([LIBS="$PTHREAD_LIBS $LIBS" CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
  CC="$PTHREAD_CC" AC_DEFINE(HAVE_PTHREAD, 1)],
  AC_MSG_ERROR([no usable pthreads library found]))
AC_TYPE_MODE_T
AC_TYPE_SIZE_T
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([arpa/inet.h ctype.h errno.h fcntl.h limits.h netinet/in.h \
  stdarg.h stdio.h stdlib.h string.h sys/param.h sys/time.h sys/types.h \
  sys/socket.h sys/stat.h sys/un.h syslog.h sysexits.h unistd.h],[],
  AC_MSG_ERROR([unable to find required header files]))

AC_CHECK_FUNCS([arc4random])
AC_REPLACE_FUNCS([mkdtemp strlcpy])

AX_PATH_MILTER([8.12],[LIBS="$MILTER_LIBS $LIBS"
  LDFLAGS="$MILTER_LDFLAGS $LDFLAGS" CPPFLAGS="$CPPFLAGS $MILTER_CPPFLAGS"],
  AC_MSG_ERROR([required milter library and header not found]))
AC_CHECK_FUNCS([smfi_opensocket])

AC_EILSEQ
gl_EOVERFLOW

AC_CONFIG_FILES([Makefile amavisd-milter/Makefile compat/Makefile])
AC_CONFIG_FILES([autoconf], [chmod +x autoconf])

AC_OUTPUT
