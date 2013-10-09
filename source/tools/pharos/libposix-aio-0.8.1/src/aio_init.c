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
#include "aio_syscalls.h"

#ifndef HAVE_MAXEVENTS_DEFAULT
/* user can modify it before first call */
int __aio_maxevents_default = 1024;
#endif

io_context_t __aio_default_context = 0;

int aio_setup(int maxevents)
{
#	ifdef HAVE_MAXEVENTS_DEFAULT
	errno = EINVAL;
	return -1;
#	else
	__aio_maxevents_default = maxevents;
	return 0;
#	endif
}

void __attribute__ ((constructor)) aio_init(void)
{
#	ifdef HAVE_MAXEVENTS_DEFAULT
	io_setup(0, &__aio_default_context);
#	else
	__aio_default_context = NULL;
#	endif /* HAVE_MAXEVENTS_DEFAULT */
}
