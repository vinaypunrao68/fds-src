from abstract_plugin import AbstractPlugin

'''
Created on Apr 13, 2015

This plugin configures all of the parsing for tenant specific operation and
maps those options to the appropriate calls for tenant management

@author: nate
'''
from services.tenant_service import TenantService
import json
from utils.tenant_converter import TenantConverter
from services.response_writer import ResponseWriter
from utils.user_converter import UserConverter
class TenantPlugin( AbstractPlugin):
    
    def __init__(self, session):
        AbstractPlugin.__init__(self, session)  
        self.__tenant_service = TenantService(self.session)  
    
    def detect_shortcut(self, args):
        '''
        @see: AbstractPlugin
        '''    
        return None
    
    def get_tenant_service(self):
        return self.__tenant_service
    
    def build_parser(self, parentParser, session): 
        '''
        @see: AbstractPlugin
        '''
        
        if session.is_allowed( "TENANT_MGMT" ) is False:
            return
        
        self.__parser = parentParser.add_parser( "tenant", help="Manage tenants of the system" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")

        self.create_list_parser(self.__subparser)
        self.create_list_users(self.__subparser)
        self.create_create_parser(self.__subparser)
        self.create_assign_parser(self.__subparser)
        self.create_unassign_parser(self.__subparser)
        
# parsers

    def create_list_parser(self, subparser):
        '''
        Create the parser to handle the listing command
        '''
        
        __list_parser = subparser.add_parser( "list", help="List all of the tenants in the system." )
        self.add_format_arg( __list_parser )
        
        __list_parser.set_defaults( func=self.list_tenants, format="tabular" )
        
    def create_list_users(self, subparser):
        '''
        Create the parser to list the users of a tenancy
        '''
        
        __user_parser = subparser.add_parser("list_users", help="List all of the users for a specific tenant.")
        self.add_format_arg(__user_parser)
        __user_parser.add_argument( "-" + AbstractPlugin.tenant_id_str, help="The ID of the tenant for whom you would like to list users.", required=True)
        
        __user_parser.set_defaults( func=self.list_users, format="tabular")
        
    def create_create_parser(self, subparser):
        '''
        create a parser to deal with tenant creation
        '''
        
        __create_parser = subparser.add_parser( "create", help="Create a new tenancy." )
        self.add_format_arg(__create_parser)
        
        __create_parser.add_argument( "-" + AbstractPlugin.name_str, help="The name of the new tenancy.", required=True )
        
        __create_parser.set_defaults( func=self.create_tenant, format="tabular" )
        
    def create_assign_parser(self, subparser):
        ''' 
        create a praser for assigning a user to a tenancy
        '''
        
        __assign_parser = subparser.add_parser( "assign_user", help="Assign a user to a tenancy." )
        self.add_format_arg(__assign_parser)
        
        __assign_parser.add_argument( "-" + AbstractPlugin.tenant_id_str, help="The ID of the tenancy you would like to assign the user to.", required=True)
        __assign_parser.add_argument( "-" + AbstractPlugin.user_id_str, help="The ID of the user that you would like to assign to the specified tenancy.", required=True)
        
        __assign_parser.set_defaults( func=self.assign_user, format="tabular" )
        
    def create_unassign_parser(self, subparser):
        '''
        create a parser to unassign a user from a tenancy
        '''
        
        __unassign_parser = subparser.add_parser( "remove_user", help="Remove a user from a specified tenancy." )
        self.add_format_arg(__unassign_parser)
        
        __unassign_parser.add_argument( "-" + AbstractPlugin.tenant_id_str, help="The ID of the tenant from which you would like to remove the user.", required=True)
        __unassign_parser.add_argument( "-" + AbstractPlugin.user_id_str, help="The ID of the user that you would like to remove.", required=True)
        
        __unassign_parser.set_defaults( func=self.remove_user, format="tabular" )

# logic to call correct services
    
    def list_tenants(self, args):
        ''' 
        Call and display the list tenants call
        '''
        
        tenants = self.get_tenant_service().list_tenants()
        
        if len(tenants) == 0:
            print "\nNo tenancies were found."
            return
        
        if args[AbstractPlugin.format_str] == "json":
            j_tenants = []
            for tenant in tenants:
                j_tenant = TenantConverter.to_json(tenant)
                j_tenant = json.loads(j_tenant)
                j_tenants.append(j_tenant)
                
            ResponseWriter.writeJson(j_tenants)
        else:
            data = ResponseWriter.prep_tenants_for_table(tenants)
            ResponseWriter.writeTabularData(data)
            
    def list_users(self, args):
        '''
        List the users for a specific Tenant
        '''
        
        tenants = self.get_tenant_service().list_tenants()
        
        my_tenant = None
        
        for tenant in tenants:
            if tenant.id == int(args[AbstractPlugin.tenant_id_str]):
                my_tenant = tenant
                break
        
        if args[AbstractPlugin.format_str] == "json":
            j_users = []
            
            for user in my_tenant.users:
                j_user = UserConverter.to_json(user)
                j_user = json.loads(j_user)
                j_users.append(j_user)
                
            ResponseWriter.writeJson(j_users)
        else:
            d_users = ResponseWriter.prep_users_for_table(my_tenant.users)
            ResponseWriter.writeTabularData(d_users)
            
    def create_tenant(self, args):
        '''
        Call the create tenant endpoint with the args passed in
        '''
        
        response = self.get_tenant_service().create_tenant( args[AbstractPlugin.name_str] )
        
        if response is not None:
            self.list_tenants(args)
            
    def assign_user(self, args):
        '''
        use the arguments to attach a user to a tenancy
        '''
        
        response = self.get_tenant_service().assign_user_to_tenant( args[AbstractPlugin.tenant_id_str], args[AbstractPlugin.user_id_str] )
        
        if response["status"].lower() == "ok":
            self.list_users(args)
            
    def remove_user(self, args):
        '''
        use the arguments to call teh unassign user from tenant call
        '''
        
        response = self.get_tenant_service().remove_user_from_tenant( args[AbstractPlugin.tenant_id_str], args[AbstractPlugin.user_id_str] )
        
        if response["status"].lower() == "ok":
            self.list_users(args)
        
