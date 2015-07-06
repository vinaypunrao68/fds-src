
class Calculated(object):
    '''
    Created on Jun 29, 2015

    basically this is just a key value pair for now
    
    @author: nate
    '''
    
    def __init__(self, key=None, value=None):
        self.key = key
        self.value = value
        
    @property
    def key(self):
        return self.__key
    
    @key.setter
    def key(self, key):
        self.__key = key
        
    @property
    def value(self):
        return self.__value
    
    @value.setter
    def value(self, value):
        self.__value = value
