from .abstract_service import AbstractService

from utils.converters.admin.user_converter import UserConverter
from model.fds_error import FdsError

class UsersService( AbstractService ):
    '''
    Created on Apr 23, 2015
    
    @author: nate
    '''
    
    def __init__(self, session):
        AbstractService.__init__(self, session)
        
    def list_users(self):
        ''' 
        get a list of all the users you're allowed to see
        '''
        url = "{}{}".format( self.get_url_preamble(), "/users" )        
        j_users = self.rest_helper.get( self.session, url )
        
        if isinstance(j_users, FdsError):
            return j_users
        
        users = []
        
        for j_user in j_users:
            user = UserConverter.build_user_from_json(j_user)
            users.append( user )
            
        return users
    
    def create_user(self, user ):
        '''
        create a new user
        '''
        
        url = "{}{}".format( self.get_url_preamble(), "/users" )
        data = UserConverter.to_json(user)
        user = self.rest_helper.post( self.session, url, data )
        
        if isinstance(user, FdsError):
            return user
        
        user= UserConverter.build_user_from_json(user)
        return user
    
    def change_password(self, user_id, user ):
        '''
        Change a users password
        '''
        
        url = "{}{}{}".format( self.get_url_preamble(), "/users/", user_id )
        data = UserConverter.to_json(user)
        response = self.rest_helper.put( self.session, url, data )
        
        if isinstance(response, FdsError):
            return response
        
        return response
    
    def reissue_user_token(self, user_id):
        '''
        Re-issue a users token.  This will effectively cause the effected user to have to revalidate themselves
        '''
        url = "{}{}{}".format( self.get_url_preamble(), "/token/", user_id )
        response = self.rest_helper.post( self.session, url )
        
        if isinstance(response, FdsError):
            return response
    
        return response
    
    def who_am_i(self):
        '''
        Retrieve the user associated with this session/token
        '''
        
        url = "{}{}".format( self.get_url_preamble(), "/userinfo" )
        me = self.rest_helper.get( self.session, url )
        
        if isinstance(me, FdsError):
            return me
        
        real_me = UserConverter.build_user_from_json(me)
        return real_me
