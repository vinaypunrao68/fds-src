from model.base_model import BaseModel

class Domain(BaseModel):
    '''
    Created on Apr 28, 2015
    
    @author: nate
    '''

    def __init__(self, an_id=-1, name=None, site="local", state="UNKNOWN"):
        BaseModel.__init__(self, an_id, name)
        self.site = site
        self.state=state
        
    @property
    def site(self):
        return self.__site
    
    @site.setter
    def site(self, site):
        self.__site = site
        
    @property
    def state(self):
        return self.__state
    
    @state.setter
    def state(self, state):
        self.__state = state
