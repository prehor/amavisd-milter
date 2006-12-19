dnl $Id: acinclude.m4,v 1.4 2006/01/25 21:53:27 reho Exp $

dnl Checks for C compiler
AC_DEFUN([AC_CHECK_C_COMPILER],
[
  dnl Configure --enable-debug option
  AC_ARG_ENABLE(debug,[  --enable-debug          enables debug output and debug symbols @<:@default=no@:>@],
  [
   if test $enableval = "no"
     then
       enable_debug="no"
     else
       enable_debug="yes"
   fi
  ],
  [
    enable_debug="no"
  ])

  dnl This prevents stupid AC_PROG_CC to add "-g" to the default CFLAGS
  CFLAGS=" $CFLAGS"

  AC_PROG_CC

  if test "$GCC" = "yes";
    then
      if test "$enable_debug" = "yes"
        then
          CFLAGS="-g -O2 $CFLAGS"
          CFLAGS="-ansi -pedantic -W -Wall -Wshadow -Wpointer-arith -Wmissing-prototypes -Wmissing-declarations -Wwrite-strings $CFLAGS"
        else
          CFLAGS="-O2 -DNDEBUG $CFLAGS"
      fi
  fi

  AC_LANG_C
])

dnl Checks for working POSIX semaphores
AC_DEFUN([AC_CHECK_POSIX_SEMAPHORES],
[
  AC_MSG_CHECKING(if POSIX semaphores are working)
  AC_TRY_RUN([
  #include <semaphore.h>
  int main() {
    sem_t sem;
    int rc;
    rc = sem_init(&sem, 0, 0);
    return rc;
  }],
    AC_MSG_RESULT(yes),
    AC_MSG_ERROR(no),
    AC_MSG_ERROR(no)
  )
])

dnl Checks for d_namlen in struct dirent
AC_DEFUN([AC_CHECK_DIRENT_D_NAMLEN],
[
  AC_MSG_CHECKING(for d_namlen in struct dirent)
  AC_TRY_COMPILE([
  #if HAVE_DIRENT_H
  # include <dirent.h>
  #else
  # define dirent direct
  # if HAVE_SYS_NDIR_H
  #  include <sys/ndir.h>
  # endif
  # if HAVE_SYS_DIR_H
  #  include <sys/dir.h>
  # endif
  # if HAVE_NDIR_H
  #  include <ndir.h>
  # endif
  #endif
  ], [
  struct dirent dp;
  int X = dp.d_namlen;
  ],
    AC_DEFINE(HAVE_D_NAMLEN, 1,
      [Define to 1 if `struct direct' has a d_namlen element])
  )
])

dnl Checks for AF_INET6
AC_DEFUN([AC_CHECK_AF_INET6],
[
  AC_CHECK_DECLS([AF_INET6],[],[],[
    #include <sys/types.h>
    #include <sys/socket.h>
  ])
])

dnl Checks for INET6_ADDRSTRLEN
AC_DEFUN([AC_CHECK_INET6_ADDRSTRLEN],
[
  AC_CHECK_DECLS([INET6_ADDRSTRLEN],[],[],[
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
  ])
])

dnl Checks for struct sockaddr_in6
AC_DEFUN([AC_CHECK_STRUCT_SOCKADDR_IN6],
[
  AC_CHECK_TYPES([struct sockaddr_in6],[],[],[
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
  ])
])
