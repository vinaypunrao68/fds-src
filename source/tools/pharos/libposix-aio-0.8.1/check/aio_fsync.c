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
#include <errno.h>
#include <string.h>

#include "aio.h"

#define FILENAME "/tmp/aio_fsync.test"
#define BUFSIZE	1024

int main(int argc, char** argv)
{
	int fd;
	struct aiocb a;
	struct aiocb b;
	int err;
	int ret;
	char *buf;


	fd = open (FILENAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

	if (fd == -1) {
		perror("aio_fsync: Cannot create testfile");
		return 1;
	}

	buf = (char*)malloc(BUFSIZE);

	if (buf == NULL) {
		perror ("aio_fsync: Failed to alloc buffer");
		return 2;
	}

	memset(&a, 0, sizeof(a));
	memset(buf, 0xac, BUFSIZE);

	a.aio_fildes = fd;
	a.aio_offset = 0;
	a.aio_buf = buf;
	a.aio_nbytes = BUFSIZE;
	a.aio_sigevent.sigev_notify = SIGEV_NONE;

	ret = aio_write(&a);

	if (ret) {
		perror("aio_fsync: aio_write");
		return 3;
	}

	memset(&b, 0, sizeof(b));
	b.aio_fildes = fd;
	b.aio_offset = 0;
	b.aio_buf = 0;
	b.aio_nbytes = 0;
	b.aio_sigevent.sigev_notify = SIGEV_NONE;

	ret = aio_fsync(O_SYNC, &b);

	if (ret) {
		printf("aio_fsync: fsync error %d (%s)\n", errno, strerror (errno));
		return 4;
	}


	while (aio_error(&a) == EINPROGRESS);

	err = aio_error(&a);
	ret = aio_return(&a);

	if (err) {
		printf ("aio_fsync: error %d (%s)\n", errno, strerror (errno));
		return 5;
	}

	if (ret != BUFSIZE) {
		printf ("aio_fsync: wrong write retval %d/%d\n", ret, BUFSIZE);
		return 6;
	}

	close(fd);

	memset(buf, 0x00, BUFSIZE);

	fd = open(FILENAME, O_RDONLY);

	a.aio_fildes = fd;
	a.aio_offset = 0;
	a.aio_buf = buf;
	a.aio_nbytes = BUFSIZE;
	a.aio_sigevent.sigev_notify = SIGEV_NONE;

	ret = aio_read(&a);

	if (ret) {
		perror("aio_fsync: aio_read");
		return 7;
	}

	while(aio_error(&a) == EINPROGRESS);

	err = aio_error(&a);
	ret = aio_return(&a);

	if (err) {
		printf ("aio_fsync: error %d (%s)\n", errno, strerror (errno));
		return 5;
	}

	if (ret != BUFSIZE) {
		printf ("aio_fsync: wrong read retval %d/%d\n", ret, BUFSIZE);
		return 6;
	}

	close(fd);

	return 0;
}
