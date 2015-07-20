from model.statistics.date_range import DateRange

class QueryCriteria(object):
    '''
    Created on Jun 29, 2015

    base class for FDS querying
    
    @author: nate
    '''
    
    def __init__(self, date_range=DateRange(), points=None):
        self.date_range = date_range
        self.contexts = []
        self.points = points
        
    @property
    def date_range(self):
        return self.__date_range
    
    @date_range.setter
    def date_range(self, date_range):
        
#         if not isinstance(date_range, DateRange):
#             raise TypeError()
        
        self.__date_range = date_range
        
    @property
    def contexts(self):
        return self.__contexts
    
    @contexts.setter
    def contexts(self, contexts):
        self.__contexts = contexts
        
    @property
    def points(self):
        return self.__points
    
    @points.setter
    def points(self, points):
        self.__points = points