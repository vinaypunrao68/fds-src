from model.admin.user import User
import json
from utils.converters.fds_id_converter import FdsIdConverter
from utils.converters.admin.role_converter import RoleConverter

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
        
        j_user["id"] = json.loads(FdsIdConverter.to_json( user.id ))
        j_user["tenant_id"] = json.loads(FdsIdConverter.to_json( user.tenant_id ))
        j_user["role"] = json.loads(RoleConverter.to_json(user.role))
        
        j_user = json.dumps(j_user)
        
        return j_user
    
    @staticmethod
    def build_user_from_json(j_user):
        
        user = User()
        
        if not isinstance(j_user, dict):
            j_user = json.loads(j_user) 
        
        user.id = FdsIdConverter.build_id_from_json( j_user.pop("id" ) )
        user.tenant_id = FdsIdConverter.build_id_from_json( j_user.pop( "tenant_id" ) )
        user.role = RoleConverter.build_role_from_json( j_user.pop("role") )
        
        return user
