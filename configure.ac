# configure.ac for blassic

AC_INIT(version.cpp)

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

# Require autoconf >= 2.50
AC_PREREQ(2.50)

# Parse version.cpp to get the version number.
# The number is defined in version.cpp to avoid affecting the Windows
# compilation process.
#
# Julian: changed the "grep -w Major" for "grep 'version::Major'"
# because the -w option in not always present.

AC_MSG_CHECKING(for version number in $srcdir/version.cpp)

VERSION_MAJOR=`grep 'version::Major' $srcdir/version.cpp | \
				sed 's/.*version::.*= *//;s/[[,;]]//'`
VERSION_MINOR=`grep 'version::Minor' $srcdir/version.cpp | \
				sed 's/.*version::.*= *//;s/[[,;]]//'`
VERSION_RELEASE=`grep 'version::Release' $srcdir/version.cpp | \
				sed 's/.*version::.*= *//;s/[[,;]]//'`

AC_MSG_RESULT([got $VERSION_MAJOR.$VERSION_MINOR.$VERSION_RELEASE])

if test "_$VERSION_MAJOR" = "_"; then
	AC_MSG_ERROR([version::Major not found])
elif test "_$VERSION_MINOR" = "_"; then
	AC_MSG_ERROR([version::Minor not found])
elif test "_$VERSION_RELEASE" = "_"; then
	AC_MSG_ERROR([version::Release not found])
fi

AM_INIT_AUTOMAKE(blassic, $VERSION_MAJOR.$VERSION_MINOR.$VERSION_RELEASE)

# This is to check in the sources that configure has been used.
AC_DEFINE([BLASSIC_CONFIG])

# Checks for programs.
AC_PROG_CXX
AC_LANG_CPLUSPLUS

# Check if compiling for windows.
case "$host" in
*-*-cygwin*|*mingw*)
	HOST_WIN=1
	;;
*)
	;;
esac

###############################################################################

#	Option to disable graphics.

AC_ARG_ENABLE(graphics, AC_HELP_STRING(
	[--enable-graphics], [Use graphics [[yes]]] ) )
if test "$enable_graphics" = "no"; then
	AC_MSG_RESULT([not using graphics])
	AC_DEFINE(BLASSIC_CONFIG_NO_GRAPHICS)
else
	AC_MSG_RESULT([using graphics])
fi

###############################################################################

#	Option to disable curses / ncurses.

if test "_$HOST_WIN" != _1
then
	AC_ARG_ENABLE(curses, AC_HELP_STRING(
		[--enable-curses], [Use curses [[yes]]] ) )
	if test "$enable_curses" = "no"; then
		AC_MSG_RESULT([not using curses])
		AC_DEFINE(BLASSIC_CONFIG_NO_CURSES)
	else
		AC_MSG_RESULT([using curses])

		#	Option to disable ncurses, checking only curses

		AC_ARG_ENABLE(curses, AC_HELP_STRING(
			[--enable-ncurses],
			[Check ncurses before curses [[yes]]] ) )
		if test "$enable_ncurses" = "no"; then
			AC_MSG_RESULT([not checking ncurses])
			AC_DEFINE(BLASSIC_CONFIG_NO_NCURSES)
		else
			AC_MSG_RESULT([checking ncurses])
		fi

	fi
fi

###############################################################################

#	Option to enable svgalib (actually unsupported).

if test "_$HOST_WIN" != _1
then
	AC_ARG_ENABLE(svgalib, AC_HELP_STRING(
		[--enable-svgalib], [Use svgalib [[yes]]] ),
		[
			if test "_$enableval" = "_no"; then
				:
				AC_SUBST(SVGALIB_CFLAGS)
				AC_SUBST(SVGALIB_LIBS)
			else
				AM_PATH_SVGALIB(1.4.0)
			fi
		],
		[
			:
			AC_SUBST(SVGALIB_CFLAGS)
			AC_SUBST(SVGALIB_LIBS)
		]
	)
	if test "_$SVGALIB_LIBS" = "_"; then
		AC_MSG_RESULT([not using svgalib])
	fi
fi

###############################################################################

# Check for cross-compiling:

if test "$cross_compiling" = "yes"
then
	if test "$CXX_FOR_BUILD" = ""
	then
		CXX_FOR_BUILD="g++"
	fi
	AC_SUBST(CXX_FOR_BUILD,[$CXX_FOR_BUILD])
	if test "$CXXFLAGS_FOR_BUILD" = ""
	then
		CXXFLAGS_FOR_BUILD=$CXXFLGAS
	fi
	AC_SUBST(CXXFLAGS_FOR_BUILD,[$CXXFLAGS_FOR_BUILD])
else
	AC_SUBST(CXX_FOR_BUILD,[$CXX])
	AC_SUBST(CXXFLAGS_FOR_BUILD,[$CXXFLAGS])
fi

###############################################################################

#	Check headers and libs.


#	Compatibility with old versions of C++

AC_CHECK_HEADERS([cstdlib])


#	Cursor, graphics and printer.

case "$host" in
*-*-cygwin*|*mingw*)
	AC_SUBST(CYGWIN_FLAGS, ["-lgdi32 -lwsock32 -lwinspool"])

	# isatty can be defined here or in unistd.h
	AC_CHECK_HEADERS([io.h])
	;;
*)
	AC_SUBST(CYGWIN_FLAGS)

	# Check for sys/mman.h, needed for mmap.
	# This test seems to not work correctly when cross-compiling.
	#AC_FUNC_MMAP
	AC_CHECK_HEADERS([sys/mman.h], AC_DEFINE([HAVE_MMAP], 1) )


	# If the systems has libdl, use it.
	AC_CHECK_LIB(dl, dlopen)

	# Check for ncurses, if not found for curses.
	# Done in a complicated way to work in all possible cases
	# and avoid to link both libs.
	if test "$enable_curses" != "no"
	then
		if test "$enable_ncurses" != "no"
		then
			AC_MSG_NOTICE([checking if ncurses or curses can be used])
		else
			AC_MSG_NOTICE([checking if curses can be used])
		fi
		if test "$enable_ncurses" != "no"
		then
			ncurses_found=yes
			AC_CHECK_HEADERS([ncurses.h],,[ncurses_found=no])
			if test "$ncurses_found" = yes
			then
				AC_CHECK_LIB([ncurses],[tputs],,[ncurses_found=no])
				if test "$ncurses_found" = yes
				then
					AC_DEFINE(BLASSIC_CONFIG_USE_NCURSES)
					AC_MSG_NOTICE([using ncurses])
				else
					AC_MSG_WARN([ncurses header found but no lib])
				fi
			fi
		fi
		if test "$ncurses_found" != yes
		then
			if test "$enable_ncurses" != "no"
			then
				AC_MSG_NOTICE([ncurses not available, checking curses])
			fi
			curses_found=yes
			AC_CHECK_HEADERS([curses.h],,[curses_found=no])
			if test "$curses_found" = yes
			then
				AC_CHECK_LIB(curses,tputs,,[curses_found=no])
				if test "$curses_found" = yes
				then
					AC_DEFINE(BLASSIC_CONFIG_USE_CURSES)
					if test "$enable_ncurses" != "no"
					then
						AC_MSG_NOTICE([using curses])
					fi
				else
					AC_MSG_WARN([curses header found but no lib])
				fi
			fi
		fi
		if test "$ncurses_found" != yes && test "$curses_found" != yes
		then
			AC_MSG_NOTICE([not using curses])
		fi
		AC_CHECK_HEADERS([term.h])
	fi


	# Check X11.
	if test "$enable_graphics" != "no"
	then
		AC_PATH_XTRA
		if test "x$no_x" = xyes
		then
			AC_MSG_NOTICE([X not available or disabled, disabling graphics])
			AC_DEFINE(BLASSIC_CONFIG_NO_GRAPHICS)
		else
			xflags="$X_CFLAGS"
			xlibs="$X_LIBS"
			xadd="$X_PRE_LIBS -lX11 $X_EXTRA_LIBS"
		fi
	fi
	AC_SUBST([BL_X_CFLAGS], [$xflags] )
	AC_SUBST([BL_X_LIBS], [$xlibs] )
	AC_SUBST([BL_X_ADD], [$xadd] )

	AC_HEADER_SYS_WAIT

	# Check some variants used in debugging when available.
	AC_CHECK_MEMBERS([siginfo_t.si_ptr],,,[#include <signal.h>])

	;;
esac

# Check availability of hyperbolic trigonometric functions.

AC_CHECK_DECLS([asinh, acosh, atanh], , , [#include <math.h>] )


###############################################################################


AC_ARG_ENABLE(installed-examples, AC_HELP_STRING(
	[--enable-installed-examples],
	[Install the example programs [[yes]]] ),
	[
		if test "_$enableval" = "_no"; then
			INSTALL_EXAMPLE_PROGS=no
		else
			INSTALL_EXAMPLE_PROGS=yes
		fi
	],
	[
		INSTALL_EXAMPLE_PROGS=yes
	]
)

AM_CONDITIONAL(INSTALL_EXAMPLE_PROGS, [test "_$INSTALL_EXAMPLE_PROGS" = _yes])




###############################################################################

# Generate files.
AC_OUTPUT([
	Makefile
	blassic.spec
])
