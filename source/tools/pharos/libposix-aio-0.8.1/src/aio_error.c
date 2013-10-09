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

#include <stdio.h>
#include <errno.h>
#include "posix_aio.h"
#include "aio_syscalls.h"

#define MAX_EVENTS	16

int aio_error64(const struct aiocb64 *__aiocbp)
{
	long long res;
	int ret;

	INIT_CTX

	if (__aiocbp == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* Check if aiocb is valid */
	if (__aiocbp->__valid == 0) {
		errno = EINVAL;
		return -1;
	}

	ret = __aio_get_res(__aiocbp, &res);

	if (ret < 0)
		return ret;
	
	if (res < 0)
		ret = -res;
	else
		ret = 0;

	return ret;
}

int aio_error(const struct aiocb *__aiocbp)
{
	return aio_error64((struct aiocb64 *)__aiocbp);
}
