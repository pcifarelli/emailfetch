#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([emailfetch], [0.1], [paul.cifarelli@thomsonreuters.com])
#LT_INIT
AC_CONFIG_SRCDIR([src/EmailFetcherMain.cpp])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS(config.h)

# --enable-docker		build docker from amazonlinux (default) (Dockerfile=Dockerfile.amazonlinux)
# --enable-docker=amazonlinux	build docker from amazonlinux (Dockerfile=Dockerfile.amazonlinux)
# --enable-docker=ubuntu	build docker from ubuntu (Dockerfile=Dockerfile.ubuntu)
#
AC_ARG_ENABLE([docker],
[ --enable-docker		Build docker image],
[case "${enableval}" in
  yes) docker=amazonlinux ;;
  amazonlinux) docker=amazonlinux ;;
  ubuntu) docker=ubuntu ;;
  no)  docker=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --with-docker]) ;;
esac], [docker=false])
AM_CONDITIONAL([DOCKER], [test x$docker = xamazonlinux -o x$docker = xubuntu])
AM_CONDITIONAL([AMAZONLINUX], [test x$docker = xamazonlinux])
AM_CONDITIONAL([UBUNTU], [test x$docker = xubuntu])

# Check for the environment variable AWSSDKPATH being set (default is /usr/local otherwise)
AC_ARG_VAR([AWSSDKPATH],[AWS C++ SDK path] )
AM_CONDITIONAL([AWSSDKPATH_NOT_SET], [test -z "$AWSSDKPATH"])

# Check for the environment variable AWSSDKLIB being set (default is /lib otherwise)
AC_ARG_VAR([AWSSDKLIB],[AWS C++ SDK lib directory name (default=lib) - e.g. AWSSDKLIB=lib64] )
AM_CONDITIONAL([AWSSDKLIB_NOT_SET], [test -z "$AWSSDKLIB"])

# Checks for programs.
AC_PROG_CXX

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_LIB([ssl], [SSL_library_init])
AC_CHECK_LIB([crypto], [SHA256_Init])
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([curl],[curl_global_cleanup])

AC_OUTPUT(Makefile src/Makefile docker/Makefile)
