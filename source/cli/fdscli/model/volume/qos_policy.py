
class QosPolicy(object):
    '''
    Created on May 29, 2015
    
    @author: nate
    '''

    def __init__(self, priority=7, iops_min=0, iops_max=0):
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
    def iops_min(self, iops_min):
        self.__iops_min = iops_min
    
    @property
    def iops_max(self):
        return self.__iops_max
    
    @iops_max.setter
    def iops_max(self, iops_max):
        self.__iops_max = iops_max