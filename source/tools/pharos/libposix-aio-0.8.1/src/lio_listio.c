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
#include <malloc.h>
#include <errno.h>
#include "posix_aio.h"
#include "aio_syscalls.h"

int lio_listio64(int __mode, struct aiocb64 *__const __list[__restrict_arr],
		 int __nent, struct sigevent *__restrict __sig)
{
	struct aiocb64	**iocbs;
	int		ret = 0;
	int		current_iocb;
	int		current_aiocb;
	int		nent_aiocb;
	int		globret;
	int		i, j;

	INIT_CTX

	if  ( ((__mode != LIO_WAIT) && (__mode != LIO_NOWAIT)) ||
	      (__list == NULL) || (__nent <= 0) ) {
		errno = EINVAL;
		return -1;
	}

	nent_aiocb = __nent;

#	ifdef HAVE_LIO_WAIT
	if (__mode  == LIO_WAIT) {
		/* Add an iocb for the wait opcode (IO_CMD_CHECKPOINT) */
		nent_aiocb++;
	} else {
#	endif /* HAVE_LIO_WAIT */
		if (__sig) {
#	ifdef HAVE_LIO_EVENTS
			/* Add an iocb for list completion notification */
			nent_aiocb++;
#	else
			errno = EINVAL;
			return -1;
#	endif /* HAVE_LIO_EVENTS */
		}

#	ifdef HAVE_LIO_WAIT
	}
#	endif /* HAVE_LIO_WAIT */

	/*
	 * Alloc new list
	 *
	 * SDU :  maybe this could benefit from glibc independent_calloc to allocate
	 *        everything in one sweep.
	 */
	iocbs = (struct aiocb64**)calloc(nent_aiocb, sizeof(struct aiocb64 *));

	if (iocbs == NULL) {
		errno = EAGAIN;
		return -1;
	}

	/*
	 * Alloc new entries if needed
	 */
	for (i=0; i<(nent_aiocb - __nent); i++) {
		iocbs[i] = calloc (1, sizeof(struct aiocb64));

		if (iocbs[i] == NULL) {
			/* Free all aiocbs allocated so far */
			for (j=0; j<i; j++)
				free (iocbs[i]);

			/* Free list */
			free (iocbs);

			errno = EAGAIN;
			return -1;
		}
	}

	globret = 0;
	current_iocb = 0;

#	ifdef HAVE_LIO_WAIT
	if (__mode  == LIO_WAIT) {
		/* Init checkpoint iocb */
		iocbs[current_iocb]->aio_fildes = 0;
		iocbs[current_iocb]->__opcode = IO_CMD_GROUP;
		iocbs[current_iocb]->__reqprio = 0;
		iocbs[current_iocb]->__buf = NULL;
		iocbs[current_iocb]->__nbytes = 0;
		iocbs[current_iocb]->offset = 0;

		iocbs[current_iocb]->sigevent = NULL;

		current_iocb++;
	}
#	endif /* HAVE_LIO_WAIT */
#	ifdef HAVE_LIO_EVENTS
	if (__sig && (__mode != LIO_WAIT)) {
		/* Init list completion notification iocb */
		iocbs[current_iocb]->aio_fildes = 0;
		iocbs[current_iocb]->__opcode = IO_CMD_GROUP;
		iocbs[current_iocb]->__reqprio = 0;
		iocbs[current_iocb]->__buf = NULL;
		iocbs[current_iocb]->__nbytes = 0;
		iocbs[current_iocb]->offset = 0;
		iocbs[current_iocb]->aio_sigevent._sigev_un._tid = 0;
		iocbs[current_iocb]->aio_sigevent.sigev_signo = 0;
		iocbs[current_iocb]->aio_sigevent.sigev_value.sival_int = 0;

		/* Setup sigevent for list completion */
		switch (__sig->sigev_notify) {
		case SIGEV_NONE:
			iocbs[current_iocb]->sigevent =
				&iocbs[current_iocb]->aio_sigevent;
			break;

#ifdef HAVE_AIO_EVENTS
		case SIGEV_SIGNAL:
			iocbs[current_iocb]->aio_sigevent.sigev_value.sival_int =
				__sig->sigev_value.sival_int;
			iocbs[current_iocb]->aio_sigevent.sigev_signo =
				__sig->sigev_signo;
			iocbs[current_iocb]->aio_sigevent.sigev_notify =
				__sig->sigev_notify;
			iocbs[current_iocb]->aio_sigevent._sigev_un._tid = 0;

			iocbs[current_iocb]->sigevent =
				&iocbs[current_iocb]->aio_sigevent;
			break;

		case SIGEV_THREAD_ID:
			iocbs[current_iocb]->aio_sigevent.sigev_value.sival_int =
				__sig->sigev_value.sival_int;
			iocbs[current_iocb]->aio_sigevent.sigev_signo =
				__sig->sigev_signo;
			iocbs[current_iocb]->aio_sigevent.sigev_notify =
				__sig->sigev_notify;
			iocbs[current_iocb]->aio_sigevent._sigev_un._tid =
				__sig->_sigev_un._tid;

			iocbs[current_iocb]->sigevent =
				&iocbs[current_iocb]->aio_sigevent;
			break;

#ifdef HAVE_PTHREAD_H
		case SIGEV_THREAD:
			iocbs[current_iocb]->aio_sigevent.sigev_value.sival_ptr =
				__sig->sigev_value.sival_ptr;
			iocbs[current_iocb]->aio_sigevent.sigev_notify =
				__sig->sigev_notify;
			iocbs[current_iocb]->aio_sigevent.sigev_notify_function =
				__sig->sigev_notify_function;
			iocbs[current_iocb]->aio_sigevent.sigev_notify_attributes =
				__sig->sigev_notify_attributes;


			ret = __aio_thread_init(&iocbs[current_iocb]->aio_sigevent);

			iocbs[current_iocb]->sigevent =
				&iocbs[current_iocb]->aio_sigevent;
		break;
#endif /* HAVE_PTHREAD_H */
#endif /* HAVE_AIO_EVENTS */

		default:
			errno = EINVAL;
			ret = -1;

			break;
		}

		if (ret != 0) {
			free (iocbs);
			return ret;
		}
		current_iocb++;
	}
#	endif /* HAVE_LIO_EVENTS */

	/* Now copy the user list into the new list */
	for (current_aiocb = 0; current_aiocb < __nent; current_aiocb++) {

		struct aiocb64 *aiocbp = __list[current_aiocb];

		if (aiocbp) {/* Ignore NULL entries */

			switch(aiocbp->aio_lio_opcode) {
			case LIO_NOP:
				/* Ignore NOP entries */
				break;

			case LIO_READ:
				/* sign extend aio_offset to 64 bits */
				aiocbp->aio_offset = ((struct aiocb *)aiocbp)->aio_offset;

				/* Add aiocb to list */
				iocbs[current_iocb] = aiocbp;

				ret = __aio_create_read(iocbs[current_iocb]);

				if (ret != 0) {
					aiocbp->__res = -errno;
					globret = -1;
				} else
					current_iocb++;
				break;

			case LIO_WRITE:
				/* sign extend aio_offset to 64 bits */
				aiocbp->aio_offset = ((struct aiocb *)aiocbp)->aio_offset;

				/* Add aiocb to list */
				iocbs[current_iocb] = aiocbp;

				ret = __aio_create_write(iocbs[current_iocb]);

				if (ret != 0) {
					aiocbp->__res = -errno;
					globret = -1;
				} else
					current_iocb++;
				break;

			default:
				globret = -1;
				break;
			}
		}
	}

	ret = io_submit(__aio_default_context, current_iocb, (struct iocb **)iocbs);

	/* Cleanup */
	for (i=0; i<(nent_aiocb - __nent); i++) {
		free (iocbs[i]);
	}

	free(iocbs);

	if (ret < 0) {
		errno = -ret;
		return -1;
	}

#	ifndef HAVE_LIO_WAIT
	if (__mode  == LIO_WAIT)
		ret = __aio_wait_all(__nent, __list, NULL);
#	endif /* HAVE_LIO_EVENTS */

	if (globret) {
		errno = EIO;
		return -1;
	}

	return 0;
}

int lio_listio(int __mode, struct aiocb *__const __list[__restrict_arr],
	       int __nent, struct sigevent *__restrict __sig)
{
	return lio_listio64(__mode, (struct aiocb64 *__const*)__list, 
			    __nent, __sig);
}
