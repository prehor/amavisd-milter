dnl $Id$

AC_INIT(amavisd-milter, 1.0.0)
AC_CONFIG_SRCDIR(aclocal/acinclude.m4)
AC_CONFIG_AUX_DIR(aclocal)
AC_CONFIG_MACRO_DIR(aclocal)
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE

AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_CHECK_C_COMPILER
AC_CHECK_PTHREADS
AC_TYPE_MODE_T
AC_TYPE_SIZE_T
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h ctype.h errno.h fcntl.h libmilter/mfapi.h \
  libmilter/mfdef.h limits.h netinet/in.h stdarg.h stdbool.h stdio.h \
  stdlib.h string.h sys/param.h sys/time.h sys/types.h sys/socket.h \
  sys/stat.h sys/un.h syslog.h sysexits.h unistd.h],[],
  AC_MSG_ERROR([unable to find required header files]))

AC_CHECK_FUNCS([strlcpy])

AC_CHECK_LIB([milter], [smfi_main], [],
  AC_MSG_ERROR([unable to find milter library]))

AC_CONFIG_FILES([Makefile amavisd-milter/Makefile compat/Makefile])
AC_CONFIG_FILES([autoconf], [chmod +x autoconf])

AC_OUTPUT
