
class Size(object):
    '''
    Created on May 29, 2015
    
    @author: nate
    '''

    def __init__(self, size=1, unit="GB"):
        self.size = size
        self.unit = unit
        
    def get_bytes(self):
        '''
        convert the values into BYTES
        '''
        
        sizes = ["B", "KB", "MB", "GB", "TB", "PB", "EB"]
        b = self.size
        index = 0
        
        for s in sizes:
            if s == self.unit:
                b = self.size * pow(1024, index)
                break
            index += 1
        # end for loop
        
        return b
                
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