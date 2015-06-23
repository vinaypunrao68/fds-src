'''
Created on Apr 9, 2015

@author: nate
'''
from services.fds_auth import FdsAuth

class MockFdsAuth(FdsAuth):
    
    def __init__(self):
        FdsAuth.__init__(self)
    
    def login(self):
    
        self.__token = "thisisavalidmocktoken"
        self.__features = ["VOL_MGMT", "TENANT_MGMT", "USER_MGMT"]
        
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
    
    def is_allowed(self, feature):
        
        for capability in self.__features:
            if ( capability == feature ):
                return True
        # end of for loop
        
        return False
    
    def get_user_id(self):
        return 1
