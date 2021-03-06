dnl Process this file with autoconf to produce a configure script.
AC_INIT(main.c)
PACKAGE=hskymon

dnl version number
MAJOR_VERSION=2
MINOR_VERSION=8
MICRO_VERSION=6
BUILD_REVISION=0
EXTRA_VERSION=-20161229
VERSION=$MAJOR_VERSION.$MINOR_VERSION.$MICRO_VERSION$EXTRA_VERSION

dnl set $target
AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE($PACKAGE, $VERSION, no-define)
dnl AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE")
dnl AC_DEFINE_UNQUOTED(VERSION, "$VERSION")
AC_SUBST(PACKAGE)
AC_SUBST(VERSION)
AC_SUBST(MAJOR_VERSION)
AC_SUBST(MINOR_VERSION)
AC_SUBST(MICRO_VERSION)
AC_SUBST(BUILD_REVISION)


AM_CONFIG_HEADER(config.h)
AM_PROG_LIBTOOL
dnl AC_ARG_PROGRAM
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AC_CHECK_PROG(WINDRES, windres, windres)
AM_CONDITIONAL(HAVE_WINDRES, test x"$WINDRES" != x)

AC_PATH_PROGS(OBJCOPY, gobjcopy objcopy)

# Checks for header files.
AC_FUNC_ALLOCA
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([pwd.h sys/utsname.h])


dnl Win32
AC_ARG_WITH(win32,
		 AC_HELP_STRING([--with-win32],
		            [Make for Win32]),
			    [use_win32=yes])
if test x"${use_win32}" = "xyes"; then
AC_DEFINE(USE_WIN32,[], [Make for Win32])
CFLAGS="$CFLAGS -mms-bitfields -DWINVER=0x0501"
LDFLAGS="$LDFLAGS -mwindows"
WINMM_LIBS="-lwinmm"
WINSOCK_LIBS="-lws2_32"
localedir='share/locale'
else
WINMM_LIBS=""
WINSOCK_LIBS=""
localedir='${prefix}/${DATADIRNAME}/locale'
fi
AC_SUBST(localedir)
AC_SUBST(WINMM_LIBS)
AC_SUBST(WINSOCK_LIBS)

dnl Gtk+
AM_PATH_GLIB_2_0(2.8.0,, AC_MSG_ERROR([glib-2.x is required]), gthread gobject)
AM_PATH_GTK_2_0(2.8.0,, AC_MSG_ERROR([gtk+-2.x is required]), gthread gobject)
AC_DEFINE([USE_GTK2],[],[Use Gtk+2.x instead of Gtk+1.2])

dnl  XMLRPC
 AC_ARG_WITH(xmlrpc,
 		 AC_HELP_STRING([--with-xmlrpc],
		            [make with xmlrpc for Gen2]),
			    [with_xmlrpc=yes])
 
 if test x"${with_xmlrpc}" = "xyes"; then
 if test x"${use_win32}" = "xyes"; then
 XMLRPC_CFALGS=""
 XMLRPC_LIBS=""
 else
 AC_DEFINE([USE_XMLRPC],[],[With xmlrpc for Gen2])

dnl for Ubuntu
dnl   --- from here ---
    XMLRPC_CFLAGS="`xmlrpc-c-config client --cflags`"
    XMLRPC_LIBS="`xmlrpc-c-config client --libs`"
dnl    XMLRPC_CFLAGS=""
dnl    XMLRPC_LIBS="-lxmlrpc_xmltok -lxmlrpc_xmlparse -lxmlrpc_util -lxmlrpc_client -lxmlrpc"
dnl   -----------------

dnl libm
AC_SEARCH_LIBS([cos], [m], [], [AC_MSG_ERROR([unable to find the cos() function])])

dnl for Fedora
dnl   --- from here ---
dnl    XMLRPC_CFLAGS="`pkg-config xmlrpc --cflags`"
dnl    XMLRPC_CFLAGS="$XMLRPC_CFLAGS `pkg-config xmlrpc_client --cflags`"
dnl    XMLRPC_CFLAGS="$XMLRPC_CFLAGS `pkg-config xmlrpc_util --cflags`"
dnl
dnl    XMLRPC_LIBS="`pkg-config xmlrpc --libs`"
dnl    XMLRPC_LIBS="$XMLRPC_LIBS `pkg-config xmlrpc_client --libs`"
dnl    XMLRPC_LIBS="$XMLRPC_LIBS `pkg-config xmlrpc_util --libs`"
dnl   -----------------
 fi
 else
 XMLRPC_CFALGS=""
 XMLRPC_LIBS=""
 fi
 AC_SUBST(XMLRPC_LIBS)
 AC_SUBST(XMLRPC_CFLAGS)

dnl XML2
AM_PATH_XML2(2.2.6,, AC_MSG_ERROR([xml-2.2.6 or later is required]))


dnl Maintainer Mode
AC_ARG_WITH(maintain,
		 AC_HELP_STRING([--with-maintain],
		            [For Maintainer]),
			    [use_maint=yes])
             

if test x"${use_maint}" = "xyes"; then
	DISABLE_DEPRECATED="-g -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"
else
	DISABLE_DEPRECATED=""
fi
AC_SUBST(DISABLE_DEPRECATED)



AC_OUTPUT([
Makefile
version.h
hskymon_res.rc
libnova/Makefile
std_data/Makefile
])


dnl Output the configuration summary
echo ""
echo "####################"
if test x"${use_win32}" = "xyes"; then
echo " $PACKAGE $VERSION build for Win32 "
else
if test x"${with_xmlrpc}" = "xyes"; then
echo " $PACKAGE $VERSION build for Gen2 "
else
echo " $PACKAGE $VERSION"
fi
fi
echo "####################"
echo ""
echo ""
echo "Configure finished, type 'make' to build."
