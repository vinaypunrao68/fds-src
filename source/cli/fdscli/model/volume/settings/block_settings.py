from model.volume.settings.volume_settings import VolumeSettings
from model.common.size import Size

class BlockSettings(VolumeSettings):
    '''
    Created on May 29, 2015
    
    @author: nate
    '''

    def __init__(self, capacity=Size(10, "GB"), block_size=None):
        self.type = "BLOCK"
        self.capacity = capacity
        self.block_size = block_size
        
    @property
    def capacity(self):
        return self.__capacity
    
    @capacity.setter
    def capacity(self, size):
        
        self.__capacity = size
        
    @property
    def block_size(self):
        return self.__block_size
    
    @block_size.setter
    def block_size(self, size):
        
        self.__block_size = size

