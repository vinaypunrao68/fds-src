class Role(object):
    '''
    Created on May 29, 2015
    
    @author: nate
    '''

    def __init__(self, name="TENANT_ADMIN", features=["VOL_MGMT", "USER_MGMT"] ):
        self.name = name
        self.features = features
        
    @property
    def name(self):
        return self.__name
    
    @name.setter
    def name(self, name):
        
        if name not in ("TENANT_ADMIN", "ADMIN", "USER"):
            raise TypeError()
        
        self.__name = name
        
    @property
    def features(self):
        return self.__features
    
    @features.setter
    def features(self, features):
        
        self.__features = features