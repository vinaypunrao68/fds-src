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

#ifndef __AIO_H
#define __AIO_H

#include <features.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>

#include <endian.h>

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

__BEGIN_DECLS

/* Asynchronous I/O control block.  */
/*
 * The AIO control block is made of the standard iocb augmented with the
 * mandatory POSIX fields.
 */
struct aiocb
{
	/* Standard iocb */
	PADDED(void *,		data);		/* data to be returned in event's data */
	unsigned int		key;		/* the kernel sets aio_key to the req # */
	unsigned int		aio_reserved1;	/* To match kernel iocb aio_reserved1 */

	short			__opcode; 	/* iocb opcode value */
	short			__reqprio;	/* iocb request priority */
	unsigned int		aio_fildes;	/* iocb file descriptor */
	PADDED(volatile void *,	__buf);		/* iocb I/O buffer */
	PADDED(unsigned long,	__nbytes);	/* iocb I/O length */
	long long		offset;		/* offset of I/O request */

	PADDED(struct sigevent *, sigevent);	/* Used for completion notification */
	PADDED(unsigned long,	reserved3);	/* To match kernel iocb aio_reserved3 */

	/* Additional POSIX fields */
	PADDED(volatile void *,	aio_buf);	/* AIO user buffer */
	PADDED(unsigned int,	aio_nbytes);	/* AIO user length */
	int			aio_lio_opcode;
	int			aio_reqprio;
	struct sigevent		aio_sigevent;

#ifdef __USE_FILE_OFFSET64
	__off64_t		aio_offset;
#else
	PADDED(__off_t, 	aio_offset);
#endif

	/* Internal */
	long long		__res;		/* aiocb return code */
	int			__gone_direct;	/* flag indicating we changed to O_DIRECT */
	int			__valid;	/* flag indicating the aiocb is valid */
};

#ifdef __USE_LARGEFILE64
struct aiocb64
{
	/* Standard iocb */
	PADDED(void *,		data);		/* data to be returned in event's data */
	unsigned int		key;		/* the kernel sets aio_key to the req # */
	unsigned int		reserved1;	/* To match kernel iocb aio_reserved1 */

	short			__opcode;	/* iocb opcode value */
	short			__reqprio;	/* iocb request priority */
	unsigned int		aio_fildes;	/* iocb file descriptor */
	PADDED(void *,		__buf);		/* iocb I/O buffer */
	PADDED(unsigned long,	__nbytes);	/* iocb I/O length */
	long long		offset;		/* offset of I/O request */

	PADDED(struct sigevent *, sigevent);	/* Used for completion notification */
	PADDED(unsigned long,	reserved3);	/* To match kernel iocb aio_reserved3 */

	/* Additional POSIX fields */
	PADDED(void *,		aio_buf);	/* AIO user buffer */
	PADDED(unsigned int,	aio_nbytes);	/* AIO user length */
	int			aio_lio_opcode;
	int			aio_reqprio;
	struct sigevent		aio_sigevent;
	__off64_t		aio_offset;

	/* Internal */
	long long		__res;		/* aiocb return code */
	int			__gone_direct;	/* flag indicating we changed to O_DIRECT */
	int			__valid;	/* flag indicating the aiocb is valid */
};

#endif /* __USE_LARGEFILE64 */

#undef PADDED

enum {
	AIO_CANCELED = 0,
#define AIO_NOTCANCELED AIO_NOTCANCELED
	AIO_NOTCANCELED = 1,
#define AIO_ALLDONE AIO_ALLDONE
	AIO_ALLDONE = 2,
#define AIO_CANCELED AIO_CANCELED
};

enum {
	LIO_NOP = 0,
#define LIO_NOP LIO_NOP
	LIO_READ = 1,
#define LIO_READ LIO_READ
	LIO_WRITE = 2,
#define LIO_WRITE LIO_WRITE
};

enum {
	LIO_NOWAIT = 0,
#define LIO_NOWAIT LIO_NOWAIT
	LIO_WAIT = 1,
#define LIO_WAIT LIO_WAIT
};

#ifndef __USE_FILE_OFFSET64
extern int aio_cancel(int __fildes, struct aiocb *__aiocbp) __THROW;
extern int aio_error(__const struct aiocb *__aiocbp) __THROW;
extern int aio_fsync(int __op, struct aiocb *__aiocbp) __THROW;
extern int aio_read(struct aiocb *__aiocbp) __THROW;
extern ssize_t aio_return(struct aiocb *__aiocbp) __THROW;
extern int aio_suspend(__const struct aiocb *__const __list[], 
		       int __nent, __const struct timespec *__timeout);
extern int aio_write(struct aiocb *__aiocbp) __THROW;
extern int lio_listio(int __mode, 
		      struct aiocb *__const __list[__restrict_arr],
		      int __nent, struct sigevent *__restrict __sig) __THROW;
#else /* __USE_FILE_OFFSET64 */
# ifdef __REDIRECT_NTH
extern int __REDIRECT_NTH (aio_cancel, (int __fildes, struct aiocb *__aiocbp),
			   aio_cancel64);
extern int __REDIRECT_NTH (aio_error, (__const struct aiocb *__aiocbp),
			   aio_error64);
extern int __REDIRECT_NTH (aio_fsync, (int __operation, struct aiocb *__aiocbp),
			   aio_fsync64);
extern int __REDIRECT_NTH (aio_read, (struct aiocb *__aiocbp),
			   aio_read64);
extern ssize_t __REDIRECT_NTH (aio_return, (struct aiocb *__aiocbp),
			       aio_return64);
extern int __REDIRECT_NTH (aio_suspend, (__const struct aiocb *__const __list[],
					 int __nent,
					 __const struct timespec *__restrict __timeout),
			   aio_suspend64);
extern int __REDIRECT_NTH (aio_write, (struct aiocb *__aiocbp),
			   aio_write64);
extern int __REDIRECT_NTH (lio_listio, (int __mode,
					struct aiocb *__const __list[__restrict_arr],
					int __nent,
					struct sigevent *__restrict __sig),
			   lio_listio64);

# else /* ! __REDIRECT */
#  define aio_cancel aio_cancel64
#  define aio_error aio_error64
#  define aio_fsync aio_fsync64
#  define aio_read aio_read64
#  define aio_return aio_return64
#  define aio_suspend aio_suspend64
#  define aio_write aio_write64
#  define lio_listio lio_listio64
# endif /* __REDIRECT */
#endif /* __USE_FILE_OFFSET64 */

#ifdef __USE_LARGEFILE64
extern int aio_cancel64 (int __fildes, struct aiocb64 *__aiocbp) __THROW;
extern int aio_error64 (__const struct aiocb64 *__aiocbp) __THROW;
extern int aio_fsync64 (int __operation, struct aiocb64 *__aiocbp) __THROW;
extern int aio_read64 (struct aiocb64 *__aiocbp) __THROW;
extern ssize_t aio_return64 (struct aiocb64 *__aiocbp) __THROW;
extern int aio_write64 (struct aiocb64 *__aiocbp) __THROW;
extern int aio_suspend64 (__const struct aiocb64 *__const __list[], int __nent,
			  __const struct timespec *__restrict __timeout) __THROW;
extern int lio_listio64 (int __mode,
			 struct aiocb64 *__const __list[__restrict_arr],
			 int __nent, struct sigevent *__restrict __sig) __THROW;
#endif /* __USE_LARGEFILE64 */

__END_DECLS


#endif /* __POSIX__AIO_H */
