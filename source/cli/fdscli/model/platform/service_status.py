
class ServiceStatus(object):
    '''
    Created on May 29, 2015
    
    @author: nate
    '''
    
    def __init_(self, state="NOT_RUNNING", error_code=0, description=""):
        self.state = state
        self.error_code = error_code
        self.description = description
        
    @property
    def state(self):
        return self.__state
    
    @state.setter
    def state(self, state):
        
        if state not in ("RUNNING", "NOT_RUNNING", "DEGRADED", "LIMITED", "ERROR", "UNREACHABLE", "UNEXPECTED_EXIT", "INITIALIZING", "SHUTTING_DOWN"):
            raise TypeError()
            return
        
        self.__state = state
        
    @property
    def error_code(self):
        return self.__error_code
        
    @error_code.setter
    def error_code(self, error_code):
        self.__error_code = error_code
        
    @property
    def description(self):
        return self.__description
    
    @description.setter
    def description(self, description):
        self.__description = description