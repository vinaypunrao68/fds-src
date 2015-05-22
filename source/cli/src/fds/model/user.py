
class User(object):
    '''
    Created on May 18, 2015
    
    @author: nate
    '''
    
    def __init__(self, username="", an_id=-1):
        self.__username = username
        self.__id = an_id
        
    @property
    def username(self):
        return self.__username
    
    @username.setter
    def username(self, username):
        self.__username = username
        
    @property
    def id(self):
        return self.__id
    
    @id.setter
    def id(self, an_id):
        self.__id = an_id
