from fds.model.volume.recurrence_rule import RecurrenceRule
from fds.model.fds_id import FdsId

class SnapshotPolicy():
    '''
    Created on Apr 23, 2015
    
    @author: nate
    '''

    def __init__(self, an_id=FdsId(), timeline_time=0, retention_time_in_seconds=0, recurrence_rule=RecurrenceRule(), preset_id=FdsId()):
        self.recurrence_rule = recurrence_rule
        self.id = an_id
        self.retention_time_in_seconds = retention_time_in_seconds
        self.preset_id = preset_id
        self.timeline_time = timeline_time
    
    @property
    def id(self):
        return self.__id
    
    @id.setter
    def id(self, an_id):
        
        if not isinstance(an_id, FdsId):
            raise TypeError()
        
        self.__id = an_id
    
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
        
        if not isinstance(rule, RecurrenceRule):
            raise TypeError()
        
        self.__recurrence_rule = rule
        
    @property
    def retention_time_in_seconds(self):
        return self.__retention
    
    @retention_time_in_seconds.setter
    def retention_time_in_seconds(self, a_time):
        self.__a_time
        
    @property
    def preset_id(self):
        return self.__preset_id
    
    @preset_id.setter
    def preset_id(self, preset_id):
        
        if not isinstance(preset_id, FdsId):
            raise TypeError()
        
        self.__preset_id = preset_id
