dnl $Id$
dnl config.m4 for extension libsms

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(libsms, for libsms support,
dnl Make sure that the comment is aligned:
dnl [  --with-libsms             Include libsms support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(libsms, whether to enable libsms support,
dnl Make sure that the comment is aligned:
[  --enable-libsms           Enable libsms support])

if test "$PHP_LIBSMS" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-libsms -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/libsms.h"  # you most likely want to change this
  dnl if test -r $PHP_LIBSMS/; then # path given as parameter
  dnl   LIBSMS_DIR=$PHP_LIBSMS
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for libsms files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       LIBSMS_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$LIBSMS_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the libsms distribution])
  dnl fi

  dnl # --with-libsms -> add include path
  dnl PHP_ADD_INCLUDE($LIBSMS_DIR/include)

  dnl # --with-libsms -> chech for lib and symbol presence
  dnl LIBNAME=libsms # you may want to change this
  dnl LIBSYMBOL=libsms # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $LIBSMS_DIR/lib, LIBSMS_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_LIBSMSLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong libsms lib version or lib not found])
  dnl ],[
  dnl   -L$LIBSMS_DIR/lib -lm -ldl
  dnl ])
  dnl
  dnl PHP_SUBST(LIBSMS_SHARED_LIBADD)

  PHP_EXTENSION(libsms, $ext_shared)
fi
