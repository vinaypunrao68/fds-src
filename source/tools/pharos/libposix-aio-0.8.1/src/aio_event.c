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

#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#ifndef HAVE_BUFFERED_AIO
#include <stdlib.h>
#include <fcntl.h>
#endif

#include "posix_aio.h"
#include "aio_syscalls.h"

#define MAX_EVENTS	256


static int update_events(struct timespec *__timeout)
{
	int		i;
	struct io_event events[MAX_EVENTS];
	int		ret;
	sigset_t	new, old;

	/* we must mask interrupt because an aio_error() or aio_return()
	 * can be interrupted by a function calling aio_error/aio_return
	 * and at this moment, the result could be neither in the kernel nor
	 * in the aiocb.
	 *
	 * A solution could be if kernel updates directly the iocb with
	 * its result (like it seems it is done in solaris and aix)
	 *
	 */

	sigfillset(&new);
	sigprocmask(SIG_BLOCK, &new, &old);

	ret = io_getevents(__aio_default_context, 1, MAX_EVENTS, events,
			   __timeout);
	if (ret < 0)
		goto out;

	for (i = 0; i < ret; i++) {
		struct aiocb64 *__aiocbp = (struct aiocb64 *)events[i].obj;

		if (__aiocbp) {
			__aiocbp->__res = events[i].res;
		}
	}

out:
	sigprocmask(SIG_SETMASK, &old, NULL);
	return ret;
}

int __aio_wait_one(int __nent, struct aiocb64 *__const __list[], 
		       const struct timespec *__timeout)
{
	struct timespec ts;
	struct timespec *timeout;
	int i;
	int ret;

	if (__timeout) {
		timeout = &ts;
		memcpy(timeout, __timeout, sizeof(ts));
	} else {
		timeout = NULL;
	}

	while (1) {
		for (i = 0; i < __nent; i++)
			if ( (__list[i] != NULL) && 
			     (__list[i]->__res != -EINPROGRESS) )
				return 0;

		ret = update_events(timeout);
		if ( (ret == -ETIMEDOUT) || (ret == 0) ) {
			errno = EAGAIN;
			return -1;
		}

		if (ret < 0) {
			errno = -ret;
			return -1;
		}
	}

	return 0;
}

#ifndef HAVE_LIO_WAIT
int __aio_wait_all(int __nent, struct aiocb64 *const __list[], 
		       const struct timespec *__timeout)
{
	struct timespec ts;
	struct timespec *timeout;
	int i;
	int ret;
	int all_done = 0;

	if (__timeout) {
		timeout = &ts;
		memcpy(timeout, __timeout, sizeof(ts));
	} else {
		timeout = NULL;
	}

	while (1) {

		all_done = 1;
		for (i = 0; i < __nent; i++) {

			if (__list[i] == NULL)
				continue;

			if (__list[i]->__res == -EINPROGRESS)
				all_done = 0;
		}

		if (all_done)
			break;

		ret = update_events(timeout);
		if ( (ret == -ETIMEDOUT) || (ret == 0) ) {
			errno = EAGAIN;
			return -1;
		}

		if (ret < 0) {
			errno = -ret;
			return -1;
		}
	}

	return 0;
}
#endif /* HAVE_LIO_WAIT */

int __aio_get_res(const struct aiocb64 *__aiocbp, long long *res)
{
	struct timespec ts = { 0, 0 };
	int ret;
#	ifndef HAVE_BUFFERED_AIO
	long fd_arg;
#	endif

	/*
	 *  As update_events may fetch more than one completion event at each
	 * call, the event we're interested in may already be available so we
	 * can avoid a few syscalls here.
	 */
	if (__aiocbp->__res == -EINPROGRESS) {
		/* event not fetched yet so go get it */
		ret = update_events(&ts);

		if (ret < 0) {
			errno = -ret;
			return -1;
		}
	}

	*res = __aiocbp->__res;

#	ifndef HAVE_BUFFERED_AIO

	/* If transaction completed without error */
	if (__aiocbp->__res >= 0) {
 
		/* Check if we allocated an aligned buffer for the request */
		if (__aiocbp->aio_buf != __aiocbp->__buf) {
			/* FIXME: should we use a lock ? */

			/*
			 * If read size exceeds requested size, we don't need the extra
			 * and the user probably don't want to be told we read more
			 */
			if (__aiocbp->__res > __aiocbp->aio_nbytes) {
				*res = __aiocbp->aio_nbytes;
				/* Update the aiocb res field */
				((struct aiocb64 *)__aiocbp)->__res = __aiocbp->aio_nbytes;
			}

			if (__aiocbp->__opcode == IO_CMD_PREAD) {

				/* Copy into the unaligned user buffer */
				memcpy(__aiocbp->aio_buf, (void*)__aiocbp->__buf,
				       __aiocbp->__res);
			}

			/* Free the allocated aligned buffer */
			free (__aiocbp->__buf);
			((struct aiocb64 *)__aiocbp)->__buf = __aiocbp->aio_buf;
		}

		/*
		 * If we went O_DIRECT in aio_read or aio_write revert back
		 * to buffered
		 */
		if (__aiocbp->__gone_direct != 0) {
			fd_arg = fcntl(__aiocbp->aio_fildes, F_GETFL);
			fcntl(__aiocbp->aio_fildes, F_SETFL, fd_arg & ~O_DIRECT);

			/* Clear flag - we need to override const here */
			((struct aiocb64 *)__aiocbp)->__gone_direct = 0;
		}

	/* An error occured - just free the allocated buffer*/
	} else if (__aiocbp->__res != -EINPROGRESS) {

		if (__aiocbp->aio_buf != __aiocbp->__buf) {
			free (__aiocbp->__buf);
			((struct aiocb64 *)__aiocbp)->__buf = __aiocbp->aio_buf;
		}

		/*
		 * If we went O_DIRECT in aio_read or aio_write revert back
		 * to buffered
		 */
		if (__aiocbp->__gone_direct != 0) {
			fd_arg = fcntl(__aiocbp->aio_fildes, F_GETFL);
			fcntl(__aiocbp->aio_fildes, F_SETFL, fd_arg & ~O_DIRECT);

			/* Clear flag - we need to override const here */
			((struct aiocb64 *)__aiocbp)->__gone_direct = 0;
		}

	}
#	endif /* HAVE_BUFFERED_AIO */

	return 0;
}

void __aio_set_res(struct aiocb64 *__aiocbp, long long res)
{
	__aiocbp->__res = res;
}
