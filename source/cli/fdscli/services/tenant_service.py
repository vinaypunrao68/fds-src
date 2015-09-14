from .abstract_service import AbstractService

from utils.converters.admin.tenant_converter import TenantConverter
from utils.converters.admin.user_converter import UserConverter
from model.fds_error import FdsError

class TenantService( AbstractService):
    '''
    Created on Apr 23, 2015
    
    @author: nate
    '''
    
    def __init__(self, session):
        AbstractService.__init__(self, session)

    def list_tenants(self):
        '''
        Get a list of all tenants in the system
        '''
        
        url = "{}{}".format( self.get_url_preamble(), "/tenants")
        response = self.rest_helper.get( self.session, url )
        
        if isinstance(response, FdsError):
            return response
        
        tenants = []
        
        for j_tenant in response:
            tenant = TenantConverter.build_tenant_from_json(j_tenant)
            tenants.append(tenant)
            
        return tenants
    
    def list_users_for_tenant(self, tenant_id):
        '''
        get a list of users that are part of this tenancy
        '''
        
        url = "{}{}{}".format( self.get_url_preamble(), "/users/tenant/", tenant_id )
        response = self.rest_helper.get( self.session, url )
        
        if isinstance(response, FdsError):
            return response
        
        users = []
        
        for j_user in response:
            user = UserConverter.build_user_from_json(j_user)
            users.append(user)
            
        return users
    
    def create_tenant(self, tenant):
        '''
        Create a new tenancy in the system
        '''
        
        url = "{}{}".format( self.get_url_preamble(), "/tenants" )
        data = TenantConverter.to_json(tenant)
        j_tenant = self.rest_helper.post( self.session, url, data )
        
        if isinstance(j_tenant, FdsError):
            return j_tenant
        
        j_tenant = TenantConverter.build_tenant_from_json(j_tenant)
        return j_tenant
    
    def assign_user_to_tenant(self, tenant_id, user_id):
        '''
        assign a user to a tenancy
        '''
        
        url = "{}{}{}{}{}".format( self.get_url_preamble(), "/tenants/", tenant_id, "/", user_id)
        response = self.rest_helper.post( self.session, url )
        
        if isinstance(response, FdsError):
            return response
        
        return response
    
    def remove_user_from_tenant(self, tenant_id, user_id):
        '''
        remove the specified user from the specified tenancy
        '''
        
        url = "{}{}{}{}{}".format( self.get_url_preamble(), "/tenants/", tenant_id, "/", user_id )
        return self.rest_helper.delete( self.session, url )
        
