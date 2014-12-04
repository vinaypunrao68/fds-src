# Makefile to create all our packages for install

all: clean generate-config-files package

.PHONY: package

clean:
	@sudo rm -f *.deb
	@sudo rm -rf tmppkg-*
	@sudo rm -f ../../package/fds-pkghelper/*.deb
	@sudo rm -f ../../../config/etc/*.conf ../../../test/formation.conf

package: 
	# If you don't want to build a "release" version of the packages, then run:
	#  make release-build=false
	#  to get Jenkins-versioned (x.y.z-buildnumber) or timestamped packages
	#  depending on the context
	#  Otherwise, defaults to old behavior of assuming you want
	#  "release" package versioning (x.y.z)
	@if [ "x${release-build}" = "xfalse" ]; then \
		fdspkgcreate ; \
		cd ../../package/fds-pkghelper && fdspkgcreate ; \
	else \
		sudo fdspkgcreate -r ; \
		sudo cd ../../package/fds-pkghelper && fdspkgcreate -r ; \
	fi

# -----------------------------------------------------------------------------
# Keep track of where all relevant ansible files live
# -----------------------------------------------------------------------------
ansible_dir := ../../../../ansible
ansible_binary := /usr/local/bin/ansible-playbook
pip_binary := /usr/bin/pip
run_ansible_playbook_local := ansible-playbook -i $(ansible_dir)/ansible_hosts -c local $(ansible_dir)/playbooks

# -----------------------------------------------------------------------------
# Run ansible to generate packageable config files
# -----------------------------------------------------------------------------
$(pip_binary):
	@sudo apt-get install -y --force-yes python-pip

$(ansible_binary): $(pip_binary)
	@sudo pip install ansible

.NOTPARALLEL: generate-config-files
generate-config-files: $(ansible_binary)
	@$(run_ansible_playbook_local)/generate_config_files.yml

showdir: $(ansible_binary)
	@ls $(ansible_dir)
