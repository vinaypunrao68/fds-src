from model.admin.role import Role
import json

class RoleConverter(object):
    '''
    Created on May 29, 2015
    
    @author: nate
    '''

    @staticmethod
    def to_json(role):
        
#         if not isinstance(role, Role):
#             raise TypeError()
        
        j_role = dict()
        
        j_role["name"] = role.name   
        j_role["features"] = role.features
        
        j_role = json.dumps(j_role)
        
        return j_role
    
    @staticmethod
    def build_role_from_json(j_str):
        
        role = Role()
        
        role.name = j_str.pop( "name", role.name )
        role.features = j_str.pop( "features", role.features )
        
        return role
