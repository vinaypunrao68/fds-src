import getpass
import requests
import os
from requests.exceptions import ConnectionError
from __builtin__ import True
from utils.configuration_manager import ConfigurationManager

class FdsAuth():
    
    '''
    Created on Apr 8, 2015

    This class encapsulates all of the authentication process necessary to interact with the FDS REST 
    endpoints.  This class will hold onto the returned information (including header token) to be 
    used in all REST interactions as well as the allowed feature list to prevent unauthorized access
    to features
    
    @author: nate
    '''        
    def __init__(self, confFile=os.path.join(os.path.expanduser("~"), ".fdscli.conf")):
        
#         self.__parser = SafeConfigParser()
#         self.__parser.read( confFile )
        
        self.__config = ConfigurationManager(["conf_file=" + confFile])
        self.__token = None
        self.__user_id = -1
        self.__features = []
        self.__hostname = self.get_from_parser( ConfigurationManager.HOSTNAME )
        self.__port = self.get_from_parser( ConfigurationManager.PORT )
        self.__username = self.get_from_parser( ConfigurationManager.USERNAME )
        self.__password = self.get_from_parser( ConfigurationManager.PASSWORD )
        
    def get_from_parser(self, option):
        if self.__config.get_value(ConfigurationManager.CONNECTION, option) is not None:
            return self.__config.get_value(ConfigurationManager.CONNECTION, option)
        else:
            return None
        
    def get_hostname(self):
        
        if ( self.__hostname is None ):
            self.__hostname = raw_input( 'Hostname: ' )
           
        return self.__hostname
    
    def get_username(self):
        
        if ( self.__username is None ):
            self.__username = raw_input( 'Login: ' )
            
        return self.__username
    
    def get_password(self):
        
        if ( self.__password is None ):
            self.__password = getpass.getpass( 'Password: ' )
            
        return self.__password

    def get_port(self):
        
        if ( self.__port is None ):
            self.__port = raw_input( 'Port: ' )
            
        return self.__port
    
    def get_token(self):
        return self.__token
    
    def get_user_id(self):
        return self.__user_id
    
    def is_allowed(self, feature):
        
        for capability in self.__features:
            if ( capability == feature ):
                return True
        # end of for loop
        
        return False
    
    def is_authenticated(self):
        
        if ( self.__token is not None ):
            return True
        
        return False
    
    def login(self):
    
        '''
        uses the connection parameters to try and login into the FDS system
        '''
    
        try:
            payload = { "login" : self.get_username(), "password" : self.get_password() }
            
            #get rid of the password immediately after its used
            self.__password = None
            response = requests.post( 'http://' + self.get_hostname() + ':' + str(self.get_port()) + "/fds/config/v08/token", params=payload )
            
            if "message" in response:
                print "Login failed.\n"
                print response.pop("message")
                return
            
            response = response.json()
            
            if ( "userId" in response ):
                self.__user_id = response["userId"]
        
            if ( "token" in response ):
                self.__token = response['token']
                
            if ( "features" in response ):
                self.__features = response["features"]
                
            return self.__token
        
        except ConnectionError:
            return None