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
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include "posix_aio.h"

#ifdef HAVE_PTHREAD_H

#include <pthread.h>

struct thread_info {
	int done;
	void *(*func)(void*);
	void *value;
};

#ifdef HAVE_AIO_EVENTS


static void *thread_start(void* ptr)
{
	sigset_t ss;
	struct thread_info *info = (struct thread_info *)ptr;
	void *(*function)(void*);
	void *value;

	sigfillset (&ss);

	/* Waiting for signals. */
	while (1) {
	        siginfo_t si;
		int result;

		result = sigwaitinfo (&ss, &si);

		if (result > 0) {
		        if (si.si_code == SI_ASYNCIO) {
			  	function = info->func;
				value = info->value;

				function (value);

				/* Free callback info */
				free (info);

			        pthread_exit (NULL);
			} else if (si.si_code == SI_TKILL) {
				free (info);
			        pthread_exit (NULL);
			}
		}
	}

}
#endif /* HAVE_AIO_EVENTS */

int __aio_thread_init(struct sigevent *event)
{
#	ifdef HAVE_AIO_EVENTS
	pthread_attr_t lattr;
	pthread_attr_t *pattr;
	struct thread_info *thread_info;
	pthread_t thread_id;
	int ret;


	/* Alloc and fill helper thread callback info */
	thread_info = (struct thread_info *)malloc(sizeof(struct thread_info));

	if (thread_info == NULL) {
		errno = -EAGAIN;
		return -1;
	}

	thread_info->func = (void *(*)(void*))event->sigev_notify_function;
	thread_info->value = event->sigev_value.sival_ptr;

	/* Use local thread_attr if user did not provide one */
	if (event->sigev_notify_attributes == NULL) {
	        pattr = &lattr;
	} else {
	        pattr = event->sigev_notify_attributes;
	}


	/* Make sure the thread is created detached */
	pthread_attr_init (pattr);
	pthread_attr_setdetachstate (pattr, PTHREAD_CREATE_DETACHED);

	/* Block RT signal (it is waited upon in the helper thread */
	sigset_t ss;
	sigset_t oss;

	sigemptyset (&ss);
	sigaddset (&ss, SIGRTMIN);
	pthread_sigmask (SIG_SETMASK, &ss, &oss);

	ret = pthread_create( &thread_id, pattr, thread_start, thread_info);

	/* Release the thread attributes */
	pthread_attr_destroy (pattr);

	if (ret) {
	        pthread_sigmask (SIG_SETMASK, &oss, NULL);
		free(thread_info);
		return ret;
	}

	event->_sigev_un._tid = getpid ();
	event->sigev_signo = SIGRTMIN;
	event->sigev_value.sival_int = 0;

	return 0;
#	else
	errno = EINVAL;
	return -1;
#	endif /* HAVE_AIO_EVENTS */
}
#endif /* HAVE_PTHREAD_H */
