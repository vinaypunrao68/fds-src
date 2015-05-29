
class Domain(object):
    '''
    Created on Apr 28, 2015
    
    @author: nate
    '''

    def __init__(self, name="FDS", an_id=-1, site="local"):
        self.__name = name
        self.__id = an_id
        self.__site = site
        
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
        
    @property
    def site(self):
        return self.__site
    
    @site.setter
    def site(self, site):
        self.__site = site