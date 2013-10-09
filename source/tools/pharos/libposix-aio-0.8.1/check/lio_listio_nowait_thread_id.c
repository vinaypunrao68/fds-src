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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "aio.h"

#define TESTFILE	"/etc/services"
#define BUFFER_SIZE	1024
#define NUM_IOS		10


static volatile unsigned int received_all = 0;
static unsigned int num_received = 0;

void sigusr1_handler(int signum, siginfo_t *info, void *context)
{
	num_received++;
}

void *thread_start(void* ptr)
{
	sigset_t ss;

	sigfillset (&ss);

	while (1) {
	        siginfo_t si;
		int result;

		result = sigwaitinfo (&ss, &si);

		if (result > 0) {

		        if (si.si_code == SI_ASYNCIO) {
				received_all = 1;
			        pthread_exit (NULL);
			} else if (si.si_code == SI_TKILL) {
				printf ("lio_listio_nowait_thread_id_thread_id: Thread : received TKILL exiting\n");
				received_all = 1;
			        pthread_exit (NULL);
			}
		}
	}

}

int main(int argc, char** argv)
{
	int fd;
	struct aiocb *list[NUM_IOS];
	unsigned char *buf[NUM_IOS];
	struct sigevent event;
	int ret;
	int i;
	struct sigaction action;

	pthread_t tid;
	sigset_t ss;
	sigset_t oss;

	sigemptyset (&ss);
	sigaddset (&ss, SIGRTMIN+2);
	pthread_sigmask (SIG_SETMASK, &ss, &oss);

	ret = pthread_create(&tid, NULL, thread_start, NULL);

	if (ret) {
		perror("aio_write_one_thread_id: pthread_create");
		return 11;
	}

	/*
	 * We must open the file O_DIRECT to make sure we're really hitting the
	 * disk and not the page cache.
	 */
	fd = open (TESTFILE,  O_RDONLY | O_DIRECT);

	if (fd == -1) {
		perror("lio_listio_nowait_thread_id: Cannot open testfile");
		return 1;
	}

	for (i = 0; i < NUM_IOS; i++) {

		ret = posix_memalign ((void **)&buf[i], 512, BUFFER_SIZE);

		if (ret != 0) {
			perror ("lio_listio_nowait_thread_id: failed to alloc buffer");
			return 2;
		}

		list[i] = (struct aiocb *)malloc(sizeof(struct aiocb));

		if (list[i] == NULL) {
			perror ("lio_listio_nowait_thread_id: failed to alloc lio entry");
			return 3;
		}

		memset(list[i], 0, sizeof(struct aiocb));
		memset(buf[i], i, BUFFER_SIZE);

		if (i == NUM_IOS -2 ) {
			/* NULL entry */
			free (list[i]);
			list[i] = NULL;
		} else if (i == NUM_IOS - 1) {
			/* LIO_NOP entry */
			list[i]->aio_lio_opcode = LIO_NOP;
		} else {
			/* Regular entry */
			list[i]->aio_fildes = fd;
			list[i]->aio_buf = buf[i];
			list[i]->aio_nbytes = BUFFER_SIZE;
			list[i]->aio_offset = BUFFER_SIZE*i;
			list[i]->aio_lio_opcode = LIO_READ;

			/* Use SIGRTMIN+1 for individual completions */
			list[i]->aio_sigevent.sigev_notify = SIGEV_SIGNAL;
			list[i]->aio_sigevent.sigev_signo = SIGRTMIN+1;
			list[i]->aio_sigevent.sigev_value.sival_ptr = list[i];
		}
	}

	/* Use notification callback for list completion */
	memset (&event, 0, sizeof (struct sigevent));
	event.sigev_notify = SIGEV_THREAD_ID;
	event.sigev_signo = SIGRTMIN+2;
	event._sigev_un._tid = getpid ();

	/* Setup handler for individual operation completion */
	action.sa_sigaction = sigusr1_handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = SA_SIGINFO|SA_RESTART;
	sigaction(SIGRTMIN+1, &action, NULL);

	/* Run list */
	ret = lio_listio(LIO_NOWAIT, list, NUM_IOS, &event);

	if (ret) {
		perror("lio_listio_nowait_thread_id: lio_listio failed");
		return 4;
	}

	if (received_all) {
		printf ("lio_listio_nowait_thread_id: lio_listio waited\n");
		return 5;
	}

	/* Wait for list processing completion */
	while(!received_all);

	if (num_received != NUM_IOS - 2) {
		printf ("lio_listio_nowait_thread_id: wrong number of events received %d/%d\n",
			num_received, NUM_IOS - 2);
		return 6;
	}

	for (i = 0; i < NUM_IOS - 2; i++) {
	  	int err = aio_error(list[i]);
		int ret = aio_return(list[i]);

		if (err) {
			printf ("lio_listio_nowait_thread_id: req %d error (%s)\n",
				i, strerror (errno));
			return 7;
		}

		if (ret != list[i]->aio_nbytes) {
			printf ("lio_listio_nowait_thread_id: req %d wrong size returned %d/%d\n",
				i, ret, list[i]->aio_nbytes);
			return 8;
		}
	}

	close(fd);

	return 0;
}
