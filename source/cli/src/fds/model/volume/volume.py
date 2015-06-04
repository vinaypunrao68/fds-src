from model.fds_id import FdsId
from model.volume.qos_policy import QosPolicy
from model.volume.data_protection_policy import DataProtectionPolicy
from model.volume.settings.object_settings import ObjectSettings
from model.volume.volume_status import VolumeStatus
from OpenSSL.rand import status
from model.volume.settings.volume_settings import VolumeSettings
class Volume( object ):
    '''
    Created on Apr 3, 2015
    
    Model object representing a volume
    
    @author: nate
    '''
    
    def __init__(self, an_id=FdsId(), qos_policy=QosPolicy(), data_protection_policy=DataProtectionPolicy(), 
                 settings=ObjectSettings(), media_policy="HYBRID_ONLY", tenant_id=FdsId(), application="None",
                 status=VolumeStatus() ):
        
        self.id = an_id
        self.media_policy = media_policy
        self.qos_policy = qos_policy
        self.data_protection_policy = data_protection_policy
        self.application = application
        self.status = status
        self.tenant_id = tenant_id
        self.settings = settings
        
    
    @property
    def id(self):
        return self.__id;
    
    @id.setter
    def id(self, anId):
        self.__id = anId
        
    @property
    def settings(self):
        return self.__settings
    
    @settings.setter
    def settings(self, settings):
        
        if not isinstance(settings, VolumeSettings):
            raise TypeError()
        
        self.__settings = settings
        
    @property
    def qos_policy(self):
        return self.__qos_policy
    
    @qos_policy.setter
    def qos_policy(self, policy):
        
        if not isinstance(policy, QosPolicy):
            raise TypeError()
        
        self.__qos_policy = policy
        
    @property
    def data_protection_policy(self):
        return self.__data_protection_policy
    
    @data_protection_policy.setter
    def data_protection_policy(self, policy):
        
        if not isinstance(policy, DataProtectionPolicy):
            raise TypeError()
        
        self.__data_protection_policy = policy
        
    @property
    def status(self):
        return self.__status
    
    @status.setter
    def status(self, status): 
        
        if not isinstance(status, VolumeStatus):
            raise TypeError()
        
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
        self.__media_policy = aPolicy
        
    @property
    def tenant_id(self):
        return self.__tenant_id
    
    @tenant_id.setter
    def tenant_id(self, anId):
        self.__tenant_id = anId