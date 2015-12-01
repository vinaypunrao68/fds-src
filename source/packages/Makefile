topdir            := ..

include $(topdir)/Makefile.incl

all:
	real_main_dir_build=$(realpath $(main_dir_build))
	@echo ${real_main_dir_build}
	./gradlew buildDeb -Pmain_dir_build=${real_main_dir_build}

clean:
	./gradlew clean -Pmain_dir_build=' '

#Do it here
