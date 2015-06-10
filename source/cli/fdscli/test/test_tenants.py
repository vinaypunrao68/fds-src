from base_cli_test import BaseCliTest
from mock import patch
import mock_functions

class TestTenants(BaseCliTest):
    '''
    Created on May 21, 2015
    
    @author: nate
    '''

    @patch( "services.tenant_service.TenantService.list_tenants", side_effect=mock_functions.listTenants)
    def test_list_tenants(self, mockList):
        '''
        Make sure we're calling list correctly
        '''
        
        args = ["tenant", "list"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockList.call_count == 1
    
    @patch( "services.tenant_service.TenantService.list_tenants", side_effect=mock_functions.listTenants)
    @patch( "services.tenant_service.TenantService.create_tenant", side_effect=mock_functions.createTenant)
    def test_create_tenant(self, mockCreate, mockList):
        '''
        Make sure we're calling the tenant creation correctly
        '''
        
        #no name
        args = ["tenant","create"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockCreate.call_count == 0
        assert mockList.call_count == 0
        
        args.append( "-name=Awesome" )
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockCreate.call_count == 1
        assert mockList.call_count == 1
        
        tenant = mockCreate.call_args[0][0]
        assert tenant.name == "Awesome"
        
    @patch( "services.tenant_service.TenantService.list_users_for_tenant", side_effect=mock_functions.listUsers)
    @patch( "services.tenant_service.TenantService.assign_user_to_tenant", side_effect=mock_functions.assignUser)
    def test_assign_user(self, mockAssign, mockList):
        '''
        Test that the assign function is called properly
        '''
        # no IDs
        args = ["tenant", "assign_user"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockAssign.call_count == 0
        assert mockList.call_count == 0
        
        # no user id
        args.append("-tenant_id=1")
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockAssign.call_count == 0
        assert mockList.call_count == 0
        
        args.append("-user_id=2")
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockAssign.call_count == 1
        assert mockList.call_count == 1
        
        tenant_id = mockAssign.call_args[0][0]
        user_id = mockAssign.call_args[0][1]
        
        assert tenant_id == "1"
        assert user_id == "2"
        
    @patch( "services.tenant_service.TenantService.list_users_for_tenant", side_effect=mock_functions.listUsers)
    @patch( "services.tenant_service.TenantService.remove_user_from_tenant", side_effect=mock_functions.removeUser)        
    def test_remove_user(self, mockRemove, mockList):
        '''
        Test that we are calling the remove function properly
        '''
        # no IDs
        args = ["tenant", "remove_user"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockRemove.call_count == 0
        assert mockList.call_count == 0
        
        # no user id
        args.append("-tenant_id=1")
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockRemove.call_count == 0
        assert mockList.call_count == 0
        
        args.append("-user_id=2")
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockRemove.call_count == 1
        assert mockList.call_count == 1
        
        tenant_id = mockRemove.call_args[0][0]
        user_id = mockRemove.call_args[0][1]
        
        assert tenant_id == "1"
        assert user_id == "2"        
