from base_cli_test import BaseCliTest
from services.fds_auth import FdsAuth
import mock_functions
from mock import patch

class TestRoles(BaseCliTest):
    '''
    Created on Jul 22, 2015
    
    @author: nate
    '''
    @patch("services.tenant_service.TenantService.list_tenants", side_effect=mock_functions.listTenants)
    @patch("services.users_service.UsersService.list_users", side_effect=mock_functions.listUsers)
    @patch("services.node_service.NodeService.list_nodes", side_effect=mock_functions.listNodes)
    @patch("plugins.service_plugin.ServicePlugin.list_services", side_effect=mock_functions.listServices)
    def test_default_perms_services(self, mock_list, mock_nodes, mock_user, mock_tenant):
        
        auth = self.auth
        auth.set_features([FdsAuth.VOL_MGMT, FdsAuth.USER_MGMT])
        
        self.cli.loadmodules()
        
        args = ["service", "list"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mock_list.call_count == 0
        
        args = ["node", "list"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mock_nodes.call_count == 0
        
        args = ["user", "list"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mock_user.call_count == 1
        
        args = ["tenant", "list"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mock_tenant.call_count == 0
        
        #adding admin
        
        auth.add_feature(FdsAuth.SYS_MGMT)
        
        self.cli.loadmodules()
        
        args = ["service", "list"]
        
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mock_list.call_count == 1
        
        args = ["node", "list"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mock_nodes.call_count == 1   
        
        #adding tenant mgmt
        auth.add_feature(FdsAuth.TENANT_MGMT)
        
        self.cli.loadmodules()
        
        args = ["tenant", "list"]
        
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mock_tenant.call_count == 1    
