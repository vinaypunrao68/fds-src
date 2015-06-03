from fds.model.tenant import Tenant
from fds.utils.user_converter import UserConverter

class TenantConverter(object):
    '''
    Created on May 18, 2015
    
    @author: nate
    '''

    @staticmethod
    def to_json(tenant):
        '''
        convert a tenant object into JSON format
        '''
        
        j_tenant = dict()
        j_tenant["name"] = tenant.name
        j_tenant["id"] = tenant.id
        
        j_users = []
        
        for user in tenant.users:
            j_user = UserConverter.to_json(user)
            j_users.append( j_user )
            
        j_tenant["users"] = j_users
        
        return j_tenant
    
    @staticmethod
    def build_tenant_from_json(j_tenant):
        '''
        convert tenant JSON into an actual object
        '''
        
        tenant = Tenant()
        
        tenant.name = j_tenant.pop( "name", "" )
        tenant.id = j_tenant.pop( "id", -1 )
        
        j_users = j_tenant.pop( "users", [])
        
        for j_user in j_users:
            user = UserConverter.build_user_from_json(j_user)
            tenant.users.append( user )
            
        return tenant
            
            