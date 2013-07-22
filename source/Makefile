SUBDIRS := lib data_mgr stor_mgr

.PHONY : all clean

all :
	for d in $(SUBDIRS); do $(MAKE) -C $$d all; done;

clean :
	for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done;
