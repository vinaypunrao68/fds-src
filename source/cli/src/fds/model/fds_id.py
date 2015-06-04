
class FdsId(object):
    '''
    Created on May 29, 2015
    
    @author: nate
    '''
    
    def __init__(self, uuid=-1, name=""):
        self.__uuid = uuid
        self.__name = name
        
    @property
    def uuid(self):
        return self.__uuid
    
    @uuid.setter
    def uuid(self, uuid):
        self.__uuid = uuid
        
    @property
    def name(self):
        return self.__name
    
    @name.setter
    def name(self, name):
        self.__name = name
