from model.volume.recurrence_rule import RecurrenceRule
from model.base_model import BaseModel

class SnapshotPolicy(BaseModel):
    '''
    Created on Apr 23, 2015
    
    @author: nate
    '''

    def __init__(self, an_id=-1, name=None, timeline_time=0, retention_time_in_seconds=0, 
            recurrence_rule=RecurrenceRule(), preset_id=None, a_type="USER"):
        BaseModel.__init__(self, an_id, name)
        self.recurrence_rule = recurrence_rule
        self.retention_time_in_seconds = retention_time_in_seconds
        self.preset_id = preset_id
        self.timeline_time = timeline_time
        self.type = a_type
    
    @property
    def timeline_time(self):
        return self.__timeline_time
    
    @timeline_time.setter
    def timeline_time(self, a_time):
        self.__timeline_time = a_time
    
    @property
    def recurrence_rule(self):
        return self.__recurrence_rule
    
    @recurrence_rule.setter
    def recurrence_rule(self, rule):
        
        self.__recurrence_rule = rule
        
    @property
    def retention_time_in_seconds(self):
        return self.__retention
    
    @retention_time_in_seconds.setter
    def retention_time_in_seconds(self, a_time):
        self.__retention = a_time
        
    @property
    def type(self):
        return self.__type
    
    @type.setter
    def type(self, a_type):
        self.__type = a_type 
        
    @property
    def preset_id(self):
        return self.__preset_id
    
    @preset_id.setter
    def preset_id(self, preset_id):
        self.__preset_id = preset_id
