# EXPAT ORIGINAL: http://expat.cvs.sourceforge.net/expat/expat/configure.in

# AC_CPP_FUNC
# ------------------ #
# Checks to see if ANSI C99 CPP variable __func__ works.
# If not, perhaps __FUNCTION__ works instead.
# If not, we'll just define __func__ to "".
AC_DEFUN([AC_CPP_FUNC],
[AC_REQUIRE([AC_PROG_CC_STDC])dnl
AC_CACHE_CHECK([for an ANSI C99-conforming __func__], ac_cv_cpp_func,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],
[[char *foo = __func__;]])],
  [ac_cv_cpp_func=yes],
  [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],
[[char *foo = __FUNCTION__;]])],
  [ac_cv_cpp_func=__FUNCTION__],
  [ac_cv_cpp_func=no])])])
if test $ac_cv_cpp_func = __FUNCTION__; then
  AC_DEFINE(__func__,__FUNCTION__,
    [Define to __FUNCTION__ or "" if `__func__' does not conform to ANSI C.])
elif test $ac_cv_cpp_func = no; then
  AC_DEFINE(__func__,"__FILE__:__LINE__",
     [Define to __FUNCTION__ or "" if `__func__' does not conform to ANSI C.])
fi
])# AC_CPP_FUNC
