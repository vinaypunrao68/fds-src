from fds.model.admin.user import User
import json
from fds.utils.converters.fds_id_converter import FdsIdConverter
from fds.utils.converters.admin.role_converter import RoleConverter

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
        
        if not isinstance(user, User):
            raise TypeError()
        
        j_user = dict()
        
        j_user["uid"] = user.id
        j_user["name"] = user.name
        j_user["tenant_id"] = user.tenant_id
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
        
        user.tenant_id = j_user.pop( "tenant_id", user.tenant_id )
        user.role = j_user.pop("roleDescriptor", user.role )
        
        return user
