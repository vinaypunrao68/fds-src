
class Statistics(object):
    '''
    Created on Jun 29, 2015

    a model object that holds the results of statistical queries
    
    @author: nate
    '''
    
    def __init__(self ):
        self.series_list = []
        self.calculated_values = []
        self.metadata = []
        
    @property
    def series_list(self):
        return self.__series_list
    
    @series_list.setter
    def series_list(self, series_list):
        self.__series_list = series_list
        
    @property
    def calculated_values(self):
        return self.__calculated_values
    
    @calculated_values.setter
    def calculated_values(self, calculated_values):
        self.__calculated_values = calculated_values
        
    @property
    def metadata(self):
        return self.__metadata
    
    @metadata.setter
    def metadata(self, metadata):
        self.__metadata = metadata
        
