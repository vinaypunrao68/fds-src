# Makefile to create all our packages for install

all: clean generate-config-files package

.PHONY: package

clean:
	@sudo rm -f *.deb
	@sudo rm -rf tmppkg-*
	@sudo rm -f ../../../config/etc/*.conf ../../../test/formation.conf

package: 
	@sudo fdspkgcreate -r

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
