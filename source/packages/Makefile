topdir            := ..

include $(topdir)/Makefile.incl

real_main_dir_build=$(realpath $(main_dir_build))

build_type=debug

ifeq ($(BUILD), release)
build_type=release
endif

$(info "build type: "$(build_type))

ifdef ARTIFACTORY_USER
gradle_arg = buildDeb deployDeb -Pmain_dir_build=${real_main_dir_build} \
 -Pbuild_type=${build_type} -Pusername=${ARTIFACTORY_USER} -Ppassword=${ARTIFACTORY_PASS}
else
gradle_arg = buildDeb -Pbuild_type=${build_type} -Pmain_dir_build=${real_main_dir_build}
endif

all:
	@echo ${real_main_dir_build}
	./gradlew ${gradle_arg} --stacktrace

clean:
	./gradlew clean -Pmain_dir_build=' ' -Pbuild_type=${build_type}

#Do it here
