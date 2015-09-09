from .abstract_plugin import AbstractPlugin
import getpass

from services.users_service import UsersService
from utils.converters.admin.user_converter import UserConverter
import json
from services.response_writer import ResponseWriter
from model.admin.user import User
from model.fds_error import FdsError
from services.fds_auth import FdsAuth

class UserPlugin(AbstractPlugin):    
    '''
    Created on Apr 13, 2015
    
    Plugin to handle the parsing of all user related tasks and the calls
    to the corresponding REST endpoints to manage users
    
    @author: nate
    '''    
    
    def __init__(self):
        AbstractPlugin.__init__(self)
        
    def detect_shortcut(self, args):
        '''
        @see: AbstractPlugin
        '''  
        
        return None        
    
    def get_user_service(self):
        
        return self.__user_service
    
    def build_parser(self, parentParser, session): 
        '''
        @see: AbstractPlugin
        '''
        
        self.session = session
        
        if not self.session.is_allowed( FdsAuth.USER_MGMT ):
            return
        
        self.__user_service = UsersService( self.session )         
        
        __who_parser = parentParser.add_parser( "whoami", help="Retrieve your user information." )
        self.add_format_arg( __who_parser )
        __who_parser.set_defaults( func=self.who_am_i, format="tabular")
        
        self.__parser = parentParser.add_parser( "user", help="Manage FDS users" )
        self.__subparser = self.__parser.add_subparsers( help="The sub-commands that are available")
        
        if self.session.is_allowed( "USER_MGMT" ):
            self.create_create_user_parser( self.__subparser )
            self.create_list_users_parser( self.__subparser )
            self.create_reissue_parser( self.__subparser )
        
        self.create_change_password_parser(self.__subparser)
    
# define the parser commands    
    
    def create_create_user_parser(self, subparser):
        '''
        Build the create user parser logic
        '''
        
        __create_parser = subparser.add_parser( "create", help="Create a new user for the system." )
        self.add_format_arg( __create_parser )
        __create_parser.add_argument( "-" + AbstractPlugin.user_name_str, help="The username for the new user", required=True )
        
        __create_parser.set_defaults( func=self.create_user, format="tabular" )
        
    def create_list_users_parser(self, subparser):
        '''
        create the parser for listing users
        '''
        
        __list_parser = subparser.add_parser( "list", help="List the users in the system." )
        self.add_format_arg( __list_parser )
        
        __list_parser.set_defaults( func=self.list_users, format="tabular" )
        
    def create_change_password_parser(self, subparser):
        '''
        create a command to allow the changing of a password
        '''
        help_str = "Change your password."
        
        user_mgmt = self.session.is_allowed("USER_MGMT")
        
        if user_mgmt is True:
            help_str = "Change a users password."
        
        __change_parser = subparser.add_parser( "change_password", help=help_str ) 
        self.add_format_arg( __change_parser )
        
        if user_mgmt is True:
            __change_parser.add_argument( "-" + AbstractPlugin.user_id_str, help="The ID of the user that you would like to set the password for", required=False, default=None)
            
        __change_parser.set_defaults( func=self.change_password, format="tabular" )
        
    
    def create_reissue_parser(self, subparser):
        '''
        create the parser for re-issuing a users token
        '''
        
        __reissue_parser = subparser.add_parser( "reissue_token", help="Re-issue a token for a specified user.  This will invalidate any sessions they are currently using." )
        __reissue_parser.add_argument( "-" + AbstractPlugin.user_id_str, help="The ID of the user whose token you would like to re-issue.", required=True )
        
        __reissue_parser.set_defaults( func=self.reissue_token ) 
    
# the command logic

    def list_users(self, args):
        '''
        List the users of the system
        '''
        users = self.get_user_service().list_users()
        
        if isinstance(users, FdsError):
            return
        
        if len(users) == 0:
            print("\nNo users found in the system.")
            return
        
        if args[AbstractPlugin.format_str] == "json":
            j_users = []
            
            for user in users:
                j_user = UserConverter.to_json(user)
                j_user = json.loads(j_user)
                j_users.append(j_user)
                
            ResponseWriter.writeJson(j_users)
        else:
            p_users = ResponseWriter.prep_users_for_table(users)
            ResponseWriter.writeTabularData(p_users)

    def create_user(self, args):
        '''
        Use the arguments to make the create user call
        '''
        user = User()
        
        user.name = args[AbstractPlugin.user_name_str]
        user.password = getpass.getpass( "Password: " )
        user.role = "USER"
        
        result = self.get_user_service().create_user( user )
        
        if isinstance( result, User):
            self.list_users(args)
            
    def who_am_i(self, args):
        '''
        gather information for the current user
        '''
        
        me = self.get_user_service().who_am_i()
        
        if not isinstance( me, FdsError ):
            return
        
        if args[AbstractPlugin.format_str] == "json":
            j_me = UserConverter.to_json(me)
            j_me = json.loads(j_me)
            ResponseWriter.writeJson(j_me)
        else:
            p_me = ResponseWriter.prep_users_for_table([me])
            ResponseWriter.writeTabularData(p_me)
        
    def change_password(self, args):
        '''
        make sense of the arguments and call the change password service call
        '''
        user = User()
        user.id = self.session.get_user_id()
        
        if args[AbstractPlugin.user_id_str] is not None:
            user.id = args[AbstractPlugin.user_id_str]
            
        user.password = getpass.getpass( "New Password: " )
        
        response = self.get_user_service().change_password(user.id, user)
        
        if not isinstance( response, FdsError ):
            print("\nPassword changed successfully.")

            
    def reissue_token(self, args):
        '''
        call the service re-issue command
        '''
        
        response = self.get_user_service().reissue_user_token(args[AbstractPlugin.user_id_str])
        
        if not isinstance( response, FdsError ):
            print("\nToken re-issued successfully.")
