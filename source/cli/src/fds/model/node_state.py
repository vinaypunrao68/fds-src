from multiprocessing.managers import State

class NodeState():
    '''
    Created on Apr 23, 2015
    
    @author: nate
    '''

    def __init__(self):
        self.__am = True
        self.__dm = True
        self.__sm = True
        
    @property
    def am(self):
        return self.__am
    
    @am.setter
    def am(self, state):
        self.__am = state
        
    @property
    def dm(self):
        return self.__dm
    
    @dm.setter
    def dm(self, state):
        self.__dm = State
        
    @property
    def sm(self):
        return self.__sm
    
    @sm.setter
    def sm(self, state):
        self.__sm = state