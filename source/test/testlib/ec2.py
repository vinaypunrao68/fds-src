##############################################################################
# author: Philippe Ribeiro                                                   #
# email: philippe@formationds                                                #
# file: ec2.py                                                               #
##############################################################################

import boto.ec2
import logging
import sys
import time

# Import local defined libraries
import aws_config
import error_codes
import config

# aws_access_key: AKIAJAWXAU57VVTDDWAA
# aws_secret_key: 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
# 'ami-1fa78f2f', instance_type='m3.medium'

AWS_ACCESS_KEY = "AKIAJAWXAU57VVTDDWAA"
AWS_SECRET_KEY = "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"

class EC2(object):
    
    # static elements of the EC2 class
    logging.basicConfig(level=logging.INFO)
    log = logging.getLogger(__name__)
    
    def __init__(self, name, region=aws_config.AWS_REGION, image=aws_config.AWS_IMAGE,
                 instance_type=aws_config.AWS_INSTANCE_TYPE, is_om_node=False):
        
        self.region = region
        self.name = name
        self.ec2_conn = boto.ec2.connect_to_region(self.region,
                                                   aws_access_key_id=aws_config.AWS_ACCESS_KEY,
                                                   aws_secret_access_key=aws_config.AWS_SECRET_KEY)
        self.instance = None
        self.reservation = None

        self.volumes = {}

        self.image = image
        self.instance_type = instance_type
        self.is_om_node = is_om_node
        
    
    def start_instance(self):
        if self.instance is  None:
            self.log.warning("Instance has already been instantiated. Nothing to do")
            return
        try:
            self.reservation = self.ec2_conn.run_instances(self.image,
                                                           instance_type=self.instance_type)
            self.instance = self.reservation.instances[0]
            self.instance.add_tag('Name', self.name)

            while self.instance.state != 'running':
                time.sleep(10)
                self.instance.update() # Updates Instance metadata
                self.log.info("Instance state: %s" % (self.instance.state))

            self.log.info("instance %s done!" % (self.instance.id))
            self.log.info("instance IP is %s" % (self.instance.ip_address))
        except Exception, e:
            self.log.exception(e)
            # please add the error code here.
            msg = {
                'error': 101,
                'message': "Failed to instantiate EC2 instance: %s" % e
            }
            raise Ec2FailedToInstantiate(msg)
    
    def get_instance_status(self):
        if self.instance == None:
            self.log.warning("Instance hasn't been created yet.")
        else:
            return self.instance.status

    def stop_instance(self):
        if self.instance == None:
            self.log.warning("Instance hasn't been created yet. Nothing to do.")
        elif self.instance.state == 'stopped':
            self.log.info("Instance is stopped. Nothing to do.")
        elif self.instance.state == 'terminated':
            self.log.info("Instance is terminated. Nothing to do.")
        elif self.instance.state == 'running':
            self.log.info("Attempting to stop instance: %s" % self.instance.id)
            self.ec2_conn.stop_instances(instance_ids=[self.instance.id])
            while self.instance.state != 'stopped':
                time.sleep(10)
                self.instance.update()
                self.log.info("Instance state: %s" % (self.instance.state))
            self.log.info("Instance %s stopped" % self.instance.id)
        else:
            self.log.warning("Unknown instance state, do nothing.")
            
    def terminate_instance(self):
        if self.instance.state != 'running':
            self.log.warning("Cannot terminate instance. it's not in a 'running' state")
        else:
            self.log.info("Attempting to terminate instance: %s" % self.instance.id)
            self.ec2_conn.terminate_instances(instance_ids=[self.instance.id])
            while self.instance.state != 'stopped':
                time.sleep(10)
                self.instance.update()
                self.log.info("Instance state: %s" % (self.instance.state))
            self.log.info("Instance %s terminated" % self.instance.id)
            self.instance == None
    
    def attach_volume(self, size, device):
        if self.instance == None:
            self.log.warning("Instance hasn't being instantiated yet.")
            return
        if device in self.volumes:
            self.log.warning("Device %s already attached" % device)
            return
        try:
            # assert the volume being created is greater than 1GB but less than 1TB
            assert (size >= 1 and size <= 1024)
            volume = self.ec_conn.create_volume(size, self.region)
            while volume.status != 'available':
                time.sleep(10)
                self.log.info(volume.state)
            
            self.log.info("Attempting to attach volume %s to instance %s" % (volume.id,
                                                                             self.instance.id))
            self.ec_conn.attach_volume(volume.id, self.instance.id, device)
            self.volumes.append(volume)
        except Exception e:
            self.log.exception(e)
        
                                                

if __name__ == "__main__":
    ec2 = EC2(name="testlib")
    ec2.start_instance()