from model.statistics.query_criteria import QueryCriteria
from model.statistics.metrics import Metric
from model.statistics.date_range import DateRange

class FirebreakQueryCriteria(QueryCriteria):
    '''
    Created on Jun 30, 2015
    
    @author: nate
    '''
    
    def __init__(self, use_size_for_value=True, date_range=DateRange(), points=1 ):
        QueryCriteria.__init__(self, date_range=date_range, points=points)
        
        self.use_size_for_value = use_size_for_value
        
        self.__metrics = [Metric.STC_SIGMA, Metric.LTC_SIGMA, Metric.STP_SIGMA, Metric.LTP_SIGMA]
        
    @property
    def use_size_for_value(self):
        return self.__size_for_value
    
    @use_size_for_value.setter
    def use_size_for_value(self, use_size_for_value):
        self.__size_for_value = use_size_for_value
        
    @property
    def metrics(self):
        return self.__metrics