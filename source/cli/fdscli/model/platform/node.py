from model.fds_id import FdsId
class Node(object):
    '''
    Created on Apr 27, 2015
    
    @author: nate
    '''

    def __init__(self, an_id=FdsId(), name="", state="UP", ip_v4_address=None):
        self.ip_v4_address = ip_v4_address
        self.ip_v6_address = None
        self.id = an_id
        self.state = state
        self.services = dict()
        self.services["AM"] = []
        self.services["DM"] = []
        self.services["SM"] = []
        self.services["PM"] = []
        self.services["OM"] = []
        
        return 
    
    @property
    def id(self):
        return self.__id
    
    @id.setter
    def id(self, an_id):
        
        if not isinstance(an_id, FdsId):
            raise TypeError()
            return
        
        self.__id = an_id
    
    @property
    def ip_v4_address(self):
        return self.__ip_v4_address
    
    @ip_v4_address.setter
    def ip_v4_address(self, an_ip):
        self.__ip_v4_address = an_ip
        
    @property
    def ip_v6_address(self):
        return self.__ip_v6_address
    
    @ip_v6_address.setter
    def ip_v6_address(self, an_ip):
        self.__ip_v6_address = an_ip
        
    @property
    def state(self):
        return self.__state
    
    @state.setter
    def state(self, a_state):
        
        if ( a_state not in ("UP", "DOWN", "UNKNOWN") ):
            raise TypeError()
            return
        
        self.__state = a_state
        
    @property
    def services(self):
        return self.__services
    
    @services.setter
    def services(self, services):
        self.__services = services