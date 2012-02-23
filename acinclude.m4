# Configure paths for SVGAlib
# Created by Pierre Sarrazin <http://sarrazip.com/>
# Tiny changes by Julian Albo.
# This file is in the public domain.
# Created from sdl.m4.

# This test must be executed in C mode (AC_LANG_C), not C++ mode.

# Defines SVGALIB_CFLAGS and SVGALIB_LIBS and submits them to AC_SUBST()

dnl AM_PATH_SVGALIB([MINIMUM-VERSION [,ACTION-IF-FOUND [,ACTION-IF-NOT-FOUND]]])
dnl Test for SVGAlib, and define SVGALIB_CFLAGS and SVGALIB_LIBS
dnl
AC_DEFUN(AM_PATH_SVGALIB,
[
  AC_ARG_WITH(svgalib-prefix,
  	AC_HELP_STRING([--with-svgalib-prefix=PFX],
  		[Prefix where SVGAlib is installed [[/usr]]] ),
    svgalib_prefix="$withval",
    svgalib_prefix="/usr"
    )
  AC_MSG_RESULT([using $svgalib_prefix as SVGAlib prefix])
  AC_ARG_ENABLE(svgalibtest,
  	AC_HELP_STRING([--disable-svgalibtest],
		[Do not try to compile and run a test program] ),
    ,
    enable_svgalibtest=yes
    )

dnl  Changed this check.
dnl  AC_REQUIRE([AC_CANONICAL_TARGET])
  AC_REQUIRE([AC_CANONICAL_HOST])
  min_svgalib_version=ifelse([$1], ,1.4.0, $1)
  AC_MSG_CHECKING([for SVGAlib - version >= $min_svgalib_version])
  no_svgalib=""


  if test "$svgalib_prefix" = /usr; then
    # Avoid the switch -I/usr/include - it might break gcc 3.x
    SVGALIB_CFLAGS=
  else
    SVGALIB_CFLAGS="-I$svgalib_prefix/include"
  fi

  SVGALIB_LIBS="-L$svgalib_prefix/lib -lvgagl -lvga"

  # Get the version number, which appears to be in the lib*.so.* filename:
  #
  if test -f $svgalib_prefix/lib/libvga.so.*.*.*; then

    _f=`echo $svgalib_prefix/lib/libvga.so.*.*.* | \
			sed 's/^.*\.so\.//'`
    svgalib_major_version=`echo $_f | \
			sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    svgalib_minor_version=`echo $_f | \
			sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    svgalib_micro_version=`echo $_f | \
			sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

    unset _f

  else
    AC_MSG_ERROR([could not find $svgalib_prefix/lib/libvga.so.*.*.*])
  fi

  if test "_$enable_svgalibtest" = "_yes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $SVGALIB_CFLAGS"
      LIBS="$LIBS $SVGALIB_LIBS"
dnl
dnl Check if the installed SVGAlib is sufficiently new.
dnl
      rm -f conf.svgalibtest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vga.h"

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = (char *)malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

int main (int argc, char *argv[])
{
  int major, minor, micro;
  char *tmp_version;

  /* This hangs on some systems (?)
  system ("touch conf.svgalibtest");
  */
  { FILE *fp = fopen("conf.svgalibtest", "a"); if ( fp ) fclose(fp); }

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_svgalib_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_svgalib_version");
     exit(1);
  }

  if (($svgalib_major_version > major) ||
      (($svgalib_major_version == major) && ($svgalib_minor_version > minor)) ||
      (($svgalib_major_version == major) && ($svgalib_minor_version == minor) && ($svgalib_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** Installed SVGAlib version is %d.%d.%d, but the minimum version\n", $svgalib_major_version, $svgalib_minor_version, $svgalib_micro_version);
      printf("*** of SVGAlib required is %d.%d.%d.\n", major, minor, micro);
      printf("*** It is best to upgrade to the required version.\n");
      printf("*** The web site is http://www.svgalib.org/\n");
      printf("*** See also --with-svgalib-prefix\n");
      return 1;
    }
}

],
      ,
      no_svgalib=yes,
      [echo $ac_n "cross compiling; assumed OK... $ac_c"]
      )

      CFLAGS="$ac_save_CFLAGS"
      LIBS="$ac_save_LIBS"
  fi


  if test "_$no_svgalib" = _ ; then
    AC_MSG_RESULT([found $svgalib_major_version.$svgalib_minor_version.$svgalib_micro_version])
    ifelse([$2], , :, [$2])     
  else
    AC_MSG_ERROR([SVGAlib >= min_svgalib_version not found])

    if test -f conf.svgalibtest ; then
      :
    else
      echo "*** Could not run SVGAlib test program, checking why..."

      CFLAGS="$CFLAGS $SVGALIB_CFLAGS"
      LIBS="$LIBS $SVGALIB_LIBS"

      AC_TRY_LINK([
#include <stdio.h>
#include "vga.h"

int main(int argc, char *argv[])
{ return 0; }
#undef  main
#define main K_and_R_C_main
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding SVGAlib or finding the wrong"
          echo "*** version of SVGAlib. If it is not finding SVGAlib, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"
	],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means SVGAlib was incorrectly installed"
          echo "*** or that you have moved SVGAlib since it was installed."
          echo "***"
          echo "*** SVGAlib web site: http://www.svgalib.org/"
	]
      )

      CFLAGS="$ac_save_CFLAGS"
      LIBS="$ac_save_LIBS"

      AC_MSG_ERROR([SVGAlib >= min_svgalib_version not found])

    fi

    SVGALIB_CFLAGS=""
    SVGALIB_LIBS=""
    ifelse([$3], , :, [$3])
  fi

  AC_SUBST(SVGALIB_CFLAGS)
  AC_SUBST(SVGALIB_LIBS)
  rm -f conf.svgalibtest
])
