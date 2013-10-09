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

#include <errno.h>
#include "posix_aio.h"
#include "aio_syscalls.h"

int aio_suspend64(__const struct aiocb64 *__const __list[__restrict_arr],
		  int __nent, __const struct timespec *__timeout)
{
	int 		ret;

	INIT_CTX

	if (__list == 0) {
		errno = EINVAL;
		return -1;
	}

	ret = __aio_wait_one(__nent, (struct aiocb64 *__const*)__list, __timeout);

	return ret;
}

int aio_suspend(__const struct aiocb *__const __list[__restrict_arr],
                       int __nent, __const struct timespec *__timeout)
{
	return aio_suspend64((__const struct aiocb64 *__const*)__list, 
			     __nent, __timeout);
}
