from fds.model.tenant import Tenant

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
        
        return j_tenant
    
    @staticmethod
    def build_tenant_from_json(j_tenant):
        '''
        convert tenant JSON into an actual object
        '''
        
        tenant = Tenant()
        
        tenant.name = j_tenant.pop( "name", "" )
        tenant.id = j_tenant.pop( "id", -1 )