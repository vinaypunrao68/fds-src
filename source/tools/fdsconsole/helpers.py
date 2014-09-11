
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
