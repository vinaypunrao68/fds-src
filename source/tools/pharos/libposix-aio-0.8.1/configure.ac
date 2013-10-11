# Copyright (c) 2004-2005, BULL S.A. All rights reserved.
# Authors: 2004, Laurent Vivier <Laurent.Vivier@bull.net>
#          2005, Sebastien Dugue <Sebastien.Dugue@bull.net>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# See the COPYING.LESSER file at the top level of this source tree.

AC_INIT([libposix-aio],[0.8.1],[Sebastien.Dugue@bull.net])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE(libposix-aio, 0.8.1)

AC_PROG_LIBTOOL

# check for libpthread
AC_CHECK_HEADERS(pthread.h)
AC_CHECK_LIB(pthread, pthread_create,,AC_MSG_ERROR([libpthread is missing]))

# Save CFLAGS
save_cflags=$CFLAGS
CFLAGS="-D_GNU_SOURCE"

AC_SYS_LARGEFILE

# let users specify a path for the testfiles for the case
# where libposix-aio is built on a filesystem that does not
# support O_DIRECT
AC_MSG_CHECKING([where to put test files])
AC_ARG_WITH(
	[testdir],
	[AC_HELP_STRING([--with-testdir=<directory>],
			[Specify location were the test files are to be created
			 during configure.])],
	[ac_testdir="$with_testdir"],
	[ac_testdir="."])
AC_MSG_RESULT($ac_testdir)

# test if kernel supports default value for max events
AC_ARG_ENABLE(
	[default-maxevents],
	[AC_HELP_STRING([--enable-default-maxevents],
			[include support for kernel default max events])],
	ac_aio_default_maxevents=$enableval,
AC_MSG_CHECKING([for default value for max events])
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <unistd.h>
#include <sys/syscall.h>

typedef struct io_context *io_context_t;

int main()
{
	int ret;

	struct io_context *ctx = NULL;

	ret = syscall (SYS_io_setup, 0, &ctx);
	if (ret)
		return 1;

	syscall (SYS_io_destroy, ctx);
	return 0;
}
]])],[ac_aio_default_maxevents=yes
	AC_MSG_RESULT(yes)
	AC_DEFINE(HAVE_MAXEVENTS_DEFAULT,,
		  Define to 1 if kernel supports io_setup(0,...))],[ac_aio_default_maxevents=no
	AC_MSG_RESULT(no)
],[]))

# test if kernel supports signal on AIO completion

AC_ARG_ENABLE(
	[aio-signal],
	[AC_HELP_STRING([--enable-aio-signal],
			[include signal support on AIO completion])],
	ac_aio_signal_event=$enableval,
AC_MSG_CHECKING([for signal support on AIO completion])
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <unistd.h>
#include <malloc.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <endian.h>
#include <syscall.h>

#define IO_CMD_PWRITE 1

#if __WORDSIZE == 64
# define PADDED(__type, __name)	__type __name;
# elif __WORDSIZE == 32
#  if defined(__LITTLE_ENDIAN)
#   define PADDED(__type, __name) __type __name; int __pad_##__name
#  elif defined(__BIG_ENDIAN)
#   define PADDED(__type, __name) int __pad_##__name; \ __type __name
#  else
#   error edit for your odd byteorder.
# endif /* _ENDIAN */
#else
# error edit for your odd word size.
#endif /* __WORDSIZE */

struct iocb {
	PADDED(void *,		data);
	unsigned int		key;
	unsigned int		aio_reserved1;

	short			aio_lio_opcode;	
	short			aio_reqprio;
	int			aio_fildes;

	PADDED(volatile void *,	aio_buf);
	PADDED(unsigned long,	aio_nbytes);
	long long		offset;

	PADDED(struct sigevent *,sigevent);
	PADDED(unsigned long,	reserved);
};

struct io_event {
	PADDED(unsigned long,	data);
	PADDED(unsigned long,	obj);
	long long		res;
	long long		res2;
};

#define SIZE	1024

static struct io_context *ctx = NULL;
static int have_aio_signal = 1;

void sigrt1_handler(int signum, siginfo_t *info, void *context)
{
	struct io_event events;
	syscall (SYS_io_getevents,ctx, 1, 1, &events, NULL);
	have_aio_signal = 0;
}

int main()
{
	int ret;
	struct sigaction action;
	char *buf;
	int fd;
	struct sigevent event;
	struct iocb iocb;
	struct iocb *iocbs[[1]];

	action.sa_sigaction = sigrt1_handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = SA_SIGINFO|SA_RESTART;
	sigaction(SIGRTMIN+1, &action, NULL);

	buf = memalign(512, SIZE);
	if (buf == NULL) {
		perror("malloc");
		return 2;
	}

	fd = open("$ac_testdir/test_event", O_RDWR | O_CREAT | O_DIRECT, 0600);
	if (fd == -1) {
		perror("open");
		return 3;
	}
	unlink("test_event");

	ret = syscall (SYS_io_setup, 256, &ctx);
	if (ret) {
		perror("io_setup");
		return 4;
	}

	memset(&iocb, 0, sizeof(struct iocb));
	iocb.aio_fildes = fd;
	iocb.aio_lio_opcode = IO_CMD_PWRITE;
	iocb.aio_reqprio = 0;
	iocb.aio_buf = buf;
	iocb.aio_nbytes = SIZE;
	iocb.offset = 0;

	memset (&event, 0, sizeof (struct sigevent));
	event._sigev_un._tid = 0;
	event.sigev_signo = SIGRTMIN + 1;
	event.sigev_value.sival_ptr = &iocb;

	iocb.sigevent = &event;

	iocbs[[0]] = &iocb;
	ret = syscall (SYS_io_submit, ctx, 1, iocbs);
	if (ret != 1) {
		perror("io_submit");
		syscall (SYS_io_destroy, ctx);
		return 5;
	}

	sleep(1);

	syscall (SYS_io_destroy, ctx);

	return have_aio_signal;
}
]])],[ac_aio_signal_event=yes
	AC_MSG_RESULT(yes)
	AC_DEFINE(HAVE_AIO_EVENTS,,
		Define to 1 if kernel supports signal on AIO completion)],[ac_aio_signal_event=no
	AC_MSG_RESULT(no)
],[]))

# test if kernel supports signal on LIO completion

AC_ARG_ENABLE(
	[lio-signal],
	[AC_HELP_STRING([--enable-lio-signal],
			[include signal support on LIO completion])],
	ac_lio_signal_event=$enableval,
AC_MSG_CHECKING([for signal support on LIO completion])
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <unistd.h>
#include <malloc.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <endian.h>
#include <syscall.h>

#define IO_CMD_PWRITE 1
#define IO_CMD_GROUP 7

#if __WORDSIZE == 64
# define PADDED(__type, __name)	__type __name;
# elif __WORDSIZE == 32
#  if defined(__LITTLE_ENDIAN)
#   define PADDED(__type, __name) __type __name; int __pad_##__name
#  elif defined(__BIG_ENDIAN)
#   define PADDED(__type, __name) int __pad_##__name; \ __type __name
#  else
#   error edit for your odd byteorder.
# endif /* _ENDIAN */
#else
# error edit for your odd word size.
#endif /* __WORDSIZE */

struct iocb {
	PADDED(void *,		data);
	unsigned int		key;
	unsigned int		aio_reserved1;

	short			aio_lio_opcode;	
	short			aio_reqprio;
	int			aio_fildes;

	PADDED(volatile void *,	aio_buf);
	PADDED(unsigned long,	aio_nbytes);
	long long		offset;

	PADDED(struct sigevent *,sigevent);
	PADDED(unsigned long,	reserved);
};

struct io_event {
	PADDED(unsigned long,	data);
	PADDED(unsigned long,	obj);
	long long		res;
	long long		res2;
};

#define SIZE	1024

static struct io_context *ctx = NULL;
static int have_lio_signal = 1;

void sigrt1_handler(int signum, siginfo_t *info, void *context)
{
	struct io_event events;
	syscall (SYS_io_getevents, ctx, 1, 1, &events, NULL); 
	have_lio_signal = 0;
}

int main()
{
	int ret;
	struct sigaction action;
	char *buf;
	int fd;
	struct sigevent event;
	struct iocb iocb[[2]];
	struct iocb *iocbs[[2]];

	action.sa_sigaction = sigrt1_handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = SA_SIGINFO|SA_RESTART;
	sigaction(SIGRTMIN+1, &action, NULL);

	buf = memalign(512, SIZE);
	if (buf == NULL) {
		perror("malloc");
		return 2;
	}

	fd = open("$ac_testdir/test_event", O_RDWR | O_CREAT | O_DIRECT, 0600);
	if (fd == -1) {
		perror("open");
		return 3;
	}
	unlink("test_event");

	ret = syscall (SYS_io_setup, 256, &ctx);
	if (ret) {
		perror("io_setup");
		return 4;
	}

	iocbs[[0]] = &iocb[[0]];

	memset(&iocb[[0]], 0, sizeof(struct iocb));
	iocb[[0]].aio_fildes = 0;
	iocb[[0]].aio_lio_opcode = IO_CMD_GROUP;
	iocb[[0]].aio_reqprio = 0;
	iocb[[0]].aio_buf = NULL;
	iocb[[0]].aio_nbytes = 0;
	iocb[[0]].offset = 0;

	memset (&event, 0, sizeof (struct sigevent));
	event._sigev_un._tid = 0;
	event.sigev_signo = SIGRTMIN + 1;
	event.sigev_value.sival_ptr = iocbs[[0]];
	iocb[[0]].sigevent = &event;

	iocbs[[1]] = &iocb[[1]];

	memset(&iocb[[1]], 0, sizeof(struct iocb));
	iocb[[1]].aio_fildes = fd;
	iocb[[1]].aio_lio_opcode = IO_CMD_PWRITE;
	iocb[[1]].aio_reqprio = 0;
	iocb[[1]].aio_buf = buf;
	iocb[[1]].aio_nbytes = SIZE;
	iocb[[1]].offset = 0;
	iocb[[1]].sigevent = NULL;

	ret = syscall (SYS_io_submit, ctx, 2, iocbs);
	if (ret != 2) {
	  syscall (SYS_io_destroy, ctx);
	  perror("io_submit");
	  return 5;
	}

	sleep(1);

	syscall (SYS_io_destroy, ctx);

	return have_lio_signal;
}
]])],[ac_lio_signal_event=yes
	AC_MSG_RESULT(yes)
	AC_DEFINE(HAVE_LIO_EVENTS,1,
			Define to  1 if kernel supports LIO events)],[ac_lio_signal_event=no
	AC_MSG_RESULT(no)
],[]))

# test if kernel supports lio wait

AC_ARG_ENABLE(
	[lio-wait],
	[AC_HELP_STRING([--enable-lio-wait],
			[include support of LIO wait])],
	ac_lio_wait=$enableval,
AC_MSG_CHECKING([for support of LIO wait])
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <unistd.h>
#include <malloc.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <endian.h>
#include <syscall.h>

#define IO_CMD_PWRITE 1
#define IO_CMD_GROUP 7

#if __WORDSIZE == 64
# define PADDED(__type, __name)	__type __name;
# elif __WORDSIZE == 32
#  if defined(__LITTLE_ENDIAN)
#   define PADDED(__type, __name) __type __name; int __pad_##__name
#  elif defined(__BIG_ENDIAN)
#   define PADDED(__type, __name) int __pad_##__name; \ __type __name
#  else
#   error edit for your odd byteorder.
# endif /* _ENDIAN */
#else
# error edit for your odd word size.
#endif /* __WORDSIZE */

struct iocb {
	PADDED(void *,		data);
	unsigned int		key;
	unsigned int		aio_reserved1;

	short			aio_lio_opcode;	
	short			aio_reqprio;
	int			aio_fildes;

	PADDED(volatile void *,	aio_buf);
	PADDED(unsigned long,	aio_nbytes);
	long long		offset;

	PADDED(struct sigevent *,sigevent);
	PADDED(unsigned long,	reserved);
};

struct io_event {
	PADDED(unsigned long,	data);
	PADDED(unsigned long,	obj);
	long long		res;
	long long		res2;
};

#define SIZE	1024

static struct io_context *ctx = NULL;

int main()
{
	int ret;
	char *buf;
	int fd;
	struct iocb iocb[[2]];
	struct iocb *iocbs[[2]];
	struct io_event events;
	struct timespec ts = { 0, 0 };

	buf = memalign(512, SIZE);
	if (buf == NULL) {
		perror("malloc");
		return 2;
	}

	fd = open("$ac_testdir/test_event", O_RDWR | O_CREAT | O_DIRECT, 0600);
	if (fd == -1) {
		perror("open");
		return 3;
	}
	unlink("test_event");

	ret = syscall (SYS_io_setup, 256, &ctx);
	if (ret) {
		perror("io_setup");
		return 4;
	}

	iocbs[[0]] = &iocb[[0]];

	memset(iocbs[[0]], 0, sizeof(struct iocb));

	iocbs[[0]]->aio_fildes = 0;
	iocbs[[0]]->aio_lio_opcode = IO_CMD_GROUP;
	iocbs[[0]]->aio_reqprio = 0;
	iocb[[0]].aio_buf = NULL;
	iocb[[0]].aio_nbytes = 0;
	iocb[[0]].offset = 0;
	iocb[[0]].sigevent = NULL;

	iocbs[[1]] = &iocb[[1]];

	memset(iocbs[[1]], 0, sizeof(struct iocb));
	iocbs[[1]]->aio_fildes = fd;
	iocbs[[1]]->aio_lio_opcode = IO_CMD_PWRITE;
	iocbs[[1]]->aio_reqprio = 0;
	iocb[[1]].aio_buf = buf;
	iocb[[1]].aio_nbytes = SIZE;
	iocb[[1]].offset = 0;
	iocb[[1]].sigevent = NULL;

	ret = syscall (SYS_io_submit, ctx, 2, iocbs);
	if (ret != 2) {
		syscall (SYS_io_destroy, ctx);
		perror("io_submit");
		return 5;
	}
	ret = syscall (SYS_io_getevents, ctx, 1, 1, &events, &ts); 
	if (ret != 1) {
		return 6;
	}
	syscall (SYS_io_destroy, ctx);

	return 0;
}
]])],[ac_lio_wait=yes
	AC_MSG_RESULT(yes)
	AC_DEFINE(HAVE_LIO_WAIT,1,
			Define to 1 if kernel supports LIO wait)],[ac_lio_wait=no
	AC_MSG_RESULT(no)
],[]))

# check for support of fd for io_cancel

AC_ARG_ENABLE(
	[cancel-fd],
	[AC_HELP_STRING([--enable-cancel-fd],
			[include support of AIO cancel on file descriptor])],
	ac_aio_cancel_fd=$enableval,
AC_MSG_CHECKING([for AIO cancel on file descriptor])
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <unistd.h>
#include <endian.h>
#include <syscall.h>

#define IO_CMD_NOOP 6

#if __WORDSIZE == 64
# define PADDED(__type, __name)	__type __name;
# elif __WORDSIZE == 32
#  if defined(__LITTLE_ENDIAN)
#   define PADDED(__type, __name) __type __name; int __pad_##__name
#  elif defined(__BIG_ENDIAN)
#   define PADDED(__type, __name) int __pad_##__name; \ __type __name
#  else
#   error edit for your odd byteorder.
# endif /* _ENDIAN */
#else
# error edit for your odd word size.
#endif /* __WORDSIZE */

struct iocb {
	PADDED(void *,		data);
	unsigned int		key;
	unsigned int		aio_reserved1;

	short			aio_lio_opcode;	
	short			aio_reqprio;
	int			aio_fildes;

	PADDED(volatile void *,	aio_buf);
	PADDED(unsigned long,	aio_nbytes);
	long long		offset;

	PADDED(struct sigevent *,sigevent);
	PADDED(unsigned long,	reserved);
};

struct io_event {
	PADDED(unsigned long,	data);
	PADDED(unsigned long,	obj);
	long long		res;
	long long		res2;
};

static struct io_context *ctx = NULL;

int main()
{
	int ret;
	struct iocb a;

	ret = syscall (SYS_io_setup, 256, &ctx);
	if (ret) {
		perror("io_setup");
		return 1;
	}

	memset(&a, 0, sizeof(struct iocb));
	a.aio_fildes = STDIN_FILENO;
	a.aio_lio_opcode = IO_CMD_NOOP;
	a.aio_reqprio = 0;
	a.aio_buf = NULL;
	a.aio_nbytes = 0;
	a.offset = 0;
	a.sigevent = NULL;

	ret = syscall (SYS_io_cancel, ctx, &a, NULL);
	if (ret < 0)
		return 2;

	return 0;
}
]])],[ac_aio_cancel_fd=yes
	AC_MSG_RESULT(yes)
	AC_DEFINE(HAVE_AIO_CANCEL_FD, 1,
		Define to  1 if kernel supports AIO cancel on file descriptor)],[ac_aio_cancel_fd=no
	AC_MSG_RESULT(no)
],[]))

# check whether user enabled buffered AIO. Default for kernels up to 2.6.12 is
# to disable.

AC_MSG_CHECKING([for buffered AIO])
AC_ARG_ENABLE(
	[buffered-aio],
	[AC_HELP_STRING([--enable-buffered-aio],
			[disable user-space workaround to use default open
			 parameters (no O_DIRECT) and non-aligned buffer])],
	ac_buffered_aio=$enableval,
	ac_buffered_aio=no)

if test x$ac_buffered_aio = xyes; then
	AC_DEFINE(HAVE_BUFFERED_AIO, 1,
		[Define to  1 if kernel supports buffered AIO])
fi

AC_MSG_RESULT($ac_buffered_aio)

# Check debug
AC_ARG_ENABLE(
	debug,
	[AC_HELP_STRING([--enable-debug],
			[turn on debugging (Default off)])],
	ac_debug=yes,
	ac_debug=no)

AM_CONDITIONAL(DEBUG, test x$ac_debug = xyes)

# Restore CFLAGS
CFLAGS=$save_cflags


# Check whether we should install in EPREFIX/lib or EPREFIX/lib64
# x86_64 architectures place 64-bit libraries in EPREFIX/lib64
# ia64 architectures place 64-bit libraries in EPREFIX/lib
# ia32 only use EPREFIX/lib
AC_MSG_CHECKING([for library suffix to use])

case $host_cpu in

	x86_64*)
		lib_suffix="64"
		msg="64"
	;;

	*)
		lib_suffix=""
		msg="None"
	;;
esac

libdir="${libdir}${lib_suffix}"
AC_SUBST([libdir], ["$libdir"])
AC_MSG_RESULT($msg)



AC_CONFIG_FILES([Makefile
include/Makefile
src/Makefile
check/Makefile])
AC_OUTPUT

echo
echo "Native AIO features detected:"
echo "   Support for default value for max events: "${ac_aio_default_maxevents}
echo "   Support for AIO signal event: "${ac_aio_signal_event}
echo "   Support for LIO signal event: "${ac_lio_signal_event}
echo "   Support for LIO wait: "${ac_lio_wait}
echo "   Support for AIO cancel fd: "${ac_aio_cancel_fd}
echo "   Support for buffered AIO: "${ac_buffered_aio}
echo

echo "Debugging support: "${ac_debug}
echo

# Display final install path
while echo "$libdir" | grep "\\$"  > /dev/null 2>&1
do
  libdir=`eval echo "${libdir}"`
done

echo "Library will be installed in" $libdir
echo
