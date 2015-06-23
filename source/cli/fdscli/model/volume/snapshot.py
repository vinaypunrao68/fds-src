import time
from model.base_model import BaseModel

class Snapshot( BaseModel ):
    '''
    Created on Apr 20, 2015
    
    Snapshot object
    
    @author: nate
    '''

    def __init__(self, retention=0, an_id=-1, volume_id=-1, name=None):
        BaseModel.__init__(self, an_id, name)
        self.retention = retention
        self.volume_id = volume_id
        self.created = time.time()
     
    @property
    def retention(self):
        return self.__retention
    
    @retention.setter
    def retention(self, retention):
        self.__retention = retention
        
    @property
    def volume_id(self):
        return self.__volume_id
    
    @volume_id.setter
    def volume_id(self, an_id):
        self.__volume_id = an_id
        
    @property
    def created(self):
        return self.__created
    
    @created.setter
    def created(self, aTime ):
        self.__created = aTime
