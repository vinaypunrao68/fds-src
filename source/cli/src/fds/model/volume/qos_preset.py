from fds.model.fds_id import FdsId

class QosPreset( object ):
    '''
    Created on May 6, 2015
    
    @author: nate
    '''
    
    def __init__(self, priority=7, iops_min=0, iops_max=0, an_id=FdsId):
        
        self.priority = priority
        self.iops_min = iops_min
        self.iops_max = iops_max
        self.id = an_id
        
    @property
    def id(self):
        return self.__id
    
    @id.setter
    def id(self, an_id):
        
        if not isinstance(an_id, FdsId):
            raise TypeError()
        
        self.__id = an_id
        
    @property
    def priority(self):
        return self.__priority
    
    @priority.setter
    def priority(self, priority):
        self.__priority = priority
        
    @property
    def iops_guarantee(self):
        return self.__iops_guarantee
    
    @iops_guarantee.setter
    def iops_guarantee(self, iops):
        self.__iops_guarantee = iops
        
    @property
    def iops_limit(self):
        return self.__iops_limit
    
    @iops_limit.setter
    def iops_limit(self, limit):
        self.__iops_limit = limit

