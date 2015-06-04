import time
from model.fds_id import FdsId

class Snapshot( object ):
    '''
    Created on Apr 20, 2015
    
    Snapshot object
    
    @author: nate
    '''

    def __init__(self, expiration=0, timeline_time=0, an_id=FdsId(), volume_id=FdsId()):
        self.expiration = expiration
        self.timeline_time = timeline_time
        self.volume_id = volume_id
        self.id = an_id
        self.created = time.time() * 1000
     
    @property
    def id(self):
        return self.__id
    
    @id.setter
    def id(self, an_id):
        
        if not isinstance(an_id, FdsId):
            raise TypeError()
        
        self.__id = an_id     
        
    @property
    def expiration(self):
        return self.__expiration
    
    @expiration.setter
    def expiration(self, expiration):
        self.__expiration = expiration
        
    @property
    def timeline_time(self):
        return self.__timeline_time
    
    @timeline_time.setter
    def timeline_time(self, aTime):
        self.__timeline_time = aTime
        
    @property
    def volume_id(self):
        return self.__volume_id
    
    @volume_id.setter
    def volume_id(self, an_id):
        
        if not isinstance(an_id, FdsId):
            raise TypeError()
        
        self.__volume_id = an_id
        
    @property
    def created(self):
        return self.__created
    
    @created.setter
    def created(self, aTime ):
        self.__created = aTime
