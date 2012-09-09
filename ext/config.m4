dnl config.m4 for extension phproto

PHP_ARG_ENABLE(phproto, whether to enable phproto support,
[  --enable-phproto              Enable phproto support])

if test "$PHP_PHPROTO" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-phproto -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/phproto.h"  # you most likely want to change this
  dnl if test -r $PHP_PHPROTO/$SEARCH_FOR; then # path given as parameter
  dnl   PHPROTO_DIR=$PHP_PHPROTO
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for phproto files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       PHPROTO_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$PHPROTO_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the phproto distribution])
  dnl fi

  dnl # --with-phproto -> add include path
  dnl PHP_ADD_INCLUDE($PHPROTO_DIR/include)

  dnl # --with-phproto -> check for lib and symbol presence
  dnl LIBNAME=phproto # you may want to change this
  dnl LIBSYMBOL=phproto # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $PHPROTO_DIR/lib, PHPROTO_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_PHPROTOLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong phproto lib version or lib not found])
  dnl ],[
  dnl   -L$PHPROTO_DIR/lib -lm
  dnl ])
  dnl
  dnl PHP_SUBST(PHPROTO_SHARED_LIBADD)

  PHP_SUBST(PHPROTO_SHARED_LIBADD)

  PHP_NEW_EXTENSION(phproto, phproto.c converter.c proto/test.pb-c.c, $ext_shared)

  PHP_ADD_LIBRARY(protobuf-c, 1, PHPROTO_SHARED_LIBADD)
fi
