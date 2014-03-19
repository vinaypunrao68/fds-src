topdir            := ../..

compile: thrift webapp
	mvn -DskipTests clean assembly:directory
	mkdir -p $(main_dir_lib)/java/
	cp -rv target/classes $(main_dir_lib)/java/
	cp -rv target/om-1.0-bin/om-1.0/* $(main_dir_lib)/java/
	cp orchMgr $(main_dir_bin)
	chmod +x $(main_dir_bin)/orchMgr

webapp:
	$(MAKE) -C admin-webapp

thrift:
	../../../Build/linux-x86_64.debug/bin/thrift -r --gen java ../../fdsp/FDSP.thrift

clean: java_clean webapp_clean

webapp_clean:
	$(MAKE) -C admin-webapp webapp_clean

java_clean:
	rm -rvf target/
	rm -rvf gen-java/
	rm -rvf $(main_dir_lib)/java

include $(topdir)/Makefile.incl
