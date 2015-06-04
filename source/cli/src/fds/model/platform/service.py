from model.fds_id import FdsId
from model.platform.service_status import ServiceStatus

class Service(object):
    '''
    Created on Apr 28, 2015
    
    @author: nate
    '''

    def __init__(self, status=ServiceStatus(), port=0, a_type="FDSP_PLATFORM", an_id=FdsId()):
        self.status = status
        self.port = port
        self.type = a_type
        self.id = an_id
    
    @property
    def id(self):
        return self.__id
    
    @id.setter
    def id(self, an_id):
        
        if not isinstance(an_id, FdsId):
            raise TypeError()
        
        self.__id = an_id
        
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
        
        if not isinstance(status, ServiceStatus):
            raise TypeError()
            return
        
        self.__status = status
      
    @property  
    def type(self):
        return self.__type
    
    @type.setter
    def type(self, a_type):
        
        if ( a_type not in ("FDSP_ACCESS_MGR", "FDSP_STOR_MGR", "FDSP_ORCH_MGR", "FDSP_PLATFORM", "FDSP_DATA_MGR")):
            raise TypeError()
            return
        
        self.__type = a_type