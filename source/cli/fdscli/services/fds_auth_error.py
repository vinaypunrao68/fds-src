
class FdsAuthError(Exception):
    '''
    Created on Jul 22, 2015
    
    @author: nate
    '''
    
    def __init__(self, message="Authorization Failed", error_code=401):
        self.__message = message
        self.__error_code = error_code
        
    @property
    def message(self):
        return self.__message
    
    @message.setter
    def message(self, message):
        self.__message = message
        
    @property
    def error_code(self):
        return self.__error_code
    
    @error_code.setter
    def error_code(self, error_code):
        self.__error_code = error_code
