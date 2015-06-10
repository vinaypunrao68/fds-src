
class BaseModel(object):
    '''
    Created on Jun 5, 2015
    
    @author: nate
    '''

    def __init__(self, an_id=-1, name="None"):
        self.id = an_id
        self.name = name
        
    @property
    def id(self):
        return self.__id
    
    @id.setter
    def id(self, an_id):
        self.__id = an_id
        
    @property
    def name(self):
        return self.__name
    
    @name.setter
    def name(self, name):
        self.__name = name