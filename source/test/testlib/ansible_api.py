from ansible.playbook import PlayBook
from ansible.inventory import Inventory
from ansible.inventory import Group
from ansible.inventory import Host
from ansible import callbacks
from ansible import utils
from ansible import constants as C

import jinja2
from tempfile import NamedTemporaryFile
import os
import config

# Boilerplace callbacks for stdout / stderr and log output
utils.VERBOSITY = 0
playbook_cb = callbacks.PlaybookCallbacks(verbose=utils.VERBOSITY)
stats = callbacks.AggregateStats()
runner_cb = callbacks.PlaybookRunnerCallbacks(stats, verbose=utils.VERBOSITY)

# Setup some constants
C.ANSIBLE_SSH_ARGS = '-o UserKnownHostsFile=/dev/null -o                        ControlMaster=auto -o ControlPersist=60s -F ./ssh.config'
C.DEFAULT_LOG_PATH = os.path.join(os.path.expanduser("~"), '/logs/ansible.log')
path = os.path.join(os.path.expanduser("~"), '/logs')

if not os.path.exists(path):
    os.makedirs(path)
    f = open(C.DEFAULT_LOG_PATH, 'w+')

reload(callbacks)

# Dynamic Inventory
# We fake a inventory file and let Ansible load if it's a real file
# Just don't tell ansible that, so we don't hurt its feelings.

inventory = """
localhost
awo-bh-01 fds_deploy_local_deps=no

[testapi:vars]
clean_fds=no
ansible_ssh_user=deploy
cluster_location=awo
deploy_artifact=nightly
fds_build=release
fds_default_nic=eth0
fds_deploy_local_deps=no
fds_om_host={{ om_ip_address }}
fds_metrics_enabled=false
fds_metricsdb_ip=192.168.2.1
fds_ssh_user=root
fds_ssh_password=passwd
fds_ft_common_enable_multi_om_support=true
fds_om_host_count=3
fds_om_influxdb_url="http://{{ om_ip_address }}:8086"
"""

def create_dynamic_template(om_ip_address, playbook="deploy_fds_ec2.yml"):
    inventory_template = jinja2.Template(inventory)
    rendered_inventory = inventory_template.render({
	'om_ip_address' : om_ip_address,
        # add the any other variable here
    })

    # Create a temporary file and write the template string to it
    hosts = NamedTemporaryFile(delete=False)
    hosts.write(rendered_inventory)
    hosts.close()
    playbook_path = os.path.join(config.ANSIBLE_PLAYBOOKS, playbook)
    key_file = os.path.join(os.path.expanduser("~"), '.ssh/fds-deploy_rsa')
    inventory_obj = Inventory(host_list=[hosts.name], vault_password="Formation1234")
    pb = PlayBook(
            playbook=playbook_path,
            remote_user='deploy',
            inventory=inventory_obj,
            callbacks=playbook_cb,
            runner_callbacks=runner_cb,
            stats=stats,
            private_key_file=key_file,
    )

    results = pb.run()
    # Ensure the stats callback is called
    playbook_cb.on_stats(pb.stats)
    os.remove(hosts.name)

    print results


if __name__ == "__main__":
    create_dynamic_template("10.20.30.40", "deploy_fds_ec2.yml")
