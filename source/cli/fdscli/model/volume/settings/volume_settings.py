
class VolumeSettings(object):
    '''
    Created on May 29, 2015
    
    @author: nate
    '''

    def __init__(self, a_type="OBJECT"):
        self.type = a_type
        
    @property
    def type(self):
        return self.__type
    
    @type.setter
    def type(self, a_type):
        
        if a_type not in ("OBJECT", "BLOCK"):
            raise TypeError()
        
        self.__type = a_type