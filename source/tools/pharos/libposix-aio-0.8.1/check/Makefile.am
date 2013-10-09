

INCLUDES = -I$(top_srcdir)/include
AM_CFLAGS = -Wall -Werror -g
LDADD = ../src/libposix-aio.la



test_programs =				\
	aio_read_one			\
	aio_read_one_sig		\
	aio_read_one_thread		\
	aio_read_one_thread_id		\
	aio_write_one			\
	aio_write_one_sig		\
	aio_write_one_thread		\
	aio_write_one_thread_id		\
	lio_listio_nowait		\
	lio_listio_nowait_sig		\
	lio_listio_nowait_thread	\
	lio_listio_nowait_thread_id	\
	lio_listio_wait			\
	aio_suspend			\
	aio_suspend_timeout		\
	aio_cancel			\
	aio_cancel_fd			\
	aio_fsync

check_PROGRAMS = $(test_programs)
noinst_PROGRAMS = $(test_programs)

TESTS = $(test_programs)



clean-local:
	rm -f *~
