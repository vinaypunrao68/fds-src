'''
Created on Apr 3, 2015

@author: nate
'''
class Volume():
    
    __name = ''
    __priority = ''
    __type = ''
    
    def get_name(self):
        return self.__name
    
    def set_name(self, aName ):
        self.__name = aName
    
    def get_priority(self):
        return self.__priority
    
    def set_priority(self, aPriority):
        self.__priority = aPriority
    
    def get_type(self):
        return self.__type
    
    def set_type(self, aType):
        self.__type = aType
    