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

#ifndef __POSIX_AIO_H
#define __POSIX_AIO_H

#include <config.h>
#include <errno.h>
#include <string.h>
#ifdef HAVE_AIO_EVENTS
#include <signal.h>
#endif /* HAVE_AIO_EVENTS */

#ifndef __USE_LARGEFILE64
/* We need the 64-bit struct and functions internally */
#  define __USE_LARGEFILE64
#  include "../include/aio.h"
#  undef __USE_LARGEFILE64
#else
#  include "../include/aio.h"
#endif

struct iocb;

typedef struct io_context *io_context_t;

typedef enum io_iocb_cmd {
	IO_CMD_PREAD = 0,
	IO_CMD_PWRITE = 1,

	IO_CMD_FSYNC = 2,
	IO_CMD_FDSYNC = 3,

	IO_CMD_POLL = 5,
	IO_CMD_NOOP = 6,

	IO_CMD_GROUP = 7,
} io_iocb_cmd_t;

#if __WORDSIZE == 64
# define PADDED(__type, __name)	__type __name;
# elif __WORDSIZE == 32
#  if defined(__LITTLE_ENDIAN)
#   define PADDED(__type, __name) __type __name; int __pad_##__name
#  elif defined(__BIG_ENDIAN)
#   define PADDED(__type, __name) int __pad_##__name; \ __type __name
#  else
#   error edit for your odd byteorder.
# endif /* _ENDIAN */
#else
# error edit for your odd word size.
#endif /* __WORDSIZE */


struct io_event {
	PADDED(unsigned long, data);
	PADDED(unsigned long, obj);
	long long res;
	long long res2;
};

#undef PADDED

typedef void (*io_callback_t)(io_context_t ctx, struct iocb *iocb, long res, long res2);

#ifdef HAVE_PTHREAD_H

#include <pthread.h>

int __aio_thread_init(struct sigevent *event);

#endif /* HAVE_PTHREAD_H */

#ifndef HAVE_MAXEVENTS_DEFAULT

/* user can modify it before first call */

extern int __aio_maxevents_default;

#endif /* HAVE_MAXEVENTS_DEFAULT */

extern io_context_t __aio_default_context;


extern int	__aio_get_res(const struct aiocb64 *__aiocbp, long long *res);
extern void	__aio_set_res(struct aiocb64 *__aiocbp, long long res);
extern int	__aio_wait_one(int __nent, struct aiocb64 *const __list[],
			       const struct timespec *__timeout);

#ifndef HAVE_LIO_WAIT
extern int 	__aio_wait_all(int __nent, struct aiocb64 *const __list[],
			       const struct timespec *__timeout);
#endif

extern int	__aio_create_write(struct aiocb64 *__aiocbp);
extern int	__aio_create_read(struct aiocb64 *__aiocbp);


#define	ASSERT_CTX(a)	if (__aio_default_context == NULL) {	\
				errno = a;			\
				return -1;			\
			}
#ifdef HAVE_MAXEVENTS_DEFAULT
#define INIT_CTX	ASSERT_CTX(ENOSYS)
#else
#define INIT_CTX	if (__aio_default_context == NULL) {		\
				ret = io_setup(__aio_maxevents_default,	\
					       &__aio_default_context);	\
				if (ret == -1)				\
					return ret;			\
			}
#endif /* HAVE_MAXEVENTS_DEFAULT */

#endif /* __POSIX_AIO_H */
