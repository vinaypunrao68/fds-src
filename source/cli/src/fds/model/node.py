class Node(object):
    '''
    Created on Apr 27, 2015
    
    @author: nate
    '''

    def __init__(self, an_id=-1, name="", state="UP", ip_v4_address=None):
        self.__ip_v4_address = ip_v4_address
        self.__ip_v6_address = None
        self.__id = an_id
        self.__name = name
        self.__state = state
        self.__services = dict()
        self.__services["AM"] = []
        self.__services["DM"] = []
        self.__services["SM"] = []
        self.__services["PM"] = []
        self.__services["OM"] = []
        
        return 
    
    @property
    def name(self):
        return self.__name
    
    @name.setter
    def name(self, a_name):
        self.__name = a_name
        
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
    def id(self):
        return self.__id
    
    @id.setter
    def id(self, an_id):
        self.__id = an_id
        
    @property
    def state(self):
        return self.__state
    
    @state.setter
    def state(self, a_state):
        self.__state = a_state
        
    @property
    def services(self):
        return self.__services
    
    @services.setter
    def services(self, services):
        self.__services = services