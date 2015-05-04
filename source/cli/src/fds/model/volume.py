class Volume( object ):
    '''
    Created on Apr 3, 2015
    
    Model object representing a volume
    
    @author: nate
    '''
    
    def __init__(self, name="Unnamed", priority=7, volume_type="object", iops_limit=0, iops_guarantee=0, media_policy="HYBRID_ONLY",
                 current_size=10, current_units="GB", tenant_id=0, continuous_protection=86400 ):
        
        self.__id = -1
        self.__name = name
        self.__priority = priority
        self.__type = volume_type
        self.__iops_limit = iops_limit
        self.__iops_guarantee = iops_guarantee
        self.__continuous_protection = continuous_protection
        self.__media_policy = media_policy
        self.__state = "Inactive"
        self.__current_size = current_size
        self.__current_units = current_units
        self.__last_performance_firebreak = 0
        self.__last_capacity_firebreak = 0
        self.__tenant_id = tenant_id
    
    @property
    def id(self):
        return self.__id;
    
    @id.setter
    def id(self, anId):
        self.__id = anId
        
    @property
    def name(self):
        return self.__name
    
    @name.setter
    def name(self, aName):
        self.__name = aName
        
    @property
    def priority(self):
        return self.__priority
    
    @priority.setter
    def priority(self, aPriority):
        self.__priority = aPriority
        
    @property
    def type(self):
        return self.__type
    
    @type.setter
    def type(self, aType):
        self.__type = aType
        
    @property
    def iops_limit(self):
        return self.__iops_limit
    
    @iops_limit.setter
    def iops_limit(self, aLimit):
        self.__iops_limit = aLimit
        
    @property 
    def iops_guarantee(self):
        return self.__iops_guarantee

    @iops_guarantee.setter
    def iops_guarantee(self, anSla):
        self.__iops_guarantee = anSla
        
    @property
    def continuous_protection(self):
        return self.__continuous_protection
    
    @continuous_protection.setter
    def continuous_protection(self, someSeconds):
        self.__continuous_protection = someSeconds
        
    @property
    def media_policy(self):
        return self.__media_policy
    
    @media_policy.setter
    def media_policy(self, aPolicy):
        self.__media_policy = aPolicy
        
    @property
    def state(self):
        return self.__state
        
    @state.setter
    def state(self, aState):
        self.__state = aState
        
    @property
    def current_size(self):
        return self.__current_size
    
    @current_size.setter
    def current_size(self, aSize):
        self.__current_size = aSize
        
    @property
    def current_units(self):
        return self.__current_units
    
    @current_units.setter
    def current_units(self, someUnits):
        self.__current_units = someUnits
        
    @property
    def tenant_id(self):
        return self.__tenant_id
    
    @tenant_id.setter
    def tenant_id(self, anId):
        self.__tenant_id = anId
        
    @property
    def last_performance_firebreak(self):
        return self.__last_performance_firebreak
    
    @last_performance_firebreak.setter
    def last_performance_firebreak(self, aTime):
        self.__last_performance_firebreak = aTime
        
    @property
    def last_capacity_firebreak(self):
        return self.__last_capacity_firebreak
    
    @last_capacity_firebreak.setter
    def last_capacity_firebreak(self, aTime):
        self.__last_capacity_firebreak = aTime