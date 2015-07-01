
class FdsError(object):
    '''
    Created on Jun 30, 2015
    
    @author: nate
    '''
    
    def __init__(self, error=None, message=None):
        self.error = error
        self.message = message
        
    @property
    def error(self):
        return self.__error
    
    @error.setter
    def error(self, error):
        self.__error = error
        
    @property
    def message(self):
        return self.__message
    
    @message.setter
    def message(self, message):
        self.__message = message
        
    