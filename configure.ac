AC_PREREQ([2.61])
AC_INIT([sdltrs], [1.2.0], [dbotw@gmx.net])
AC_CONFIG_SRCDIR([src/trs.h])
AM_INIT_AUTOMAKE([subdir-objects])
AC_PROG_CC
AM_PROG_CC_C_O
AC_C_BIGENDIAN([AC_DEFINE([big_endian])])
AC_CANONICAL_HOST

case $host_os in
  *mingw*)
    AC_DEFINE([WIN32])
    ;;
  *)
    AS_IF([test "x$with_x" != "xno"],
    [AC_PATH_XTRA
     CFLAGS="$CFLAGS $X_CFLAGS"
     LDFLAGS="$LDFLAGS $X_LIBS -lX11"],
    [AC_DEFINE([NOX])])
    ;;
esac

AC_ARG_ENABLE([readline],
  [AS_HELP_STRING([--enable-readline], [readline support for zbx debugger])],
  [AC_SEARCH_LIBS(tgetent, curses ncurses ncursesw, [], [])
   AC_SEARCH_LIBS([readline], [readline], AC_DEFINE([READLINE]),
   AC_MSG_ERROR([Unable to find readline function]))])

AC_ARG_ENABLE([sdl2],
  [AS_HELP_STRING([--enable-sdl2], [Experimental SDL2 version])],
  [AC_DEFINE([SDL2])
   SDL_VERS=SDL2
   SDL_CONF=sdl2-config],
  [SDL_VERS=SDL
   SDL_CONF=sdl-config])

CFLAGS="$CFLAGS $( $SDL_CONF --cflags )"
LDFLAGS="$LDFLAGS $( $SDL_CONF --libs )"

AC_CHECK_LIB([$SDL_VERS],[SDL_Init], [:],
  AC_MSG_ERROR([Unable to find SDL_Init function in $SDL_VERS]))

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
