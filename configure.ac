AC_PREREQ([2.71])
AC_INIT([norcom2kv],[3.0.3],[kvoss@met.no])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_MACRO_DIR([m4])

LT_INIT
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_INSTALL

AC_CONFIG_HEADERS(config.h)
AX_CXX_COMPILE_STDCXX([20],[noext],[mandatory]) 
KV_FIND_OMNIORB4
PKG_CHECK_MODULES(putools, puTools)
LIBKVCPP

GTEST_CHECK
GMOCK_CHECK

AX_BOOST_BASE
AX_BOOST_THREAD
if test -z $BOOST_THREAD_LIB; then AC_MSG_ERROR([Unable to find boost thread library]); fi 
AX_BOOST_REGEX
if test -z $BOOST_REGEX_LIB; then AC_MSG_ERROR([Unable to find boost regex library]); fi 
AX_BOOST_FILESYSTEM
if test -z $BOOST_FILESYSTEM_LIB; then AC_MSG_ERROR([Unable to find boost filesystem library]); fi
AX_BOOST_SYSTEM
if test -z $BOOST_SYSTEM_LIB; then AC_MSG_ERROR([Unable to find boost system library]); fi
AX_BOOST_PROGRAM_OPTIONS
if test -z $BOOST_PROGRAM_OPTIONS_LIB; then AC_MSG_ERROR([Unable to find boost program_options library]); fi
 


AC_CONFIG_FILES([Makefile
                 src/Makefile
])


AC_OUTPUT
