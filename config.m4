PHP_ARG_ENABLE(xhp, xhp,
[ --enable-xhp   Enable XHP])

PHP_REQUIRE_CXX()
if test "$PHP_XHP" = "yes"; then
  AC_MSG_CHECKING([for flex])
  FLEX=`which flex35 2>/dev/null || which flex 2>/dev/null`
  if test -z "$FLEX"; then
    AC_MSG_ERROR([not found])
  fi
  AC_MSG_RESULT([found $FLEX])

  AC_MSG_CHECKING([for bison])
  BISON=`which bison 2>/dev/null`
  if test -z "$BISON"; then
    AC_MSG_ERROR([not found])
  fi
  AC_MSG_RESULT([found $BISON])

  AC_MSG_CHECKING([for re2c])
  RE2C=`which re2c 2>/dev/null`
  if test -z "$RE2C"; then
    AC_MSG_ERROR([not found])
  fi
  AC_MSG_RESULT([found $RE2C])

  CXXFLAGS="$CXXFLAGS -DUSE_SGI_ROPE"

  PHP_ADD_LIBRARY(stdc++,, XHP_SHARED_LIBADD)
  PHP_SUBST(XHP_SHARED_LIBADD)
  PHP_NEW_EXTENSION(xhp, ext.cpp ext_tokeniz.cpp, $ext_shared)
  PHP_ADD_LIBRARY_WITH_PATH(xhp, $ext_srcdir/xhp/, XHP_SHARED_LIBADD)
  PHP_ADD_MAKEFILE_FRAGMENT
fi
