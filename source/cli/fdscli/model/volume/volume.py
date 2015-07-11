from model.volume.qos_policy import QosPolicy
from model.volume.data_protection_policy import DataProtectionPolicy
from model.volume.settings.object_settings import ObjectSettings
from model.volume.volume_status import VolumeStatus
from model.volume.settings.volume_settings import VolumeSettings
from model.base_model import BaseModel
from model.volume.data_protection_policy_preset import DataProtectionPolicyPreset

class Volume( BaseModel ):
    '''
    Created on Apr 3, 2015
    
    Model object representing a volume
    
    @author: nate
    '''
    
    def __init__(self, an_id=-1, name=None, qos_policy=QosPolicy(), data_protection_policy=DataProtectionPolicy(), 
                 settings=ObjectSettings(), media_policy="HYBRID", tenant=None, application="None",
                 status=VolumeStatus(), creation_time=0 ):
        
        BaseModel.__init__(self, an_id, name)
        self.media_policy = media_policy
        self.qos_policy = qos_policy
        self.data_protection_policy = data_protection_policy
        self.application = application
        self.status = status
        self.tenant = tenant
        self.settings = settings
        self.creation_time = creation_time
        
    @property
    def settings(self):
        return self.__settings
    
    @settings.setter
    def settings(self, settings):
        self.__settings = settings
        
    @property
    def qos_policy(self):
        return self.__qos_policy
    
    @qos_policy.setter
    def qos_policy(self, policy):
        
#         if not isinstance(policy, QosPolicy):
#             raise TypeError()
        
        self.__qos_policy = policy
        
    @property
    def data_protection_policy(self):
        return self.__data_protection_policy
    
    @data_protection_policy.setter
    def data_protection_policy(self, policy):
        
        self.__data_protection_policy = policy
        
    @property
    def status(self):
        return self.__status
    
    @status.setter
    def status(self, status): 
    
        self.__status = status
        
    @property
    def application(self):
        return self.__application
    
    @application.setter
    def application(self, application):
        self.__application = application
        
    @property
    def media_policy(self):
        return self.__media_policy
    
    @media_policy.setter
    def media_policy(self, aPolicy):
        
        if aPolicy not in ("HDD", "SSD", "HYBRID"):
            raise TypeError()
                
        self.__media_policy = aPolicy
        
    @property
    def tenant(self):
        return self.__tenant
    
    @tenant.setter
    def tenant(self, tenant):
        self.__tenant = tenant
        
    @property
    def creation_time(self):
        return self.__creation_time
    
    @creation_time.setter
    def creation_time(self, creation_time):
        self.__creation_time = creation_time
