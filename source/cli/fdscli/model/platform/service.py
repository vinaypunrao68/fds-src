from model.platform.service_status import ServiceStatus
from model.base_model import BaseModel

class Service(BaseModel):
    '''
    Created on Apr 28, 2015
    
    @author: nate
    '''

    def __init__(self, status=ServiceStatus(), port=0, a_type="PM", an_id=-1, name=None):
        BaseModel.__init__(self, an_id, name)
        self.status = status
        self.port = port
        self.type = a_type
        
    @property
    def port(self):
        return self.__port
    
    @port.setter
    def port(self, port):
        self.__port = port
        
    @property
    def status(self):
        return self.__status
    
    @status.setter
    def status(self, status):
        
        self.__status = status
      
    @property  
    def type(self):
        return self.__type
    
    @type.setter
    def type(self, a_type):
        
        if ( a_type not in ("AM", "SM", "OM", "PM", "DM")):
            raise TypeError()
            return
        
        self.__type = a_type
