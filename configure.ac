AC_PREREQ([2.59])
AC_INIT([ax25-tools],[0.0.10-rc5],[linux-hams@vger.kernel.org])
AC_CONFIG_SRCDIR(ax25/ax25d.c)
AM_INIT_AUTOMAKE([1.7])

AC_CONFIG_HEADERS(config.h)

dnl Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL

dnl Check for X and set a conditional
AC_PATH_XTRA
AM_CONDITIONAL(HAVE_X, test -z "$no_x")

dnl Checks for libraries.
AC_SUBST(Z_LIB)
AC_SUBST(AX25_LIB)
AC_SUBST(UTIL_LIB)
AC_SUBST(FLTK_LIB)
AC_CHECK_LIB(z, zlibVersion,Z_LIB="-lz",Z_LIB=)
AC_CHECK_LIB(ax25, ax25_config_load_ports, AX25_LIB="-lax25", AC_MSG_ERROR([Could not find the libax25 libraries; aborting]))
dnl openpty is in libutil on Linux but in libc on other systems.
AC_CHECK_FUNC(openpty, UTIL_LIB=, AC_CHECK_LIB(util, openpty, UTIL_LIB="-lutil", AC_MSG_ERROR([Could not find libutil; aborting])))
AC_CHECK_LIB([fltk], [main], [FLTK_LIB=`fltk-config --ldflags`], FLTK_LIB=)
AM_CONDITIONAL([HAVE_FLTK], [test -n "$FLTK_LIB"])

dnl The kernel dependent programs configure the Linux AX.25 stack through
dnl ioctls and headers that exist nowhere else - SIOCADDRT, SIOCSIFHWADDR,
dnl linux/bpqether.h and their like.  This asks whether they can be built
dnl here, not whether a stack is running: on a Linux without the ax25
dnl module loaded they still build, and libax25 decides per process at run
dnl time whether AF_AX25 goes to the kernel or to AGWPE.  The programs that
dnl work through libax25 rather than through those ioctls are outside the
dnl conditional and are built everywhere.  Same test as in libax25 and
dnl ax25-apps.
AC_MSG_CHECKING([whether native kernel AX.25 support is available])
case `uname -s` in
	Linux) ac_have_kernel_ax25=yes ;;
	*)     ac_have_kernel_ax25=no ;;
esac
AC_MSG_RESULT([$ac_have_kernel_ax25])
AM_CONDITIONAL(HAVE_KERNEL_AX25, test "x$ac_have_kernel_ax25" = "xyes")

dnl Checks for header files.
dnl AC_PATH_X
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h limits.h paths.h strings.h sys/file.h sys/ioctl.h sys/time.h syslog.h unistd.h zlib.h)

dnl Checks for working glibc 2.1 headers
AC_CHECK_TYPES([struct ax25_fwd_struct], [],
	[AC_MSG_ERROR([Both glibc and libax25 are too old or both installed])],
	[[#include <netax25/ax25.h>]])
AC_CHECK_MEMBER([struct nr_route_struct.ndigis], [],
	[AC_MSG_ERROR([Both glibc and libax25 are too old or both installed])],
	[[#include <netrom/netrom.h>]])
AC_CHECK_TYPES([struct rose_facilities_struct], [],
	[AC_MSG_ERROR([Both glibc and libax25 are too old or both installed])],
	[[#include <netax25/ax25.h>	/* Or <netrose/rose.h> will blow up */
	#include <netrose/rose.h>
	]])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_UID_T
AC_TYPE_PID_T
AC_HEADER_TIME
AC_STRUCT_TM

dnl Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MEMCMP
AC_FUNC_STRFTIME
AC_FUNC_WAIT3
AC_CHECK_FUNCS(gethostname gettimeofday mkdir select socket strdup strerror strspn strstr strtol strtoul uname)

dnl Only use -Wall if we have gcc
if test "x$GCC" = "xyes"; then
  if test -z "`echo "$CFLAGS" | grep "\-Wall" 2> /dev/null`" ; then
    CFLAGS="$CFLAGS -Wall"
  fi
fi

basever=$(echo $PACKAGE_VERSION | sed -e 's@-.*$@@')
AC_SUBST(BASEVERSION, [$basever])
extraver=$(echo $PACKAGE_VERSION | sed -e 's@@<:@^-@:>@*-@@' -e 's@-@_@')
AC_SUBST(EXTRAVERSION, [$extraver])

AC_CONFIG_FILES([netrom/Makefile tcpip/Makefile ax25/Makefile ax25/axgetput/Makefile Makefile rose/Makefile user_call/Makefile kiss/Makefile 6pack/Makefile hdlcutil/Makefile hdlcutil/fl/Makefile yamdrv/Makefile dmascc/Makefile ax25-tools.spec])
AC_OUTPUT
