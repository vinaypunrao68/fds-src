
class TimelinePreset(object):
    '''
    Created on May 6, 2015
    
    @author: nate
    '''
    
    def __init__(self, name="", an_id=-1, continuous_protection=86400, policies=list()):
        self.__name = name
        self.__id = an_id
        self.__continuous_protection = continuous_protection
        self.__policies = policies
        
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
    def continuous_protection(self):
        return self.__continuous_protection
    
    @continuous_protection.setter
    def continuous_protection(self, a_time):
        self.__continuous_protection = a_time
        
    @property
    def policies(self):
        return self.__policies
    
    @policies.setter
    def policies(self, policies):
        self.__policies = policies