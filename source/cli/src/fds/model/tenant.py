
class Tenant(object):
    '''
    Created on May 18, 2015
    
    @author: nate
    '''

    def __init__(self, name="", an_id=-1):
        self.__name = name
        
    @property
    def name(self):
        return self.__name
    
    @name.setter
    def name(self, name):
        self.__name = name
        
    @property
    def id(self):
        return self.__id
    
    @id.setter
    def id(self, an_id):
        self.__id = an_id