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

#include <malloc.h>
#include <errno.h>
#include "posix_aio.h"
#include "aio_syscalls.h"

int aio_fsync64(int __op, struct aiocb64 *__aiocbp)
{
	int ret;

	INIT_CTX

	if (__aiocbp == NULL) {
		errno = EINVAL;
		return -1;
	}

	__aiocbp->aio_reqprio = 0;

	/* Mark the aiocb as valid */
	__aiocbp->__valid = 1;

	switch(__op) {
	case O_SYNC:
		__aiocbp->__opcode = IO_CMD_FSYNC;

		ret = io_submit(__aio_default_context, 1, (struct iocb **)&__aiocbp);

		break;
#if O_DSYNC != O_SYNC
	case O_DSYNC:
		__aiocbp->__opcode = IO_CMD_FDSYNC;

		ret = io_submit(__aio_default_context, 1, (struct iocb **)&__aiocbp);

		break;
#endif
	default:
		ret = -EINVAL;
		break;
	}

	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	return 0;
}

int aio_fsync(int __op, struct aiocb *__aiocbp)
{
	return aio_fsync64(__op, (struct aiocb64 *)__aiocbp);
}
