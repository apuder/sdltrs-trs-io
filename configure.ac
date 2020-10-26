AC_PREREQ([2.61])
AC_INIT([sdltrs], [1.2.13], [dbotw@gmx.net])
AC_CONFIG_SRCDIR([src/trs.h])
AM_INIT_AUTOMAKE([subdir-objects])
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_MAKE_SET
AC_C_BIGENDIAN([AC_DEFINE([big_endian])])
AC_CANONICAL_HOST
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_FUNC_ERROR_AT_LINE
AC_CHECK_HEADERS([fcntl.h limits.h sys/file.h sys/ioctl.h sys/time.h termios.h])
AC_CHECK_FUNCS([ftruncate getcwd gettimeofday memmove memset putenv strcasecmp strchr strdup strerror strncasecmp strrchr strtol])

case $host_os in
  *mingw*)
    AC_DEFINE([WIN32])
    ;;
  *)
    AS_IF([test "x$with_x" != "xno"],
    [AC_PATH_X
     AC_PATH_XTRA
     CFLAGS="$CFLAGS $X_CFLAGS"
     LDFLAGS="$LDFLAGS $X_LIBS"
     AC_CHECK_LIB([X11], [XOpenDisplay], [], AC_DEFINE([NOX]))],
    [AC_DEFINE([NOX])])
    ;;
esac

AC_ARG_ENABLE([fastmove],
  [AS_HELP_STRING([--enable-fastmove], [faster but not accurate Z80 block moves])],
  [AC_DEFINE([FAST_MOVE])
   AC_MSG_NOTICE([faster but not accurate Z80 block moves enabled])])

AC_ARG_ENABLE([readline],
  [AS_HELP_STRING([--enable-readline], [readline support for zbx debugger])],
  [AC_SEARCH_LIBS(tgetent, curses ncurses ncursesw, [], [])
   AC_SEARCH_LIBS([readline], [readline], AC_DEFINE([READLINE]),
   AC_MSG_ERROR([Unable to find readline function]))])

AC_ARG_ENABLE([sdl1],
  [AS_HELP_STRING([--enable-sdl1], [SDL1 version])],
  [SDL_VERS=SDL
   SDL_CONF=sdl-config],
  [SDL_VERS=SDL2
   SDL_CONF=sdl2-config
   AC_CHECK_PROG(SDL_CHECK, $SDL_CONF, yes)
   AS_IF([test x"$SDL_CHECK" != x"yes"],
     [AC_MSG_WARN([$SDL_VERS not found, falling back to SDL1])
      SDL_VERS=SDL
      SDL_CONF=sdl-config],
     [AC_DEFINE([SDL2])])])

AC_ARG_ENABLE([zbx],
  [AS_HELP_STRING([--enable-zbx], [build with integrated Z80 debugger zbx])],
  [AC_DEFINE([ZBX])])

CFLAGS="$CFLAGS $( $SDL_CONF --cflags )"
LIBS="$LIBS $( $SDL_CONF --libs )"

AS_IF([test "x$enable_zbx" = "xyes"],
  [LIBS="$( echo $LIBS | sed 's/ -mwindows//' )"],
  [AC_MSG_NOTICE([integrated Z80 debugger zbx DISABLED])])

AC_CHECK_LIB([$SDL_VERS], [SDL_Init], [:],
  AC_MSG_ERROR([Unable to find SDL_Init function in $SDL_VERS]))

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
