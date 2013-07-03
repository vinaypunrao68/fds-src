EXTRA_CFLAGS += -DLIB_KERNEL -I../include
obj-m += vvclib.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm *~
