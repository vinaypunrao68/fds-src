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

int aio_cancel64(int __fildes, struct aiocb64 *__aiocbp)
{
	struct io_event	event;
	int		ret;

	INIT_CTX

	if (__aiocbp == NULL) {
		/* cancel all operations against fildes */
#ifdef HAVE_AIO_CANCEL_FD

		struct aiocb tmp_aiocb;

		memset(&tmp_aiocb, 0, sizeof(struct aiocb));
		tmp_aiocb.aio_fildes = __fildes;
		tmp_aiocb.__opcode = IO_CMD_NOOP;
		tmp_aiocb.__reqprio = 0;
		tmp_aiocb.__buf = NULL;
		tmp_aiocb.__nbytes = 0;
		tmp_aiocb.offset = 0;
		tmp_aiocb.sigevent = NULL;

		ret = io_cancel(__aio_default_context,
				(struct iocb *)&tmp_aiocb, NULL);
	
		if (ret < 0) {
			errno = -ret;
			return -1;
		}

		return ret;
#else
		return AIO_NOTCANCELED;
#endif
	}

	ret = io_cancel(__aio_default_context,
			(struct iocb *)__aiocbp, &event);

	if ( (ret == -EINVAL) && (__aiocbp->__res != EINPROGRESS) )
		return AIO_ALLDONE;

	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	__aio_set_res(__aiocbp, event.res);

	return AIO_CANCELED;
}

int aio_cancel(int __fildes, struct aiocb *__aiocbp)
{
	return aio_cancel64(__fildes, (struct aiocb64 *)__aiocbp);
}
