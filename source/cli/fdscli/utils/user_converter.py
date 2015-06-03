from model.user import User
import json

class UserConverter(object):
    '''
    Created on May 18, 2015
    
    @author: nate
    '''
    
    @staticmethod
    def to_json(user):
        '''
        Convert a user object into a JSON representation
        '''
        
        j_user = dict()
        j_user["identifier"] = user.username
        j_user["id"] = user.id
        
        j_user = json.dumps(j_user)
        
        return j_user
    
    @staticmethod
    def build_user_from_json(j_user):
        
        user = User()
        
        user.username = j_user.pop( "identifier", "" )
        user.id = j_user.pop( "id", -1 )
        
        return user
