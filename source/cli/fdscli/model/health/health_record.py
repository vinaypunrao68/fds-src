from model.health.health_state import HealthState
from model.health.health_category import HealthCategory

class HealthRecord(object):
    '''
    Created on Jun 29, 2015
    
    @author: nate
    '''

    def __init__(self, state=HealthState.GOOD, category=HealthCategory.CAPACITY, message=None  ):
        self.state = state
        self.category = category
        self.message = message
        
    @property
    def state(self):
        return self.__state
    
    @state.setter
    def state(self, state):
        self.__state = state
        
    @property
    def category(self):
        return self.__category
    
    @category.setter
    def category(self, category):
        self.__category = category
        
    @property
    def message(self):
        return self.__message
    
    @message.setter
    def message(self, message):
        self.__message = message