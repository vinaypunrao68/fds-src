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
#include <signal.h>
#include <errno.h>

#include "aio.h"

#define TESTFILE	"/etc/services"
#define BUFFER_SIZE	1024
#define NUM_IOS		10

static volatile unsigned int received_all = 0;
static unsigned int received_selected = 0;

void sigusr1_handler(int signum, siginfo_t *info, void *context)
{
	if (info->si_value.sival_int == 6)
		received_selected = 1;
}

void sigusr2_handler(int signum, siginfo_t *info, void *context)
{
	received_all = 1;
}

int main(int argc, char** argv)
{
	int fd;
	struct aiocb *aiocbs[NUM_IOS];
	unsigned char *buf[NUM_IOS];
	struct sigevent event;
	int ret;
	int i;
	struct sigaction action;

	/*
	 * We must open the file O_DIRECT to make sure we're really hitting the
	 * disk and not the page cache.
	 */
	fd = open (TESTFILE,  O_RDONLY | O_DIRECT);

	if (fd == -1) {
		perror("aio_suspend: Cannot create testfile");
		return 1;
	}

	for (i = 0; i < NUM_IOS; i++) {

		ret = posix_memalign ((void **)&buf[i], 512, BUFFER_SIZE);

		if (ret != 0) {
			perror ("aio_suspend: failed to alloc buffer");
			return 2;
		}

		aiocbs[i] = (struct aiocb*)malloc(sizeof(struct aiocb));

		if (aiocbs[i] == NULL) {
			perror ("aio_suspend: failed to alloc aio entry");
			return 3;
		}

		memset(aiocbs[i], 0, sizeof(struct aiocb));
		memset(buf[i], i, BUFFER_SIZE);

		aiocbs[i]->aio_fildes = fd;
		aiocbs[i]->aio_buf = buf[i];
		aiocbs[i]->aio_nbytes = BUFFER_SIZE;
		aiocbs[i]->aio_offset = i * BUFFER_SIZE;
		aiocbs[i]->aio_lio_opcode = LIO_READ;

		/* Use SIGRTMIN+1 for individual completions */
		aiocbs[i]->aio_sigevent.sigev_notify = SIGEV_SIGNAL;
		aiocbs[i]->aio_sigevent.sigev_signo = SIGRTMIN+1;
		aiocbs[i]->aio_sigevent.sigev_value.sival_int = i;
	}

	/* Use SIGRTMIN+2 for list completion */
	memset (&event, 0, sizeof (struct sigevent));
	event.sigev_notify = SIGEV_SIGNAL;
	event.sigev_signo = SIGRTMIN+2;
	event.sigev_value.sival_ptr = NULL;

	/* Setup handler for individual operation completion */
	action.sa_sigaction = sigusr1_handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = SA_SIGINFO|SA_RESTART;
	sigaction(SIGRTMIN+1, &action, NULL);

	/* Setup handler for list completion */
	action.sa_sigaction = sigusr2_handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = SA_SIGINFO|SA_RESTART;
	sigaction(SIGRTMIN+2, &action, NULL);

	/* Setup suspend list */
	struct aiocb *list[2];
	list[0] = NULL;
	list[1] = aiocbs[6];


	ret = lio_listio (LIO_NOWAIT, aiocbs, NUM_IOS, &event);

	if (ret) {
		printf("aio_suspend: lio_listio failed (%s)\n", strerror(errno));
		return 4;
	}

	if (received_selected) {
		printf ("aio_suspend: AIOCB 6 already completed before suspend\n");
		return 5;
	}

	ret = aio_suspend((const struct aiocb **)list, 2, NULL);

	if (!received_selected) {
		printf ("aio_suspend: AIOCB 6 should have completed after suspend\n");
		return 6;
	}

	if (ret) {
		printf ("aio_suspend: uspend error (%s)\n", strerror (errno));
		return 7;
	}

	/* Wait for list processing completion */
	while(!received_all);

	/* Check return code and free things */
	for (i = 0; i < NUM_IOS; i++) {
	  	int err = aio_error(aiocbs[i]);
		int ret = aio_return(aiocbs[i]);

		if (err) {
			printf ("aio_suspend: req %d error (%s)\n",
				i, strerror (errno));
			return 8;
		}

		if (ret != aiocbs[i]->aio_nbytes) {
			printf ("aio_suspend: req %d wrong size returned %d/%d\n",
				i, ret, aiocbs[i]->aio_nbytes);
			return 9;
		}

		free ((void *)aiocbs[i]->aio_buf);
		free (aiocbs[i]);
	}

	close(fd);

	return 0;
}
