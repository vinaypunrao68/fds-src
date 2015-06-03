
class QosPreset( object ):
    '''
    Created on May 6, 2015
    
    @author: nate
    '''
    
    def __init__(self, priority=7, iops_guarantee=0, iops_limit=0, name="", an_id=-1):
        
        self.__priority = priority
        self.__iops_guarantee = iops_guarantee
        self.__iops_limit = iops_limit
        self.__name = name
        self.__id = an_id
        
    @property
    def id(self):
        return self.__id
    
    @id.setter
    def id(self, an_id):
        self.__id = an_id
        
    @property
    def name(self):
        return self.__name
    
    @name.setter
    def name(self, name):
        self.__name = name
        
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

