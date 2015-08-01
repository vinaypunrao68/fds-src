'''
Created on Apr 9, 2015

@author: nate
'''
from services.fds_auth import FdsAuth

class MockFdsAuth(FdsAuth):
    
    def __init__(self):
        FdsAuth.__init__(self)
        self.__features = [FdsAuth.VOL_MGMT, FdsAuth.TENANT_MGMT, FdsAuth.USER_MGMT, FdsAuth.SYS_MGMT]
    
    def login(self):
    
        self.__token = "thisisavalidmocktoken"
        
        return "thisisavalidmocktoken"
    
    def add_feature(self, feature):
        '''
        allows a way to alter the features list
        '''
        self.__features.append( feature )
        
    def remove_feature(self, bad_feature):
        '''
        allows a way to remove a feature from the list
        '''
        
        for feature in self.__features:
            if feature == bad_feature:
                self.__features.remove(feature)
                break
    
    def set_features(self, features):
        
        self.__features = features
    
    def is_allowed(self, feature):
        
        for capability in self.__features:
            if ( capability == feature ):
                return True
        # end of for loop
        
        return False
    
    def is_authenticated(self):
        
        if ( self.__token is not None ):
            return True
        
        return False    
    
    def logout(self):
        self.__token = None
    
    def get_user_id(self):
        return 1
