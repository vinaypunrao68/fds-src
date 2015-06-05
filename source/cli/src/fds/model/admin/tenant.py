from fds.model.fds_id import FdsId

class Tenant(object):
    '''
    Created on May 18, 2015
    
    @author: nate
    '''

    def __init__(self, an_id=FdsId()):
        self.id = an_id
        
    @property
    def id(self):
        return self.__id
    
    @id.setter
    def id(self, an_id):
        
        if not isinstance(an_id, FdsId):
            raise TypeError()
        
        self.__id = an_id