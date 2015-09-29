from model.platform.address import Address
from model.base_model import BaseModel

class Node(BaseModel):
    '''
    Created on Apr 27, 2015
    
    @author: nate
    '''

    def __init__(self, an_id=-1, name=None, state="UP", address=Address()):
        BaseModel.__init__(self, an_id, name)
        self.address = address
        self.state = state
        self.services = dict()
        self.services["AM"] = []
        self.services["DM"] = []
        self.services["SM"] = []
        self.services["PM"] = []
        self.services["OM"] = []
        
        return 
    
    @property
    def address(self):
        return self.__address
    
    @address.setter
    def address(self, an_ip):
        self.__address = an_ip
        
    @property
    def state(self):
        return self.__state
    
    @state.setter
    def state(self, a_state):
        
        if ( a_state not in ("UP", "DOWN", "UNKNOWN", "DISCOVERED", "REMOVED", "MIGRATION", "STANDBY") ):
            raise TypeError()
            return
        
        self.__state = a_state
        
    @property
    def services(self):
        return self.__services
    
    @services.setter
    def services(self, services):
        self.__services = services
