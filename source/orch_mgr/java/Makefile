topdir            := ../..

compile: thrift
	mvn -DskipTests clean assembly:directory
	mkdir -p $(main_dir_lib)/java/
	cp -rv target/classes $(main_dir_lib)/java/
	cp -rv admin-webapp $(main_dir_lib)
	cp -rv target/om-1.0-bin/om-1.0/* $(main_dir_lib)/java/
	cp orchMgr $(main_dir_bin)
	chmod +x $(main_dir_bin)/orchMgr

thrift:
	../../../Build/linux-x86_64.debug/bin/thrift -r --gen java ../../fdsp/FDSP.thrift

clean: java_clean webapp_clean

webapp_clean:
	rm -rvf $(main_dir_lib)/admin-webapp/*

java_clean:
	rm -rvf target/
	rm -rvf gen-java/
	rm -rvf $(main_dir_lib)/java

include $(topdir)/Makefile.incl
