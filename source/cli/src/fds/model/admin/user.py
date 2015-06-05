from fds.model.base_model import BaseModel

class User(BaseModel):
    '''
    Created on May 18, 2015
    
    @author: nate
    '''
    
    def __init__(self, an_id=-1, name="None", tenant_id=-1, role="USER"):
        BaseModel.__init__(self, an_id, name)
        self.tenant_id = tenant_id
        self.role = role
        self.password = None
        
    @property
    def tenant_id(self):
        return self.__tenant_id
    
    @tenant_id.setter
    def tenant_id(self, tenant_id):
        self.__tenant_id = tenant_id
        
    @property
    def role(self):
        return self.__role
    
    @role.setter
    def role(self, role):
        
        if role not in ("ADMIN", "USER"):
            raise TypeError()
        
        self.__role = role
        
    @property
    def password(self):
        return self.__password
    
    @password.setter
    def password(self, password):
        self.__password = password
    
    
