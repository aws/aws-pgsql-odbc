# Process this file with autoconf to produce a configure script.
AC_INIT(awspsqlodbc, 1.00.0100)
AC_PREREQ(2.69)
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_MACRO_DIRS([config])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([bind.c])
AM_MAINTAINER_MODE

# Define a macro to convert relative path to full path
# First argument is the relative path
# Second argument is the variable to store the full path
AC_DEFUN([AC_FULL_PATH], [
  AC_MSG_CHECKING([for full path of $1])
  full_path=`cd $1 && pwd`
  AC_MSG_RESULT([$full_path])
  eval $2=$full_path
])

# 0. Options processing

AC_PROG_CC
AC_PROG_CXX

#
# Add a debug option (no value required, enabled if present)
#
AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug], [enable debug mode (default is off)])],
    [enable_debug=true],  # If --enable-debug is present, set to true
    [enable_debug=false]  # If --enable-debug is absent, set to false
)

# Set CFLAGS and CXXFLAGS based on the debug option
if test "$enable_debug" = "true"; then
    CFLAGS="-g -O0 -fno-inline"
    CXXFLAGS="-g -O0 -fno-inline"
    AC_DEFINE([DEBUG], [1], [Enable debugging])
fi

# Output debugging status
AM_CONDITIONAL([DEBUG_MODE], [test "$enable_debug" = "true"])
AC_MSG_CHECKING([whether to enable debugging])
AC_MSG_RESULT([$enable_debug])

AM_CONDITIONAL([GCC], [test -n "$GCC"])

# C++ language level
CXXFLAGS="${CXXFLAGS} -std=c++20"

# All AC_CHECK_SIZEOF() or AC_CHECK_TYPES() calls were move from 4.
# unixODBC wants the following to get sane behavior for ODBCINT64
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long int)	# for unixODBC
AC_CHECK_SIZEOF(void *)
AC_CHECK_TYPES([long long])
AC_CHECK_TYPES([signed char])
AC_CHECK_TYPES([ssize_t])

AC_CHECK_SIZEOF([bool], [],
[#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#endif])

AC_CHECK_HEADER_STDBOOL()

dnl We use <stdbool.h> if we have it and it declares type bool as having
dnl size 1.  Otherwise, c.h will fall back to declaring bool as unsigned char.
if test "$ac_cv_header_stdbool_h" = yes && test "$ac_cv_sizeof_bool" = 1; then
  AC_DEFINE([PG_USE_STDBOOL], 1,
            [Define to 1 to use <stdbool.h> to define type bool.])
fi

AC_TYPE_SIZE_T

# Check if "-Wall" is valid
if test -n "$GCC" && test "$ac_test_CFLAGS" != set; then

	AC_MSG_CHECKING(-Wall is a valid compile option)
	CFLAGS_SAVE="${CFLAGS}"
	CFLAGS="${CFLAGS} -Wall"
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
		[[#include <stdio.h>]],
		[])],
		[AC_MSG_RESULT(yes)],
		[AC_MSG_RESULT(no)
		 CFLAGS="${CFLAGS_SAVE}"])
	CXXFLAGS="${CXXFLAGS} -Wall"
fi

#
# Apple build support
#
AC_CANONICAL_HOST
AC_MSG_NOTICE(["HOST = ${host}"])

case "$host" in
  *apple-darwin*)
    AC_DEFINE([HAVE_STRLCAT], [], [])
    LDFLAGS="${LDFLAGS} -framework CoreFoundation -framework Security"
    MACOS_BUILD=yes
    OS_TYPE="macos"
  ;;
  *linux*)
    OS_TYPE="linux"
    ;;
  *)
    OS_TYPE="other"
    ;;
esac
AC_MSG_NOTICE(["OS_TYPE = ${OS_TYPE}"])
AC_SUBST(OS_TYPE)

#
# Whether unixODBC driver manager is used
#
AC_ARG_WITH(unixodbc, [  --with-unixodbc[[=DIR]]	  [[default=yes]] DIR is the unixODBC base install
			  directory or the path to odbc_config],
[], [with_unixodbc=yes])
AC_MSG_NOTICE([with_unixodbc: $with_unixodbc])


#
# Whether iODBC driver manager is used
#
AC_ARG_WITH(iodbc, [  --with-iodbc[[=DIR]]	  [[default=no]] DIR is the iODBC base install
			  directory or the path to iodbc-config],
[], [with_iodbc=no])


if test "$with_iodbc" != no; then
	with_unixodbc=no
	AC_DEFINE(WITH_IODBC, 1, [Define to 1 to build with iODBC support])
	if test "$with_iodbc" = yes; then
		AC_PATH_PROGS(ODBC_CONFIG, iodbc-config)
	else
		ODBC_CONFIG=$with_iodbc
	fi
	if test ! -x "${ODBC_CONFIG}/bin/iodbc-config"; then
		if test ! -x "${ODBC_CONFIG}"; then
			AC_MSG_ERROR([iodbc-config not found (required for iODBC build)])
		fi
	else
		ODBC_CONFIG=${ODBC_CONFIG}/bin/iodbc-config
	fi
fi

wo_odbc_config=__without_odbc_config
if test "$with_unixodbc" != no; then
	AC_DEFINE(WITH_UNIXODBC, 1,
            [Define to 1 to build with unixODBC support])
	if test "$with_unixodbc" = yes; then
		AC_PATH_PROGS(ODBC_CONFIG, odbc_config)
	else
		ODBC_CONFIG=$with_unixodbc
	fi
	if test "${ODBC_CONFIG}" = "${wo_odbc_config}"; then
		:
	elif test ! -x "${ODBC_CONFIG}/bin/odbc_config"; then
		if test ! -x "${ODBC_CONFIG}"; then
			AC_MSG_ERROR([odbc_config not found (required for unixODBC build)])
		fi
	else
		ODBC_CONFIG=${ODBC_CONFIG}/bin/odbc_config
	fi
fi

#
# ODBC include and library
#

if test "$ODBC_CONFIG" != ""; then
	if test "$with_iodbc" != no; then
		ODBC_INCLUDE=`${ODBC_CONFIG} --cflags`
		CPPFLAGS="$CPPFLAGS ${ODBC_INCLUDE}"
		# Linking libiodoc is rather problematic
		[ODBC_LIBDIR=`${ODBC_CONFIG} --libs | sed -e "s/^\(-L\|.*[ \t]-L\)\([^ \n\r\f\t]*\).*$/-L\2/"`]
		LDFLAGS="$LDFLAGS ${ODBC_LIBDIR}"
		LIBODBC=`${ODBC_CONFIG} --libs`	# for regression test
	elif test "${ODBC_CONFIG}" = "${wo_odbc_config}"; then
		ODBC_INCLUDE=/usr/include
		CPPFLAGS="$CPPFLAGS -I${ODBC_INCLUDE}"
		ODBC_LIBDiR=""
		LIBODBC="-lodbc"	# for regression test
	else
		ODBC_INCLUDE=`${ODBC_CONFIG} --include-prefix`
		CPPFLAGS="$CPPFLAGS -I${ODBC_INCLUDE}"
		# Linking libodbc is rather problematic
		ODBC_LIBDIR=`${ODBC_CONFIG} --lib-prefix`
		if test "${ODBC_LIBDIR}" != ""; then
			LDFLAGS="$LDFLAGS -L${ODBC_LIBDIR}"
		fi
		if test "$MACOS_BUILD" == yes; then
			LDFLAGS="$LDFLAGS -lodbcinst"
		fi
		LIBODBC=`${ODBC_CONFIG} --libs`	# for regression test
	fi
	AC_MSG_NOTICE([using $ODBC_INCLUDE $ODBC_LIBDIR])
	AC_MSG_NOTICE([using $LIBODBC for regression test])
fi
AC_SUBST(LIBODBC)

#
# SQLCOLATTRIBUTE_SQLLEN check
#
AC_MSG_CHECKING(last argument to SQLColAttribute is SQLLEN *)
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include <sql.h>

SQLRETURN SQL_API SQLColAttribute(SQLHSTMT StatementHandle,
  SQLUSMALLINT ColumnNumber,
  SQLUSMALLINT FieldIdentifier,
  SQLPOINTER CharacterAttribute,
  SQLSMALLINT BufferLength,
  SQLSMALLINT *StringLength,
  SQLLEN *NumericAttribute)
{
  SQLLEN t = 1;
  *NumericAttribute = t;
  return 0;
}
]])],
  [AC_DEFINE(SQLCOLATTRIBUTE_SQLLEN, 1,
	[Define to 1 if SQLColAttribute use SQLLEN])
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)]
)


#
# Whether libpq functionalities are used
#
AC_ARG_WITH(libpq, [  --with-libpq[[=DIR]]	  DIR is the PostgreSQL base install
			  directory or the path to pg_config],
[], [with_libpq=yes])

if test "$with_libpq" = no; then
	AC_MSG_ERROR([libpq is mandatory, --without-libpq is no longer supported])
fi

if test "$with_libpq" != yes; then
	if test -d "$with_libpq"; then
		PATH="$with_libpq/bin:$PATH"
		CPPFLAGS="$CPPFLAGS -I$with_libpq/include -I$with_libpq/include/postgresql/internal"
		LDFLAGS="$LDFLAGS -L$with_libpq/lib"
	else
		if test -x "$with_libpq"; then
			PG_CONFIG=$with_libpq
		else
			AC_MSG_ERROR([specified pg_config not found])
		fi
	fi
fi

#
# Pthreads
#

PGAC_ARG_BOOL(enable, pthreads, yes,
              [  --disable-pthreads      do not build with POSIX threads],
              [AC_DEFINE(POSIX_MULTITHREAD_SUPPORT, 1,
                         [Define to 1 to build with pthreads support (--enable-pthreads)])
               AC_DEFINE(_REENTRANT, 1, [Define _REENTRANT for several platforms])])


#
# Find libpq headers and libraries
#

if test -z "$PG_CONFIG"; then
  AC_MSG_NOTICE([Checking for pg_config])
  AC_PATH_PROGS(PG_CONFIG, pg_config)
fi

if test -n "$PG_CONFIG"; then
  AC_MSG_NOTICE([Running PG_CONFIG: ${PG_CONFIG}])
  pg_includedir=`"$PG_CONFIG" --includedir`
  pg_pkg_includedir=`"$PG_CONFIG" --pkgincludedir`
  pg_libdir=`"$PG_CONFIG" --libdir`
  CPPFLAGS="$CPPFLAGS -I$pg_includedir -I$pg_pkg_includedir/internal"
  LDFLAGS="$LDFLAGS -L$pg_libdir -lpq"
fi

#
# aws-rds-odbc
#
AC_ARG_WITH(awsrdsodbc, [  --with-awsrdsodbc		  [[default=yes]] Build with aws-rds-odbc],
[], [with_awsrdsodbc=yes])

AWS_RDS_ODBC_DEPENDENCIES="
	curl
	z
"

if test "$with_awsrdsodbc" != no; then
	AC_FULL_PATH([libs/aws-rds-odbc], [AWS_RDS_ODBC_PATH])
	CPPFLAGS="${CPPFLAGS} -I${AWS_RDS_ODBC_PATH}/src -I${AWS_RDS_ODBC_PATH}/aws_sdk/install/include"

	for dependency in $AWS_RDS_ODBC_DEPENDENCIES; do
		AC_CHECK_LIB([$dependency], [main], [], [AC_MSG_ERROR([lib${dependency} library not found])])
		LDFLAGS="${LDFLAGS} -l${dependency}"
	done

	# macOS build of glog needs gflags
	if test "$MACOS_BUILD" == yes; then
		LDFLAGS="${LDFLAGS} -L/opt/homebrew/opt/gflags/lib -lc++ -lgflags"
	fi
fi
AM_CONDITIONAL([WITH_AWSRDSODBC], [test "x$with_awsrdsodbc" = "xyes"])

# 1. Programs

# 'prove' is used to pretty-print regression test results, if available.
AC_CHECK_PROGS(PROVE, prove)

# 2. Libraries

LT_INIT([disable-static win32-dll dlopen])

# if test "$GITHUB_ACTIONS" != "true" ]; then
if test "$with_unixodbc" != no; then
  AC_SEARCH_LIBS(SQLGetPrivateProfileString, odbcinst, [],
                 [AC_MSG_ERROR([unixODBC library "odbcinst" not found])])
fi
# fi

if test "$with_iodbc" != no; then
  AC_SEARCH_LIBS(SQLGetPrivateProfileString, iodbcinst, [],
                 [AC_MSG_ERROR([iODBC library "iodbcinst" not found])])
fi

if test "$enable_pthreads" = yes; then
  AC_CHECK_LIB(pthreads, pthread_create,
               [],
	       [AC_CHECK_LIB(pthread, pthread_create)])
fi

if test "$GITHUB_ACTIONS" != "true" ]; then
	AC_CHECK_LIB(pq, PQsetSingleRowMode, [],
		      [AC_MSG_ERROR([libpq library version >= 9.2 is required])])
fi

# 3. Header files

AC_CHECK_HEADERS(locale.h sys/time.h uchar.h)
if test "$GITHUB_ACTIONS" != "true" ]; then
	AC_CHECK_HEADER(libpq-fe.h,,[AC_MSG_ERROR([libpq header not found])])
fi
AC_HEADER_STDBOOL


# 4. Types(all members were moved to 0.)

# 5. Structures

AC_STRUCT_TM

# 6. Compiler characteristics

AC_C_CONST


# 7. Functions, global variables

AC_FUNC_STRERROR_R
AC_CHECK_FUNCS(strtoul strtoll strlcat mbstowcs wcstombs mbrtoc16 c16rtomb)

AC_CHECK_FUNCS(PQsslInUse)

if test "$enable_pthreads" = yes; then
  AC_CHECK_FUNCS(localtime_r strtok_r pthread_mutexattr_settype)

  if test x"$ac_cv_func_pthread_mutexattr_settype" = xyes; then
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <pthread.h>]],
                 [[int i = PTHREAD_MUTEX_RECURSIVE;]])],
                 [AC_DEFINE(PG_RECURSIVE_MUTEXATTR, PTHREAD_MUTEX_RECURSIVE,
                            [Define if you have PTHREAD_MUTEX_RECURSIVE])],
                 [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <pthread.h>]],
		                 [[int i = PTHREAD_MUTEX_RECURSIVE_NP;]])],
		                 [AC_DEFINE(PG_RECURSIVE_MUTEXATTR, PTHREAD_MUTEX_RECURSIVE_NP,
                                            [Define if you have PTHREAD_MUTEX_RECURSIVE_NP])])])
  fi
fi

AC_MSG_NOTICE(["AWS_RDS_ODBC_PATH = ${AWS_RDS_ODBC_PATH}"])
AC_MSG_NOTICE(["CFLAGS = ${CFLAGS}"])
AC_MSG_NOTICE(["CPPFLAGS = ${CPPFLAGS}"])
AC_MSG_NOTICE(["CXXFLAGS = ${CXXFLAGS}"])
AC_MSG_NOTICE(["LDFLAGS = ${LDFLAGS}"])

AC_SUBST(AWS_RDS_ODBC_PATH)
AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(LDFLAGS)

AC_CONFIG_FILES([Makefile test/Makefile])
AC_OUTPUT
