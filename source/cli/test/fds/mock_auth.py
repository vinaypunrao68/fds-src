'''
Created on Apr 9, 2015

@author: nate
'''
from fds.services.fds_auth import FdsAuth

class MockFdsAuth(FdsAuth):
    
    def __init__(self):
        FdsAuth.__init__(self)
    
    def login(self):
    
        self.__token = "thisisavalidmocktoken"
        self.__features = ["VOL_MGMT", "TENANT_MGMT"]
        
        return "thisisavalidmocktoken"
    
    def isAllowed(self, feature):
        
        for capability in self.__features:
            if ( capability == feature ):
                return True
        # end of for loop
        
        return False