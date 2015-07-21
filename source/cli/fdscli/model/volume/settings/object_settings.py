from model.volume.settings.volume_settings import VolumeSettings
from model.common.size import Size

class ObjectSettings(VolumeSettings):
    '''
    Created on May 29, 2015
    
    @author: nate
    '''

    def __init__(self, max_object_size=None):
        self.type = "OBJECT"
        self.max_object_size = max_object_size
        
    @property
    def max_object_size(self):
        return self.__max_object_size
    
    @max_object_size.setter
    def max_object_size(self, size):
        
        self.__max_object_size = size
