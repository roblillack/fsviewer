dnl Process this file with autoconf to produce a configure script.

AC_INIT(FSViewer.app,0.2.6,[guido.scholz@bayernline.de])
AC_CONFIG_SRCDIR([src/FSViewer.c])
AM_INIT_AUTOMAKE([dist-bzip2])
AC_CONFIG_HEADERS([src/config.h])

dnl Checks for programs.
AC_PROG_CC
dnl AC_PROG_INSTALL
dnl AC_PROG_LN_S

dnl AC_ARG_PROGRAM
AC_PROG_RANLIB

dnl the prefix
dnl ==========
dnl
dnl move this earlier in the script... anyone know why this is handled
dnl in such a bizarre way?

test "x$prefix" = xNONE && prefix=$ac_default_prefix
dnl Let make expand exec_prefix.
test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'

dnl look for X windows first so further libs check
AC_PATH_X 

if test "$x_includes" != "NONE"; then
        CFLAGS="$CFLAGS -I$x_includes"
fi
if test "$x_libraries" != "NONE"; then
        LDFLAGS="$LDFLAGS -L$x_libraries"
fi

dnl Additional "default" items
#LIBS="$LIBS -lX11"
#CFLAGS="$CFLAGS -I/usr/X11/include"
#LDFLAGS="$LDFLAGS -L/usr/X11/lib"

dnl Check for get-wraster-flags
AC_CHECK_PROG(GET_WRASTER_FLAGS, get-wraster-flags, get-wraster-flags)
if test "$GET_WRASTER_FLAGS" != ""; then
	dnl CFLAGS first
        c_flags=`$GET_WRASTER_FLAGS --cflags`
        echo $c_flags
        CFLAGS="$CFLAGS $c_flags"

	dnl LDFLAGS next
        ld_flags=`$GET_WRASTER_FLAGS --ldflags`
	echo $ld_flags
        LDFLAGS="$LDFLAGS $ld_flags"
	
	dnl And finally libs
        libs_flags=`$GET_WRASTER_FLAGS --libs`
	echo $libs_flags
        LIBS="$LIBS $libs_flags"
	
else

	AC_MSG_ERROR("Danger Will Robinson: Unable to find get-wraster-flags in $PATH variable.");
fi

AC_CHECK_LIB(Xmu, XmuClientWindow)

dnl Support for extralibs
dnl ==============================================

extralibs=""

AC_ARG_WITH(extralibs,
[  --with-extralibs=LIBS    specify any libs the script doesn't detect e.g. -lmylib], extralibs=$withval )

if test "x$extralibs" != "x"; then
    LIBS="$extralibs $LIBS"
fi

AC_CHECK_LIB(Xft, XftFontOpen,,
             AC_MSG_ERROR([Xft library not found]))
AC_CHECK_LIB(WMaker, WMAppSetMainMenu,,
             AC_MSG_ERROR([WMaker library not found]))
AC_CHECK_LIB(WINGs, WMAppSetMainMenu,,
             AC_MSG_ERROR([WINGs library not found]))

dnl Checks for PropList (not necessary since version 0.2.3b)
dnl AC_CHECK_HEADERS(proplist.h,, AC_MSG_WARN(Can't find PropList include-file: Please install libPropList (included in WindowMaker)))
dnl AC_CHECK_LIB(PropList, PLGetProplistWithPath,, AC_MSG_WARN(Can't find PropList library: Please install libPropList (included in WindowMaker)), , $LIBS)

dnl Needed by Sun/Solaris
AC_CHECK_LIB(nsl, gethostbyname)
AC_CHECK_LIB(socket, socket)

dnl Needed by FreeBSD
AC_CHECK_LIB(kvm, kvm_read)

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(unistd.h)
AC_HEADER_DIRENT

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_STRUCT_TM

dnl Checks for library functions.
AC_CHECK_FUNCS(mktime strdup)

dnl Disable Clock in Inspector
dnl ==========================

clk=no
AC_ARG_ENABLE(clk,
[  --enable-clk            enable inspector clk (default = yes)],
                clk=$enableval, clk=yes)
AH_TEMPLATE([CLK],[enable inspector clk])
if test "$clk" = yes; then
    AC_DEFINE([CLK], [1])
    dnl AC_DEFINE(CLK, 1)
else
    AC_DEFINE([CLK], [0])
    dnl    AC_DEFINE(CLK, 0)	
fi


dnl Support for GNUSTEP_LOCAL_ROOT
dnl ==============================================

appspath=""

AC_ARG_WITH(appspath,
[  --with-appspath=PATH    specify the directory for GNUstep applications], appspath=$withval )

if test "x$appspath" = "x"; then
    gnustepdir="${prefix}/GNUstep"
dnl WMPrefs uses this method but it doesn't seem to work for me
dnl if there are problems with ICONDIR or fsviewerdir uncomment 
dnl the next line
dnl    gnustepdir='$(prefix)/GNUstep'

    if test "x$GNUSTEP_LOCAL_ROOT" != "x" ; then
        gnustepdir=`echo "$GNUSTEP_LOCAL_ROOT" | sed -e "s|^${prefix}|prefix|"`
        gnustepdir=`echo $gnustepdir | sed -e 's|^prefix|${prefix}|'`
    fi

    with_appspath=$gnustepdir/Apps
fi

fsviewerdir=$with_appspath/FSViewer.app
AC_SUBST(fsviewerdir)

AH_TEMPLATE([ICONDIR],[Where to put icon files])
AC_DEFINE_UNQUOTED([ICONDIR], "${fsviewerdir}")

dnl Support for NLS
dnl ==============================================
dnl directory where .mo (translation) files should be placed

LOCALEDIR="${prefix}/share/locale"

AC_SUBST(LOCALEDIR)
AH_TEMPLATE([LOCALEDIR],[Where to put locale files])
AC_DEFINE_UNQUOTED(LOCALEDIR, "$LOCALEDIR")


AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.15])

AC_OUTPUT(Makefile src/Makefile src/regexp/Makefile defs/Makefile \
  po/Makefile.in m4/Makefile tiff/Makefile xpm/Makefile man/Makefile)
