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
#include <unistd.h>
#ifndef HAVE_BUFFERED_AIO
#include <stdlib.h>
#include <fcntl.h>
#endif

#include "posix_aio.h"
#include "aio_syscalls.h"

int __aio_create_read(struct aiocb64 *__aiocbp)
{
	int	ret = 0;

#	ifndef HAVE_BUFFERED_AIO
	long fd_arg;
#	endif

	if (__aiocbp->aio_reqprio < 0) {
		errno = EINVAL;
		return -1;
	}

	if (__aiocbp->aio_nbytes < 0) {
		errno = EINVAL;
		return -1;
	}

	__aiocbp->__res = -EINPROGRESS;

	__aiocbp->__opcode = (short)IO_CMD_PREAD;
	__aiocbp->__reqprio = (short)__aiocbp->aio_reqprio;
	__aiocbp->offset = __aiocbp->aio_offset;

	/* Mark the aiocb as valid */
	__aiocbp->__valid = 1;

#	ifndef HAVE_BUFFERED_AIO

	fd_arg = fcntl(__aiocbp->aio_fildes, F_GETFL);

	if (!(fd_arg & O_DIRECT)) {
		fcntl(__aiocbp->aio_fildes, F_SETFL, fd_arg | O_DIRECT);
		__aiocbp->__gone_direct = 1;
	}

	if ( ((unsigned long)__aiocbp->aio_buf & 0x1FF) ||
	     (__aiocbp->aio_nbytes & 0x1FF) ) {
		/* Penalty here for not using an aligned buffer in the first place */
		__aiocbp->__nbytes = (__aiocbp->aio_nbytes + 0x1FF) & ~0x1FF;
		ret = posix_memalign(&__aiocbp->__buf, 0x200, __aiocbp->__nbytes);

		if (ret!= 0) {
			errno = EAGAIN;
			return -1;
		}
	} else {
		/* Good - user provided an aligned buffer with an aligned size */
		__aiocbp->__buf    = __aiocbp->aio_buf;
		__aiocbp->__nbytes = __aiocbp->aio_nbytes;
	}
	
#	else
	__aiocbp->__buf    = __aiocbp->aio_buf;
	__aiocbp->__nbytes = __aiocbp->aio_nbytes;
#	endif

	/* Check sigevent */
	switch (__aiocbp->aio_sigevent.sigev_notify) {

	case SIGEV_NONE:
		__aiocbp->sigevent = NULL;
		break;

#ifdef HAVE_AIO_EVENTS
	case SIGEV_SIGNAL:
	case SIGEV_THREAD_ID:
		__aiocbp->sigevent = &__aiocbp->aio_sigevent;
		break;

#ifdef HAVE_PTHREAD_H
	case SIGEV_THREAD:
		ret = __aio_thread_init(&__aiocbp->aio_sigevent);
		__aiocbp->sigevent = &__aiocbp->aio_sigevent;
		break;
#endif /* HAVE_PTHREAD_H */
#endif /* HAVE_AIO_EVENTS */

	default:
		errno = EINVAL;
		ret = -1;

		break;
	}

#ifndef HAVE_BUFFERED_AIO
	if (ret != 0) {
		/* Free allocated aligned buffer if needed */
		if (__aiocbp->__buf != __aiocbp->aio_buf) {
			free (__aiocbp->__buf);
			__aiocbp->__buf = __aiocbp->aio_buf;
		}
	}
#endif /* !HAVE_BUFFERED_AIO */

	return ret;
}

int aio_read64(struct aiocb64 *__aiocbp)
{
	int		ret;

	INIT_CTX

	if (__aiocbp == NULL) {
		errno = EINVAL;
		return -1;
	}

	ret = __aio_create_read(__aiocbp);

	if (ret != 0)
		return -1;

	ret = io_submit(__aio_default_context, 1 , (struct iocb **)&__aiocbp);

	if (ret < 0) {
#ifndef HAVE_BUFFERED_AIO
		/* Free allocated aligned buffer if needed */
		if (__aiocbp->__buf != __aiocbp->aio_buf) {
			free (__aiocbp->__buf);
			__aiocbp->__buf = __aiocbp->aio_buf;
		}
#endif /* !HAVE_BUFFERED_AIO */

		errno = -ret;
		return -1;
	}

	return 0;
}

int aio_read(struct aiocb *__aiocbp)
{
	struct aiocb64 *__paiocb = (struct aiocb64 *)__aiocbp;

	/* sign extend aio_offset to 64 bits */
	__paiocb->aio_offset = __aiocbp->aio_offset;

	return aio_read64(__paiocb);
}
