from model.admin.tenant import Tenant
from utils.converters.fds_id_converter import FdsIdConverter

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
        if not isinstance(tenant, Tenant):
            raise TypeError()
        
        j_tenant = dict()
        j_tenant["id"] = FdsIdConverter.to_json(tenant.id)
        
        return j_tenant
    
    @staticmethod
    def build_tenant_from_json(j_tenant):
        '''
        convert tenant JSON into an actual object
        '''
        
        tenant = Tenant()
        
        tenant.id = FdsIdConverter.build_id_from_json(j_tenant.pop("id") )
        
        return tenant
            
            
