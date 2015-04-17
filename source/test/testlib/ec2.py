##############################################################################
# author: Philippe Ribeiro                                                   #
# email: philippe@formationds                                                #
# file: ec2.py                                                               #
##############################################################################

# Import Python libraries
import ansible.runner
import ansible.playbook
import ansible.inventory

import boto.ec2
import logging
import sys
import time

from ansible import callbacks
from ansible import utils

# Import local defined libraries
import aws_config
import error_codes
import config

class EC2(object):
    
    # static elements of the EC2 class
    logging.basicConfig(level=logging.INFO)
    log = logging.getLogger(__name__)
    
    stats = callbacks.AggregateStats()
    playbook_cb = callbacks.PlaybookCallbacks(verbose=utils.VERBOSITY)
    runner_cb = callbacks.PlaybookRunnerCallbacks(stats, verbose=utils.VERBOSITY)

    def __init__(self, name, is_om_node=False):
        # create a connection to the service
        self.name = name
        self.is_om_node = is_om_node
        
    def test(self):
        inventory = ansible.inventory.Inventory("cody_ec2", vault_password="Formation1234")
        
        pb = ansible.playbook.PlayBook(
                    playbook="deploy_fds_ec2.yml",
                    stats=self.stats,
                    inventory=inventory,
                    callbacks=self.playbook_cb,
                    runner_callbacks=self.runner_cb,
                    check=True
        )
        #for (play_ds, play_basedir) in zip(pb.playbook, pb.play_basedirs):
        #    import ipdb
        #    ipdb.set_trace()
            # can play around here to see what's going on.
        rb.run()

    def start_instance(self):
        '''
        Create a single EC2 instance, using Ansible's Python API
        
        '''
        # construct the ansible runner and execute on the host
        results = ansible.runner.Runner(
            pattern='*', forks=1,
            module_name="ec2",
        ).run()
        
        if results is None:
            error = {
                'code': 200,
                'method': __class__.__name__ + "::start_intance()",
                'msg': "failed to create EC2 instance"
            }
            self.log.exception(error)
            raise error_codes.Ec2FailedToInstantiate(error)

        self.log.info("UP **************")
        for (hostname, result) in results['contacted'].items():
            if 'failed' in results:
                error = {
                    code: 201,
                    'method': __class__.__name__ + "::start_intance()",
                    msg: "%s >>> %s" % (hostname, result['stdout'])
                }
                self.log.exception(error)
                raise error_codes.Ec2FailedToInitialize(message)
            else:
                return hostname

    def stop_instance(self):
        pass
        
    def terminate_instance(self):
        pass
        
    def check_status(self):
        return self.instance.status
    
    def add_volume(self, size, device):
        pass

if __name__ == "__main__":
    ec2 = EC2(name="testec2")
    ec2.test()