from base_cli_test import BaseCliTest
import mock_functions
from mock import patch


class TestUsers( BaseCliTest):
    '''
    Created on May 21, 2015
    
    @author: nate
    '''
    
    @patch( "services.users_service.UsersService.list_users", side_effect=mock_functions.listUsers)
    def test_list_users(self, mockList):
        '''
        Test that list users is called correctly
        '''
        args = ["user", "list"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockList.call_count == 1
        
    @patch( "getpass.getpass", side_effect=mock_functions.passwordGetter)
    @patch( "services.users_service.UsersService.create_user", side_effect=mock_functions.createUser )
    @patch( "services.users_service.UsersService.list_users", side_effect=mock_functions.listUsers)
    def test_create_user(self, mockList, mockCreate, mockPass):
        '''
        Test that the creation service is called correctly
        '''
        
        args = ["user", "create"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockCreate.call_count == 0
        assert mockList.call_count == 0
        
        args.append( "-username=joe")
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockPass.call_count == 1
        assert mockCreate.call_count == 1
        assert mockList.call_count == 1
        
        user = mockCreate.call_args[0][0]
        
        assert user.name == "joe"
        
    @patch( "services.users_service.UsersService.change_password", side_effect=mock_functions.changePassword )
    @patch( "getpass.getpass", side_effect=mock_functions.passwordGetter)
    def test_change_password(self, mockPass, mockChange):
        '''
        test that the change password function is called correctly
        '''
        
        self.auth.remove_feature( "USER_MGMT" )
        
        args = ["user", "change_password"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockPass.call_count == 1
        assert mockChange.call_count == 1
        
        user_id = mockChange.call_args[0][0]
        assert user_id == 1
        
        self.auth.add_feature("USER_MGMT")
        
        args = ["user", "change_password", "-user_id=3"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockPass.call_count == 2
        assert mockChange.call_count == 2
        
        user_id = mockChange.call_args[0][0]
        assert user_id == "3"
        
    @patch( "services.users_service.UsersService.reissue_user_token", side_effect=mock_functions.reissueToken)
    def test_reissue_token(self, mockReissue):
        '''
        Test that we request the token reissue correctly
        '''
        
        #no user ID
        args = ["user", "reissue_token"]
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockReissue.call_count == 0
        
        args.append( "-user_id=4" )
        self.callMessageFormatter(args)
        self.cli.run(args)
        
        assert mockReissue.call_count == 1
        
        user_id = mockReissue.call_args[0][0]
        assert user_id == "4"
