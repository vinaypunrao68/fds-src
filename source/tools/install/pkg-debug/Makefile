# Makefile to create all our packages for install

all: clean package

.PHONY: package

clean:
	@sudo rm -f *.deb
	@sudo rm -rf tmppkg-*
	@sudo rm -f ../../package/fds-pkghelper/*.deb

package: 
	@echo "!!!WARNING!!! This is deprecated! Packaging FDS artifacts is now handled by omnibus. To package:\ncd fds-src && make package fds-platform BUILD_TYPE=(debug|release)"
	@/bin/false
	# If you don't want to build a "release" version of the packages, then run:
	#  make release-build=false
	#  to get Jenkins-versioned (x.y.z-buildnumber) or timestamped packages
	#  depending on the context
	#  Otherwise, defaults to old behavior of assuming you want
	#  "release" package versioning (x.y.z)
#ifneq "x$(release-build)" "xfalse"
#	../../package/fds-pkg-create.py
#else
#	sudo ../../package/fds-pkg-create.py -r
#endif

# -----------------------------------------------------------------------------
# Keep track of where all relevant ansible files live
# -----------------------------------------------------------------------------
#ansible_dir := ../../../../ansible
#ansible_binary := /usr/local/bin/ansible-playbook
#pip_binary := /usr/bin/pip
#run_ansible_playbook_local := ansible-playbook -i $(ansible_dir)/inventory/localhost -c local $(ansible_dir)/playbooks
#
# -----------------------------------------------------------------------------
# Run ansible to generate packageable config files
# -----------------------------------------------------------------------------
#$(pip_binary):
#	@sudo apt-get install -y --force-yes python-pip
#
#$(ansible_binary): $(pip_binary)
#	@sudo pip install ansible
#
#.NOTPARALLEL: generate-config-files
#generate-config-files: $(ansible_binary)
#	@$(run_ansible_playbook_local)/generate_config_files.yml
#
#showdir: $(ansible_binary)
#	@ls $(ansible_dir)
