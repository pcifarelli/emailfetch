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
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-docker]) ;;
esac], [docker=false])
AM_CONDITIONAL([DOCKER], [test x$docker = xamazonlinux -o x$docker = xubuntu])
AM_CONDITIONAL([AMAZONLINUX], [test x$docker = xamazonlinux])
AM_CONDITIONAL([UBUNTU], [test x$docker = xubuntu])

# Check for the environment variable AWSSDKPATH being set (default is /usr/local otherwise)
AC_ARG_VAR([AWSSDKPATH],[AWS C++ SDK path (default=/usr/local) e.g. AWSSDKPATH=/opt/aws] )
AM_CONDITIONAL([AWSSDKPATH_NOT_SET], [test -z "$AWSSDKPATH"])
AM_CONDITIONAL([AWSSDKPATH_ENV_SET], [test ! -z "$AWS_SDK_PATH"])

# Check whether the AWS SDK libs are in lib or lib64
AM_COND_IF([AWSSDKPATH_NOT_SET],
           [AM_COND_IF([AWSSDKPATH_ENV_SET],
                       [AM_CONDITIONAL([AWSLIB64], [test -e ${AWS_SDK_PATH}/lib64/libaws-cpp-sdk-core.so])],
                       [AM_CONDITIONAL([AWSLIB64], [test -e /usr/local/lib64/libaws-cpp-sdk-core.so])])],
           [AM_CONDITIONAL([AWSLIB64], [test -e $AWSSDKPATH/lib64/libaws-cpp-sdk-core.so])]
           )

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
