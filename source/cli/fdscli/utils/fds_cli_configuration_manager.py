import os
from ConfigParser import SafeConfigParser

class FdsCliConfigurationManager(object):
    '''
    Created on Jun 25, 2015

    just an easy wrapper around our configuration
    
    @author: nate
    '''
    __instance = None
    __parser = None
    
    #sections
    CONNECTION = "connection"
    TOGGLES = "toggles"
    
    CMD_HISTORY = "command_history"
    HOSTNAME = "hostname"
    PASSWORD = "password"
    PORT = "port"
    USERNAME = "username"
    PROTOCOL = "protocol"

    def __new__(cls, *args, **kwargs):
        '''
        Overriding to make this a singleton
        '''
        
        if not cls.__instance:
            cls.__instance = super(FdsCliConfigurationManager, cls).__new__(cls, *args, **kwargs)
        
        return cls.__instance
    
    def __init__(self, args=[]):
        '''
        Set the config file location and then do the first read
        '''
        
        #if instance has already been created we can skip the initialization
        if self.__parser is not None:
            return
        
        self.__parser = SafeConfigParser()
        
        if len( args ) > 0:
            for arg in args:
                #this is for Luke because he needs to be happy in his life.
                if arg.startswith("conf_file="):
                    self.__config_location = args[0][args[0].index("=") + 1:]
        else:
            self.__config_location = os.path.join( os.path.expanduser("~"), ".fdscli.conf" )
            
        self.refresh()
       
        
    def refresh(self):
        ''' 
        re-read the configuration file
        '''
        self.__parser.read(self.__config_location)
        
    def get_value(self, section, option ):
        
        if self.__parser.has_section(section) and self.__parser.has_option(section, option):
            return self.__parser.get(section, option)
        else:
            return None
        
    def _set_value(self, section, option, value):
        '''
        This should NOT be called by anyone but test methods.
        '''
        self.__parser.set(section, option, value)
