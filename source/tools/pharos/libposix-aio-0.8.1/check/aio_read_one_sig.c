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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "aio.h"

#define BLOCK_SIZE	32768
#define TESTFILE	"/etc/services"

volatile unsigned int	done = 0;

void sigrt_handler(int signo, siginfo_t *info, void *context)
{
	struct aiocb *a = info->si_ptr;
	int ret;

	ret = aio_error(a);

	if (ret)
		printf("aio_read_one_sig: aio_error error (%s)\n", strerror(ret));

	done = 1;
}

int main(int argc, char** argv)
{
	struct sigaction action;
	int fd;
	int ret;
	char *buf;
	struct aiocb a;
	int len;

	action.sa_sigaction = sigrt_handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = SA_SIGINFO|SA_RESTART;
	sigaction(SIGRTMIN, &action, NULL);

	buf = (char*)malloc(BLOCK_SIZE);

	if (buf == NULL) {
		perror("aio_read_one_sig: malloc");
		return 1;
	}

	fd = open(TESTFILE, O_RDONLY);

	if (fd == -1) {
		perror("aio_read_one_sig: Cannot open testfile");
		return 3;
	}

	memset(&a, 0, sizeof(struct aiocb));
	a.aio_fildes = fd;
	a.aio_offset = 0;
	memset(buf, 0x00, BLOCK_SIZE);
	a.aio_buf = buf;
	a.aio_nbytes = BLOCK_SIZE;
	a.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	a.aio_sigevent.sigev_signo = SIGRTMIN;
	a.aio_sigevent.sigev_value.sival_ptr = &a;

	ret = aio_read(&a);

	if (ret) {
		perror("aio_read_one_sig: aio_read error");
		return 4;
	}

	while (!done);

	len = aio_return(&a);

	if (len<0) {
		printf ("aio_read_one_sig: aio_return error\n");
		return 6;
	}

	close(fd);
	
	return 0;
}
