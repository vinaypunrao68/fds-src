topdir            := ../..

compile:
	mvn -DskipTests clean assembly:directory
	mkdir -p $(main_dir_lib)/java/
	cp -rv target/classes $(main_dir_lib)/java/
	cp -rv target/om-1.0-bin/om-1.0/* $(main_dir_lib)/java/
	cp orchMgr $(main_dir_bin)
	chmod +x $(main_dir_bin)/orchMgr

clean: java_clean

java_clean:
	rm -rvf target/
	rm -rvf $(main_dir_lib)/java

include $(topdir)/Makefile.incl
