SUBDIRS := lib/leveldb-1.12.0 lib util fdsp util/hash util/concurrency data_mgr stor_mgr_ice stor_hvisor_ice

.PHONY : all clean

all :
	for d in $(SUBDIRS); do $(MAKE) -C $$d all; done;

clean :
	for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done;
