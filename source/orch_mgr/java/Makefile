topdir            := ../..

compile:
	mvn compile
	cp -rv target $(main_dir_bin)

clean: java_clean

java_clean:
	rm -rvf target/
	rm -rvf $(main_dir_bin)/target

include $(topdir)/Makefile.incl
