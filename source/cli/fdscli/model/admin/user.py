from model.base_model import BaseModel

class User(BaseModel):
    '''
    Created on May 18, 2015
    
    @author: nate
    '''
    
    def __init__(self, an_id=-1, name="None", tenant=None, role="USER"):
        BaseModel.__init__(self, an_id, name)
        self.tenant = tenant
        self.role = role
        self.password = None
        
    @property
    def tenant(self):
        return self.__tenant
    
    @tenant.setter
    def tenant(self, tenant):
        self.__tenant = tenant
        
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
    
    
