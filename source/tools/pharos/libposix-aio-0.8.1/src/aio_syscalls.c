/*
 * Copyright (c) 2004-2005, BULL S.A. All rights reserved.
 * Authors: 2004, Laurent Vivier <Laurent.Vivier@bull.net>
 *          2005, Sebastien Dugue <Sebastien.Dugue@bull.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * See the COPYING.LESSER file at the top level of this source tree.
 */

#include "posix_aio.h"

#if defined(__i386__)
#include "syscall_x86.h"
#elif defined(__x86_64__)
#include "syscall_x86_64.h"
#elif defined(__ia64__)
#include "syscall_ia64.h"
#else
#error "No definition for your architecture"
#endif



aio_syscall2(int, io_setup, int, maxevents, io_context_t *, ctxp)

aio_syscall1(int, io_destroy, io_context_t, ctx)

aio_syscall3(int, io_submit, io_context_t, ctx, long, nr, struct iocb **, iocbs)

aio_syscall3(int, io_cancel, io_context_t, ctx, struct iocb *, iocb, struct io_event *, event)

aio_syscall5(int, io_getevents, io_context_t, ctx, long, min_nr, long, nr, struct io_event *,
	     events, struct timespec *, timeout)
