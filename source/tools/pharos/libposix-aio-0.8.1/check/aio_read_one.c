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


int main(int argc, char** argv)
{
	int fd;
	int ret;
	char *buf;
	struct aiocb a;
	int len;

	buf = (char*)malloc(BLOCK_SIZE);

	if (buf == NULL) {
		perror("aio_read_one: malloc");
		return 1;
	}

	fd = open(TESTFILE, O_RDONLY);

	if (fd == -1) {
		perror("aio_read_one: Cannot open testfile");
		return 3;
	}

	memset(&a, 0, sizeof(struct aiocb));
	a.aio_fildes = fd;
	a.aio_offset = 0;
	memset(buf, 0x00, BLOCK_SIZE);
	a.aio_buf = buf;
	a.aio_nbytes = BLOCK_SIZE;
	a.aio_sigevent.sigev_notify = SIGEV_NONE;

	ret = aio_read(&a);

	if (ret) {
		perror("aio_read_one: aio_read error");
		return 4;
	}

	/* Wait until completion */
	while ((ret = aio_error(&a)) == EINPROGRESS);

	if (ret) {
		perror("aio_read_one: aio_error error");
		return 5;
	}

	len = aio_return(&a);

	if (len<0) {
		printf ("aio_read_one: aio_return error\n");
		return 6;
	}

	close(fd);
	
	return 0;
}
