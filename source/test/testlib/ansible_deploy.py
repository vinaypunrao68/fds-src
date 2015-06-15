from ansible.playbook import PlayBook
from ansible.inventory import Inventory
from ansible.inventory import Group
from ansible.inventory import Host
from ansible import constants as C
from ansible import callbacks
from ansible import utils

import os
import logging
import config

logger = logging.getLogger('scope.name')

# Setup some constants
C.ANSIBLE_SSH_ARGS = '-o UserKnownHostsFile=/dev/null -o ControlMaster=auto -o ControlPersist=60s -F ./ssh.config'
C.DEFAULT_LOG_PATH = os.path.join(os.path.expanduser("~"), '/logs/ansible.log')

# When it's first loaded, the `logger` object is not set
# so we reload it after setting the DEFAULT_LOG_PATH constant
# see: https://github.com/ansible/ansible/blob/v1.9.0-2/lib/ansible/callbacks.py#L34-L45
reload(callbacks)

def run_playbook(ipaddress, om_ip_address, playbook='add_fds_node.yml'):
    # Boilerplace callbacks for stdout/stderr and log output
    utils.VERBOSITY = 0
    playbook_cb = callbacks.PlaybookCallbacks(verbose=utils.VERBOSITY)
    stats = callbacks.AggregateStats()
    runner_cb = callbacks.PlaybookRunnerCallbacks(stats, verbose=utils.VERBOSITY)

    host = Host(name=ipaddress)
    group = Group(name="new-nodes")
    group.add_host(host)
    inventory = Inventory(host_list=[], vault_password="Formation1234")
    inventory.add_group(group)
    key_file = os.path.join(os.path.expanduser("~"), '.ssh/fds-deploy_rsa')
    playbook_path = os.path.join(config.ANSIBLE_PLAYBOOKS, playbook)
    pb = PlayBook(
            playbook=playbook_path,
            inventory = inventory,
            remote_user='deploy',
            callbacks=playbook_cb,
            runner_callbacks=runner_cb,
            stats=stats,
            private_key_file=key_file
    )

    results = pb.run()
    print results
