import time
import math

class DateRange(object):
    '''
    Created on Jun 29, 2015

    generic date range object
    
    The times are all in SECONDS
    
    @author: nate
    '''
    
    def __init__(self, start=0, end=(math.floor(time.time())) ):
        
        # so the checks work out...
        self.__start = start
        self.__end = end
        
        
    @property
    def start(self):
        return self.__start
    
    @start.setter
    def start(self, start):
        
        if start > self.end:
            start = self.end-1
        
        self.__start = start
        
    @property
    def end(self):
        return self.__end
    
    @end.setter
    def end(self, end):
        
        if end < self.start:
            end = self.start+1
        
        self.__end = end