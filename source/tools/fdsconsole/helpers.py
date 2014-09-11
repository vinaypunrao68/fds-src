
ATTR_CLICMD='clicmd'

class AccessLevel:
    '''
    Defines different access levels for users
    '''
    USER  = 1
    ADMIN = 2
    DEBUG = 3

    @staticmethod
    def getName(level):
        level = int(level)
        if 1 == level: return 'USER'
        if 2 == level: return 'ADMIN'
        if 3 == level: return 'DEBUG'
        return None

    @staticmethod
    def getLevel(name):
        name = name.upper()
        if name == 'USER': return 1
        if name == 'ADMIN' : return 2
        if name == 'DEBUG' : return 3
        return 0

    @staticmethod
    def getLevels():
        return ['ADMIN', 'DEBUG', 'USER']

class ConfigData:
    '''
    structure to share/ store & retrieve data
    across different contexts
    '''
    def __init__(self,data):
        self.__data = data

    def set(self, key, value, namespace):
        if namespace not in self.__data:
            self.__data[namespace] = {}

        if key not in self.__data[namespace]:
            self.__data[namespace][key] = {}

        self.__data[namespace][key] = value
        return True

    def get(self, key, namespace):
        if namespace not in self.__data:
            return None

        if key not in self.__data[namespace]:
            return None

        return self.__data[namespace][key]
    
    def getSystem(self, key):
        return self.get(key, '__system__')

    def setSystem(self, key, value):
        return self.set(key, value, '__system__')

def setupHistoryFile():
    '''
    stores and retrieves the command history specific to the user
    '''
    import os
    import readline
    histfile = os.path.join(os.path.expanduser("~"), ".fdsconsole_history")
    try:
        readline.read_history_file(histfile)
    except IOError:
        pass
    import atexit
    atexit.register(readline.write_history_file, histfile)
