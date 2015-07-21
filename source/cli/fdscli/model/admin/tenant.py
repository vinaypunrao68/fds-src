from model.base_model import BaseModel

class Tenant(BaseModel):
    '''
    Created on May 18, 2015
    
    @author: nate
    '''

    def __init__(self, an_id=-1, name=None):
        BaseModel.__init__(self, an_id, name)
