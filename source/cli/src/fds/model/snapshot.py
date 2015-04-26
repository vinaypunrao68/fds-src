import time

class Snapshot( object ):
    '''
    Created on Apr 20, 2015
    
    Snapshot object
    
    @author: nate
    '''

    def __init__(self, name="Unnamed", retention=0, timeline_time=0):
        self.__name = name
        self.__retention = retention
        self.__timeline_time = timeline_time
        self.__volume_id = None
        self.__id = -1
        self.__created = time.time() * 1000
       
    @property 
    def name(self):
        return self.__name
    
    @name.setter
    def name(self, name):
        self.__name = name
        
    @property
    def retention(self):
        return self.__retention
    
    @retention.setter
    def retention(self, retention):
        self.__retention = retention
        
    @property
    def id(self):
        return self.__id
    
    @id.setter
    def id(self, an_id):
        self.__id = an_id
        
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
        self.__volume_id = an_id
        
    @property
    def created(self):
        return self.__created
    
    @created.setter
    def created(self, aTime ):
        self.__created = aTime
