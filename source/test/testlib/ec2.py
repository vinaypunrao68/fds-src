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
import sqlite
import aws_config
import error_codes
import config


class EC2(object):
    '''
    The class EC2 is responsible for instantiating, deleting and halting AWS instances,
    as well as adding, attaching, detaching and volumes. Each EC2 object has a single
    single AWS instance it manages.
    
    Attributes:
    -----------
    name: str
        the name of the instance
    region: str
        the AWS region where the instance will be created. Defaults to 'us-west-2'
    image: str
        the AWS image the user pretends to use. Defaults to 'ami-1fa78f2f'
    instance_type: str
        the type of the AWS instance. Defaults to 'm3.medium'
    is_om_node: str
        specify if the node being created is OM.
    '''
    
    # static elements of the EC2 class
    logging.basicConfig(level=logging.INFO,
                        format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
                        datefmt='%m-%d %H:%M')
    log = logging.getLogger(__name__)
    
    def __init__(self, name, region=aws_config.AWS_REGION, image=aws_config.AWS_IMAGE,
                 instance_type=aws_config.AWS_INSTANCE_TYPE, is_om_node=False):
        
        self.region = region
        self.name = name
        # establish the connection with AWS
        (access_key, secret_key) = sqlite.get_aws_credentials()
        self.ec2_conn = boto.ec2.connect_to_region(self.region,
                                                   aws_access_key_id=access_key,
                                                   aws_secret_access_key=secret_key)
        self.instance = None
        self.reservation = None

        self.volumes = {}

        self.image = image
        self.instance_type = instance_type
        self.is_om_node = is_om_node
        # add a 300 seconds timeout for each operation (5 minutes)
        self.max_wait = 300
        
    def start_instance(self):
        '''
        Start the instance, but first checks if hasn't been instantiated.
        '''
        if self.instance != None:
            self.log.warning("Instance has already been instantiated. Nothing to do")
            return
        try:
            self.reservation = self.ec2_conn.run_instances(self.image,
                                                           instance_type=self.instance_type)
            self.instance = self.reservation.instances[0]
            self.instance.add_tag('Name', self.name)
            timeout = time.time() + self.max_wait
            while self.instance.state != 'running' and time.time() <= timeout:
                time.sleep(10)
                self.instance.update() # Updates Instance metadata
                self.log.info("Instance state: %s" % (self.instance.state))

            self.log.info("instance %s done!" % (self.instance.id))
            self.log.info("instance IP is %s" % (self.instance.ip_address))
        except Exception, e:
            # @TODO: please add the error code here (Philippe)
            self.log.exception(e)
    
    def get_volumes_status(self, id):
        '''
        Return the current status of the volume, given its ID
        
        Attributes:
        -----------
        id: int
            the volume ID
            
        Returns:
        --------
        str : the volume status
        '''
        if self.volumes == {}:
            self.log.warning("No volumes attached yet.")
            return None
        for volume in self.volumes.keys():
            if volume.id == id:
                return volume.status
    
    def resume_instance(self):
        '''
        If the AWS instance is on a 'stopped' (halted) state, the user can bring it back by running
        the resume_instance method. All the volumes and state should be saved in the instance.
        '''
        if self.instance == None:
            self.log.warning("Instance hasn't been created yet.")
            return
        elif self.instance.state != 'stopped':
            self.log.warning("Instance is not in a stopped state. Can't be resumed.")
            return
        else:
            self.instance.start()
            timeout = time.time() + self.max_wait
            while self.instance.state != 'running' and time.time() <= timeout:
                time.sleep(10)
                self.instance.update() # Updates Instance metadata
                self.log.info("Instance state: %s" % (self.instance.state))

    def get_instance_status(self):
        '''
        Returns the instance status
        
        Returns:
        --------
        str: the status
        '''
        if self.instance == None:
            self.log.warning("Instance hasn't been created yet.")
            return None
        else:
            return self.instance.state

    def stop_instance(self):
        '''
        If the machine is on a 'running' state, the user has the option to stop (halt) such instance
        This will cause the machine to not consume AWS resources, however any EBS volume attached to
        it will continue to exist. After that, the user can resume the machine and bring instance
        to the previous state.
        '''
        if self.instance == None:
            self.log.warning("Instance hasn't been created yet. Nothing to do.")
        elif self.instance.state == 'stopped':
            self.log.info("Instance is stopped. Nothing to do.")
        elif self.instance.state == 'terminated':
            self.log.info("Instance is terminated. Nothing to do.")
        elif self.instance.state == 'running':
            self.log.info("Attempting to stop instance: %s" % self.instance.id)
            self.ec2_conn.stop_instances(instance_ids=[self.instance.id])
            timeout = time.time() + self.max_wait
            while self.instance.state != 'stopped' and time.time() <= timeout:
                time.sleep(10)
                self.instance.update()
                self.log.info("Instance state: %s" % (self.instance.state))
            self.log.info("Instance %s stopped" % self.instance.id)
        else:
            self.log.warning("Unknown instance state, do nothing.")
            
    def terminate_instance(self):
        '''
        Once the instance has been created, and it's on a 'running' or 'stopped' state,
        the user can terminate (destroy) the instance, and any volume attached to it will also be
        deleted. User will no longer have this instance.
        '''
        if self.instance.state not in ('running', 'stopped'):
            self.log.warning("Cannot terminate. It's not in a 'running' or 'stopped' state")
        else:
            self.log.info("Attempting to terminate instance: %s" % self.instance.id)
            self.ec2_conn.terminate_instances(instance_ids=[self.instance.id])
            timeout = time.time() + self.max_wait
            while self.instance.state != 'terminated' and time.time() <= timeout:
                time.sleep(10)
                self.instance.update()
                self.log.info("Instance state: %s" % (self.instance.state))
            self.log.info("Instance %s terminated" % self.instance.id)
            self.instance == None
    
    def delete_volume(self, volume):
        '''
        If the volume object is a valid one, delete is from EBS
        
        Attributes:
        -----------
        volume: The EBS volume object
        
        Returns:
        --------
        bool: the result of the delete volume operation
        '''
        if volume is None:
            self.log.warning("Volume object is Null. Nothing to do.")
            return False
        else:
            self.log.info("Deleting volume %s" % volume.id)
            return volume.delete()

    def detach_volume(self, device):
        '''
        Given a volume, i.e. /dev/sba, which has been attached to this instance,
        detach and delete the volume
        
        Attributes:
        -----------
        device: str
            the name of the device mounted: /dev/sda

        Returns:
        --------
        bool: True if the operation has been completed successfully, False otherwise. 
        '''
        if self.instance == None:
            self.log.warning("Instance hasn't being instantiated yet.")
            return False
        elif device not in self.volumes:
            self.log.warning("Device %s is not attached" % device)
            return False
        else:
            volume = self.volumes[device]
            # if self.ec2_conn.attach_volume(volume.id, self.instance.id, device, force=True):
            if volume.detach(force=True):
                self.log.info("Device %s has been detached from %s" % (device, self.instance.id))
                return volume.attachment_state()
                del self.volumes[device]
            return True
            
    def attach_volume(self, size, device):
        '''
        Create and attach a volume to the instance. First check if volume is already attached or
        available before doing any operation.

        Attributes:
        -----------
        size: int
            the size of the volume, in GB. Has to be between 1GB and 1TB
        device: str
            the device mount point name, i.e /dev/sdx
            
        Returns:
        --------
        bool : True if volume successfully created and attached. False otherwise
        '''
        if self.instance == None:
            self.log.warning("Instance hasn't being instantiated yet.")
            return False
        if device in self.volumes:
            self.log.warning("Device %s already attached" % device)
            return False
        try:
            # assert the volume being created is greater than 1GB but less than 1TB
            assert (size >= 1 and size <= 1024)
            volume = self.ec2_conn.create_volume(size, self.instance.placement)
            timeout = time.time() + self.max_wait
            while volume.status != 'available' and time.time() <= timeout:
                time.sleep(10)
                self.log.info("Volume status: %s" % volume.status)
                volume.update()
            
            self.log.info("Attempting to attach volume %s to instance %s" % (volume.id,
                                                                             self.instance.id))
            if not volume.attach(self.instance.id, device):
                raise Exception("Failed to attach volume %s to instance %s" % (volume.id,
                                                                             self.instance.id))
                sys.exit(2)
            timeout = time.time() + self.max_wait
            while volume.attachment_state() == 'attaching' and time.time() <= timeout:
                    self.log.info('Attachment state: ', volume.attach_data.status)
                    time.sleep(10)
                    volume.update()
            # delete the volume on termination
            self.instance.modify_attribute('blockDeviceMapping', {device : True})
            self.volumes[device] = volume
            return True
        except Exception, e:
            # @TODO: please add the error code here (Philippe)
            self.log.exception(e)