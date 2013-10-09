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


#define BLOCK_SIZE	1024
#define NUM_BLOCKS	10
#define TESTFILE	"/etc/services"

static struct aiocb a[NUM_BLOCKS];

int main(int argc, char** argv)
{
	int fd;
	int ret;
	char *buf;
	int i;

	buf = (char*)malloc(NUM_BLOCKS * BLOCK_SIZE);

	if (buf == NULL) {
		perror("aio_cancel_fd: malloc");
		return 1;
	}

	fd = open(TESTFILE, O_RDONLY);

	if (fd == -1) {
		perror("aio_cancel_fd: Cannot open testfile");
		return 2;
	}

	for (i = 0; i < NUM_BLOCKS; i++) {
		memset(&a[i], 0, sizeof(struct aiocb));
		a[i].aio_fildes = fd;
		a[i].aio_offset = 0;
		memset(buf + i * BLOCK_SIZE, 0x00, BLOCK_SIZE);
		a[i].aio_buf = buf + i * BLOCK_SIZE;
		a[i].aio_nbytes = BLOCK_SIZE;
		a[i].aio_sigevent.sigev_notify = SIGEV_NONE;

		ret = aio_read(&a[i]);

		if (ret) {
			perror("");
			return 3;
		}
	}

	ret = aio_cancel(fd, NULL);

	close(fd);

	if (ret < 0) {
	  	printf ("aio_cancel_fd: cancel error %d (%s)\n",
			errno, strerror (errno));
		return 5;
	} else {
		if (ret == AIO_CANCELED) {
			return 0;
		}
		else if (ret == AIO_NOTCANCELED) {
			printf ("aio_cancel_fd: cancel returned AIO_NOTCANCELED\n");
			return 6;
		}
		else if (ret == AIO_ALLDONE) {
			printf ("aio_cancel_fd: cancel returned AIO_ALLDONE\n");
			return 7;
		}
		else {
			printf ("aio_cancel_fd: cancel returned unknown code %d\n",
				ret);
			return 8;
		}
	}

	return 0;
}
