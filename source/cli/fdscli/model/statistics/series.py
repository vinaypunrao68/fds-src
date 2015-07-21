
class Series(object):
    '''
    Created on Jun 29, 2015
    
    container for the raw data points that result from a query
    
    @author: nate
    '''
    
    def __init__(self, a_type="PUTS", context=None):
        self.context = context
        self.type = a_type
        self.datapoints = []
        
    @property
    def context(self):
        return self.__context
    
    @context.setter
    def context(self, context):
        self.__context = context
        
    @property
    def type(self):
        return self.__type
    
    @type.setter
    def type(self, a_type):
        self.__type = a_type
        
    @property
    def datapoints(self):
        return self.__datapoints
    
    @datapoints.setter
    def datapoints(self, datapoints):
        self.__datapoints = datapoints
    