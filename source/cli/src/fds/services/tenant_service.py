from abstract_service import AbstractService
from fds.utils.tenant_converter import TenantConverter


class TenantService( AbstractService):
    '''
    Created on Apr 23, 2015
    
    @author: nate
    '''

    def list_tenants(self):
        '''
        Get a list of all tenants in the system
        '''
        
        url = "{}{}".format( self.get_url_preamble(), "/api/system/tenants")
        response = self.rest_helper.get( self.session, url )
        
        tenants = []
        
        for j_tenant in response:
            tenant = TenantConverter.build_tenant_from_json(j_tenant)
            tenants.append(tenant)
            
        return tenants
    
    def create_tenant(self, tenant_name):
        '''
        Create a new tenancy in the system
        '''
        
        url = "{}{}{}".format( self.get_url_preamble(), "/api/system/tenants/", tenant_name)
        return self.rest_helper.post( self.session, url )
    
    def assign_user_to_tenant(self, tenant_id, user_id):
        '''
        assign a user to a tenancy
        '''
        
        url = "{}{}{}{}{}".format( self.get_url_preamble(), "/api/system/tenants/", tenant_id, "/", user_id)
        return self.rest_helper.put( self.session, url )
    
    def remove_user_from_tenant(self, tenant_id, user_id):
        '''
        remove the specified user from the specified tenancy
        '''
        
        url = "{}{}{}{}{}".format( self.get_url_preamble(), "/api/system/tenants/", tenant_id, "/", user_id )
        return self.rest_helper.delete( self.session, url )
        