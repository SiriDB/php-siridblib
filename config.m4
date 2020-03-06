PHP_ARG_ENABLE(siridb, whether to enable siridb
World support,
[ --enable-siridb   Enable siridb support])
if test "$PHP_SIRIDB" = "yes"; then
  

    AC_MSG_CHECKING(for libsiridb in default path)
    for i in /usr/local /usr; do
        for j in so dylib; do
        if test -r $i/lib/libsiridb.$j; then
            SIRIDB_DIR=$i
            AC_MSG_RESULT(found libsiridb.$j in $i)
        fi
        done
    done

    if test -z "$SIRIDB_DIR"; then
        AC_MSG_RESULT(not found)
        AC_MSG_ERROR(Please install the libsiridb library)
    fi

    AC_MSG_CHECKING([siridb header files])
    SIRIDB_INC_DIR="/usr/local/include/libsiridb"
    INC_FILES="$SIRIDB_INC_DIR/siridb.h"
    for i in $INC_FILES; do
        if test ! -f $i; then
        AC_MSG_ERROR([siridb header files not found in $SIRIDB_INC_DIR])
        fi
    done
    AC_MSG_RESULT($SIRIDB_INC_DIR)

    PHP_ADD_INCLUDE($SIRIDB_INC_DIR)

    PHP_CHECK_LIBRARY(siridb, siridb_create,[
        PHP_ADD_LIBRARY(siridb,1,SIRIDB_SHARED_LIBADD)
        PHP_SUBST(SIRIDB_SHARED_LIBADD)
        AC_DEFINE(HAVE_SIRIDB, 1, [whether the siridb extension is enabled])
    ],[
        AC_MSG_ERROR([Please install libsiridb on the system])
    ])
    
    PHP_NEW_EXTENSION(siridb, siridb.c, $ext_shared)

    
fi