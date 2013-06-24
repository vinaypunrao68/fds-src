IDIR =-I../include -I/usr/include/mysql
CC=gcc
CFLAGS=$(IDIR) -g -DTVC_DBG
LDIR =-L/usr/lib/mysql
LIBS=$(LDIR) -lssl -lcrypto -lmysqlclient

DEPS = tvc_private.h ../include/tvclib.h ../include/fds_commons.h

SRCS = tvclib.c unit_test.c

OBJS = $(SRCS:%.c=%.o)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

unit_test: $(OBJS)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(OBJS) *~ core 
