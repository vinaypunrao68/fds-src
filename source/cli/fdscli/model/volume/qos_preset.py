from model.base_model import BaseModel

class QosPreset( BaseModel ):
    '''
    Created on May 6, 2015
    
    @author: nate
    '''
    
    def __init__(self, priority=7, iops_min=0, iops_max=0, an_id=-1, name=None):
        BaseModel.__init__(self, an_id, name)
        self.priority = priority
        self.iops_min = iops_min
        self.iops_max = iops_max
        
    @property
    def priority(self):
        return self.__priority
    
    @priority.setter
    def priority(self, priority):
        self.__priority = priority
        
    @property
    def iops_min(self):
        return self.__iops_min
    
    @iops_min.setter
    def iops_min(self, iops):
        self.__iops_min = iops
        
    @property
    def iops_max(self):
        return self.__iops_min
    
    @iops_max.setter
    def iops_max(self, limit):
        self.__iops_max = limit

