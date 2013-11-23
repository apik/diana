AC_INIT([diana], 2.3.6, https://github.com/apik/diana/)
AC_CONFIG_SRCDIR(src/init.c)

AM_INIT_AUTOMAKE([foreign])

dnl lib needed for DIANA
AC_CHECK_LIB([m],[ceil])
                                                           
AC_PROG_CC
AC_PROG_F77

AC_PROG_INSTALL
AC_LANG([C])

dnl Checking for wget or curl
dnl there is no wget on Mac OS X by default 
AC_CHECK_PROG(WGET_CHECK,wget,yes)
AC_CHECK_PROG(CURL_CHECK,curl,yes)
if test x"$WGET_CHECK" != x"yes" ; then
if test x"$CURL_CHECK" != x"yes" ; then
AC_MSG_ERROR([Please install wget or curl  before installing.])
else
dnl we have curl installed
AC_SUBST([DOWNLOAD_PROGRAM],["curl  --user anonymous:DIANA -o"])
fi
else 
dnl we have wget installed
AC_SUBST([DOWNLOAD_PROGRAM],["wget --quiet --user=anonymous --password=DIANA -O"])
fi
    

AC_OUTPUT(Makefile diana qgraf/Makefile src/Makefile )
