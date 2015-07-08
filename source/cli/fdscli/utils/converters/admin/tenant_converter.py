from model.admin.tenant import Tenant
import json

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
#         if not isinstance(tenant, Tenant):
#             raise TypeError()
        
        j_tenant = dict()
        j_tenant["uid"] = tenant.id
        j_tenant["name"] = tenant.name
        
        j_tenant = json.dumps(j_tenant)
        
        return j_tenant
    
    @staticmethod
    def build_tenant_from_json(j_tenant):
        '''
        convert tenant JSON into an actual object
        '''
        
        tenant = Tenant()
        
        if not isinstance(j_tenant, dict):
            j_tenant = json.loads(j_tenant)
        
        tenant.id = j_tenant.pop( "uid", tenant.id )
        tenant.name = j_tenant.pop( "name", tenant.name )
        
        return tenant
            
            
