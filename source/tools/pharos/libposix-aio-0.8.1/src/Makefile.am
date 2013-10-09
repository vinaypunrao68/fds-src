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

lib_LTLIBRARIES = libposix-aio.la

noinst_HEADERS = posix_aio.h aio_syscalls.h syscall_x86.h syscall_ia64.h \
		 syscall_x86_64.h 

libposix_aio_la_SOURCES = aio_syscalls.c aio_cancel.c aio_error.c aio_fsync.c \
			  aio_read.c aio_return.c aio_suspend.c aio_write.c   \
			  lio_listio.c aio_init.c aio_thread_create.c aio_event.c

if DEBUG
AM_CFLAGS	= -g -Wall -Werror -fno-strict-aliasing -D_GNU_SOURCE
else
AM_CFLAGS	= -O3 -Wall -Werror -fno-strict-aliasing -D_GNU_SOURCE
endif

clean-local:
	rm -f *~
