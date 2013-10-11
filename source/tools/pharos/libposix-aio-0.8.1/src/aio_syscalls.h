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

#ifndef __AIO_SYSCALLS_H
#define __AIO_SYSCALLS_H

#include "posix_aio.h"

extern int io_setup	(int maxevents, io_context_t *ctxp);
extern int io_destroy	(io_context_t ctx);
extern int io_submit	(io_context_t ctx, long nr, struct iocb *ios[]);
extern int io_cancel	(io_context_t ctx, struct iocb *iocb, struct io_event *evt);
extern int io_getevents	(io_context_t ctx_id, long min_nr, long nr, struct io_event *events,
			 struct timespec *timeout);


#endif /* __AIO_SYSCALLS_H */
