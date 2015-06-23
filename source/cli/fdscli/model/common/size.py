
class Size(object):
    '''
    Created on May 29, 2015
    
    @author: nate
    '''

    def __init__(self, size=1, unit="GB"):
        self.size = size
        self.unit = unit
        
    @property
    def size(self):
        return self.__size
    
    @size.setter
    def size(self, size):
        self.__size = size
        
    @property
    def unit(self):
        return self.__unit
    
    @unit.setter
    def unit(self, unit):
        
        if unit not in ("B", "KB", "MB", "GB", "TB", "PB", "EB"):
            raise TypeError()
        
        self.__unit = unit