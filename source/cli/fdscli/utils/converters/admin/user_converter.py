from model.admin.user import User
import json
from utils.converters.fds_id_converter import FdsIdConverter
from utils.converters.admin.role_converter import RoleConverter
from utils.converters.admin.tenant_converter import TenantConverter

class UserConverter(object):
    '''
    Created on May 18, 2015
    
    @author: nate
    '''
    
    @staticmethod
    def to_json(user):
        '''
        Convert a user object into a JSON representation
        '''
        
#         if not isinstance(user, User):
#             raise TypeError()
        
        j_user = dict()
        
        j_user["uid"] = user.id
        j_user["name"] = user.name
        
        if user.tenant is not None:
            j_user["tenant"] = TenantConverter.to_json(user.tenant)
            
        j_user["roleDescriptor"] = user.role
        
        if user.password is not None:
            j_user["password"] = user.password
        
        j_user = json.dumps(j_user)
        
        return j_user
    
    @staticmethod
    def build_user_from_json(j_user):
        
        user = User()
        
        if not isinstance(j_user, dict):
            j_user = json.loads(j_user) 
        
        user.id = j_user.pop( "uid", user.id )
        user.name = j_user.pop( "name", user.name )
        
        if "tenant" in j_user:
            user.tenant = TenantConverter.build_tenant_from_json( j_user.pop("tenant") )
        
        user.role = j_user.pop("roleDescriptor", user.role )
        
        return user
