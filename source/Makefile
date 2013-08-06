SUBDIRS := lib data_mgr util util/hash util/concurrency lib/leveldb-1.12.0 stor_mgr

.PHONY : all clean

all :
	for d in $(SUBDIRS); do $(MAKE) -C $$d all; done;

clean :
	for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done;
