import getpass
import requests
import os
from utils.fds_cli_configuration_manager import FdsCliConfigurationManager
from requests.packages import urllib3
from services.fds_auth_error import FdsAuthError

class FdsAuth():
    
    '''
    Created on Apr 8, 2015

    This class encapsulates all of the authentication process necessary to interact with the FDS REST 
    endpoints.  This class will hold onto the returned information (including header token) to be 
    used in all REST interactions as well as the allowed feature list to prevent unauthorized access
    to features
    
    @author: nate
    '''        
    
    #feature list
    VOL_MGMT = "VOL_MGMT"
    SYS_MGMT = "SYS_MGMT"
    USER_MGMT = "USER_MGMT"
    TENANT_MGMT = "TENANT_MGMT"
    
    def __init__(self, confFile=os.path.join(os.path.expanduser("~"), ".fdscli.conf")):
        
#         self.__parser = SafeConfigParser()
#         self.__parser.read( confFile )
        
        self.__token = None
        self.__user_id = -1
        self.__features = [FdsAuth.VOL_MGMT, FdsAuth.USER_MGMT]
        self.__config = FdsCliConfigurationManager(["conf_file=" + confFile])
        self.refresh()
        
    def refresh(self):
        self.__config.refresh()
        self.__hostname = self.get_from_parser( FdsCliConfigurationManager.HOSTNAME )
        self.__port = self.get_from_parser( FdsCliConfigurationManager.PORT )
        self.__username = self.get_from_parser( FdsCliConfigurationManager.USERNAME )
        self.__protocol = self.get_from_parser( FdsCliConfigurationManager.PROTOCOL )
        
    def get_from_parser(self, option):
        if self.__config.get_value(FdsCliConfigurationManager.CONNECTION, option) is not None:
            return self.__config.get_value(FdsCliConfigurationManager.CONNECTION, option)
        else:
            return None
        
    def get_protocol(self):
        if self.__protocol is None:
            return "http"
        
        return self.__protocol
        
    def get_hostname(self):
        
        if ( self.__hostname is None ):
            self.__hostname = raw_input( 'Hostname: ' )
           
        return self.__hostname
    
    def get_username(self):
        
        if ( self.__username is None ):
            self.__username = raw_input( 'Login: ' )
            
        return self.__username
    
    def get_password(self):
        ''' 
        get the password fresh each time so that it does not
        stay in memory long
        '''
        
        passwd = self.get_from_parser(FdsCliConfigurationManager.PASSWORD)
        
        if passwd is None:
            passwd = getpass.getpass( 'Password: ' )
            
        return passwd

    def get_port(self):
        
        if ( self.__port is None ):
            self.__port = raw_input( 'Port: ' )
            
        return self.__port
    
    def get_token(self):
        
        if self.__token is None:
                self.login()
        
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
    
    def logout(self):
        '''
        effectively lose the connection session
        '''
        self.__token = None
        
        #re load the login settings
        FdsCliConfigurationManager().refresh()
    
    def login(self):
    
        '''
        uses the connection parameters to try and login into the FDS system
        '''
    
        payload = { "login" : self.get_username(), "password" : self.get_password() }
    
        urllib3.disable_warnings()
        url = "{}://{}:{}/fds/config/v08/token".format( self.get_protocol(), self.get_hostname(), self.get_port())
        response = None
        
        try:
            response = requests.post( url, params=payload, verify=False )
        except Exception:
            self.refresh()
            raise FdsAuthError(message="Login URL was not found.  Your OM may be down or unreachable.", error_code=404)
        
        #error occurred so get back to the starting line
        if "message" in response or (hasattr(response, "ok") and response.ok is False):
            self.refresh()
            raise FdsAuthError()
        
        response = response.json()
        
        if ( "userId" in response ):
            self.__user_id = response["userId"]
    
        if ( "token" in response ):
            self.__token = response['token']
            
        if ( "features" in response ):
            self.__features = response["features"]
            
        return self.__token
