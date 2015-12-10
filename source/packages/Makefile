topdir            := ..

include $(topdir)/Makefile.incl

real_main_dir_build=$(realpath $(main_dir_build))

ifdef ARTIFACTORY_USER
gradle_arg = buildDeb deployDeb -Pmain_dir_build=${real_main_dir_build} \
-Pusername=${ARTIFACTORY_USER} -Ppassword=${ARTIFACTORY_PASS}
else
gradle_arg = buildDeb -Pmain_dir_build=${real_main_dir_build}
endif

all:
	@echo ${real_main_dir_build}
	./gradlew ${gradle_arg} --stacktrace

clean:
	./gradlew clean -Pmain_dir_build=' '

#Do it here
