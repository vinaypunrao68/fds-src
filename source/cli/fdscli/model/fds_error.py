
class FdsError(object):
    '''
    Created on Jun 30, 2015
    
    @author: nate
    '''
    
    def __init__(self, error=None):
        self.__error = error
        
    @property
    def error(self):
        return self.__error
    
    @error.setter
    def error(self, error):
        self.__error = error