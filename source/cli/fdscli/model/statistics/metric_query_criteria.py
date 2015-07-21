from model.statistics.query_criteria import QueryCriteria
from model.statistics.date_range import DateRange

class MetricQueryCriteria(QueryCriteria):
    '''
    Created on Jun 29, 2015

    query object that is sent to the server when gathering statistics
    
    @author: nate
    '''
    
    def __init__(self, date_range=DateRange()):
        QueryCriteria.__init__(self, date_range)
        self.metrics = []
        
    @property
    def metrics(self):
        return self.__metrics
    
    @metrics.setter
    def metrics(self, metrics):
        self.__metrics = metrics