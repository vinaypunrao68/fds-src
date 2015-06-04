from model.fds_id import FdsId

class Domain(object):
    '''
    Created on Apr 28, 2015
    
    @author: nate
    '''

    def __init__(self, an_id=FdsId(), site="local"):
        self.id = an_id
        self.site = site
        
    @property
    def id(self):
        return self.__id
    
    @id.setter
    def id(self, an_id):
        
        if not isinstance(an_id, FdsId):
            raise TypeError()
            return
        
        self.__id = an_id
        
    @property
    def site(self):
        return self.__site
    
    @site.setter
    def site(self, site):
        self.__site = site