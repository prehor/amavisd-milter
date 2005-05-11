dnl $Id$

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
