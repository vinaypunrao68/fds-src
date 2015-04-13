'''
Created on Apr 3, 2015

@author: nate
'''
import json

class Volume():
    
    __id = -1
    __name = ""
    __priority = 7
    __type = "object"
    __iops_limit = 0
    __iops_guarantee = 0
    __continuous_protection = 60400
    __media_policy = "hybrid"
    __state = "Inactive"
    __current_size = 0
    __current_units = "B"
    
    @staticmethod
    def build_volume_from_json( json ):

        volume = Volume();
        
        volume.set_name( json.pop( "name", None ) )
        volume.set_iops_guarantee( json.pop( "sla", None ) )
        volume.set_iops_limit( json.pop( "limit", None ) )
        volume.set_id( json.pop( "id", None ) )
        volume.set_media_policy( json.pop( "mediaPolicy", None ) )
        
        dc = json.pop( "data_connector", None )
        volume.set_type( dc.pop( "type", None ) )
        
        volume.set_state( json.pop( "state", None ) )
        
        cu = json.pop( "current_usage", None )
        volume.set_current_size( cu.pop( "size", None ) )
        volume.set_current_units( cu.pop( "unit", None ) )
        
        return volume
    
    def get_id(self):
        return self.__id
    
    def set_id(self, anId ):
        self.__id = anId
    
    def get_name(self):
        return self.__name
    
    def set_name(self, aName ):
        self.__name = aName
    
    def get_state(self):
        return self.__state
    
    def set_state(self, aState):
        self.__state = aState
    
    def get_priority(self):
        return self.__priority
    
    def set_priority(self, aPriority):
        self.__priority = aPriority
    
    def get_type(self):
        return self.__type
    
    def set_type(self, aType):
        self.__type = aType
        
    def get_iops_limit(self):
        return self.__iops_limit
    
    def set_iops_limit(self, aLimit):
        self.__iops_limit = aLimit
        
    def get_iops_guarantee(self):
        return self.__iops_guarantee
    
    def set_iops_guarantee(self, aMin):
        self.__iops_guarantee = aMin
        
    def get_continuous_protection(self):
        return self.__continuous_protection 
        
    def set_continuous_protection(self, seconds):
        self.__continuous_protection = seconds
        
    def get_media_policy(self):
        return self.__media_policy
    
    def set_media_policy(self, aPolicy):
        self.__media_policy = aPolicy 
        
    def get_current_size(self):
        return self.__current_size
    
    def set_current_size(self, aSize ):
        self.__current_size = aSize
        
    def get_current_units(self):
        return self.__current_units
    
    def set_current_units(self, units):
        self.__current_units = units
    