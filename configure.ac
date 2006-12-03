dnl $Id: configure.ac,v 1.29 2006/10/19 20:52:57 reho Exp $

AC_INIT(amavisd-milter, 1.2.0)
AC_CONFIG_SRCDIR(aclocal/acinclude.m4)
AC_CONFIG_AUX_DIR(aclocal)
AC_CONFIG_MACRO_DIR(aclocal)
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE

AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_CHECK_C_COMPILER
AC_CPP_FUNC
ACX_PTHREAD([LIBS="$PTHREAD_LIBS $LIBS" CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
  CC="$PTHREAD_CC"] AC_DEFINE(HAVE_PTHREAD, 1),
  AC_MSG_ERROR([no usable pthreads library found]))
AC_TYPE_MODE_T
AC_TYPE_SIZE_T
AC_HEADER_DIRENT
AC_CHECK_DIRENT_D_NAMLEN
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_HEADER_TIME
AC_STRUCT_TIMEZONE
AC_CHECK_HEADERS([arpa/inet.h ctype.h errno.h fcntl.h limits.h \
  netinet/in.h stdarg.h stdio.h stdlib.h string.h sys/param.h sys/time.h \
  sys/types.h sys/socket.h sys/stat.h sys/un.h syslog.h sysexits.h unistd.h],[],
  AC_MSG_ERROR([unable to find required header files]))

AC_CHECK_LIB(rt, sem_init, LIBS="$LIBS -lrt")
AC_CHECK_POSIX_SEMAPHORES

AC_CHECK_HEADERS([fts.h])
AC_CHECK_FUNCS([arc4random])
AC_REPLACE_FUNCS([daemon fts_open mkdtemp strlcpy])

AX_PATH_MILTER([8.12], [LIBS="$MILTER_LIBS $LIBS"
  LDFLAGS="$MILTER_LDFLAGS $LDFLAGS" CPPFLAGS="$CPPFLAGS $MILTER_CPPFLAGS"],
  AC_MSG_ERROR([required milter library and header not found]))
AC_CHECK_FUNCS([smfi_insheader smfi_opensocket smfi_progress])

AC_EILSEQ
gl_EOVERFLOW

AC_CONFIG_FILES([Makefile amavisd-milter/Makefile compat/Makefile])
AC_CONFIG_FILES([autoconf], [chmod +x autoconf])

AC_OUTPUT
