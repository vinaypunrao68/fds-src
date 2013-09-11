SUBDIRS := lib/leveldb-1.12.0 util fdsp lib util/hash util/concurrency data_mgr stor_mgr_ice stor_hvisor_ice orch_mgr fds_client

.PHONY : all clean

all :
	for d in $(SUBDIRS); do $(MAKE) -C $$d all; done;

clean :
	for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done;
