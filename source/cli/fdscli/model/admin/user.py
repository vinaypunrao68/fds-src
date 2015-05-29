from model.fds_id import FdsId
from model.admin.role import Role

class User(object):
    '''
    Created on May 18, 2015
    
    @author: nate
    '''
    
    def __init__(self, an_id=FdsId(), tenant_id=FdsId(), role=Role()):
        self.id = an_id
        self.tenant_id = tenant_id
        self.role = role
    
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
    def tenant_id(self):
        return self.__tenant_id
    
    @tenant_id.setter
    def tenant_id(self, tenant_id):
        
        if not isinstance(tenant_id, FdsId):
            raise TypeError()
            return
        
        self.__tenant_id = tenant_id
        
    @property
    def role(self):
        return self.__role
    
    @role.setter
    def role(self, role):
        
        if not isinstance(role, Role):
            raise TypeError()
        
        self.__role = role
    
    
