import time

class Duration(object):
    '''
    Created on May 29, 2015
    
    @author: nate
    '''

    def __init__(self, time=1, unit="DAYS"):
        self.time = time
        self.unit = unit
        
    @property
    def time(self):
        return self.__time
    
    @time.setter
    def time(self, time):
        self.__time = time
        
    @property
    def unit(self):
        return self.__unit
    
    @unit.setter
    def unit(self, unit):
        
        if unit not in ("MILLISECONDS", "SECONDS", "MINUTES", "HOURS", "DAYS"):
            raise TypeError()
        
        self.__unit = unit