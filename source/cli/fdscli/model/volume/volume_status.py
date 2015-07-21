from model.common.size import Size

class VolumeStatus(object):
    '''
    Created on May 29, 2015
    
    @author: nate
    '''

    def __init__(self, state="ACTIVE", current_usage=Size(0,"GB"), last_capacity_firebreak=0, last_performance_firebreak=0):
        self.state = state
        self.current_usage = current_usage
        self.last_capacity_firebreak = last_capacity_firebreak
        self.last_performance_firebreak = last_performance_firebreak
        
    @property
    def state(self):
        return self.__state
    
    @state.setter
    def state(self, state):
        
#         if state not in ("ACTIVE", "INACTIVE", "AVAILABLE", "NOT_AVAILABLE"):
#             raise TypeError()
        
        self.__state = state
        
    @property
    def current_usage(self):
        return self.__current_usage
    
    @current_usage.setter
    def current_usage(self, size):
        
#         if not isinstance(size, Size):
#             raise TypeError()
        
        self.__current_usage = size
        
    @property
    def last_capacity_firebreak(self):
        return self.__last_capacity_firebreak
    
    @last_capacity_firebreak.setter
    def last_capacity_firebreak(self, a_time):
        self.__last_capacity_firebreak = a_time
        
    @property
    def last_performance_firebreak(self):
        return self.__last_performance_firebreak
    
    @last_performance_firebreak.setter
    def last_performance_firebreak(self, a_time):
        self.__last_performance_firebreak = a_time
