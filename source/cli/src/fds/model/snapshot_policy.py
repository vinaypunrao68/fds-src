from fds.model.recurrence_rule import RecurrenceRule

class SnapshotPolicy():
    '''
    Created on Apr 23, 2015
    
    @author: nate
    '''

    def __init__(self):
        self.__recurrence_rule = RecurrenceRule()
        self.__name = ""
        self.__retention = 86400
        self.__timeline_time = 0
        self.__id = -1
    
    @property
    def id(self):
        return self.__id
    
    @id.setter
    def id(self, an_id):
        self.__id = an_id
    
    @property
    def recurrence_rule(self):
        return self.__recurrence_rule
    
    @recurrence_rule.setter
    def recurrence_rule(self, rule):
        self.__recurrence_rule = rule
        
    @property
    def name(self):
        return self.__name
    
    @name.setter
    def name(self, a_name):
        self.__name = a_name
        
    @property
    def retention(self):
        return self.__retention
    
    @retention.setter
    def retention(self, a_time):
        self.__a_time
        
    @property
    def timeline_time(self):
        return self.__timeline_time
    
    @timeline_time.setter
    def timeline_time(self, a_time):
        self.__timeline_time = a_time