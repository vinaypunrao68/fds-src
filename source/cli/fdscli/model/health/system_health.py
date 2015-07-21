from model.health.health_state import HealthState

class SystemHealth(object):
    '''
    Created on Jun 29, 2015
    
    @author: nate
    '''

    def __init__(self, overall_health=HealthState.GOOD):
        self.health_records = []
        self.overall_health = overall_health
        
    @property
    def health_records(self):
        return self.__health_records
    
    @health_records.setter
    def health_records(self, health_records):
        self.__health_records = health_records
        
    @property
    def overall_health(self):
        return self.__overall_health
    
    @overall_health.setter
    def overall_health(self, overall_health):
        self.__overall_health = overall_health