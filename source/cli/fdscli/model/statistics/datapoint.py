
class Datapoint(object):
    '''
    Created on Jun 29, 2015
    
    @author: nate
    '''

    def __init__(self, x=0, y=0):
        self.x = x
        self.y = y
        
    @property
    def x(self):
        return self.__x
    
    @x.setter
    def x(self, x):
        self.__x = x
        
    @property
    def y(self):
        return self.__y
    
    @y.setter
    def y(self, y):
        self.__y = y