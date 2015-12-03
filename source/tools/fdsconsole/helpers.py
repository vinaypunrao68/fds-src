
ATTR_CLICMD='clicmd'
KEY_SYSTEM = '__system__'
KEY_ACCESSLEVEL = '__accesslevel__'
KEY_HOST = 'host'
KEY_PORT = 'port'
KEY_USER = 'username'
KEY_PASS = 'password'
KEY_GRIDOUTPUT = 'gridoutput'
PROTECTED_KEYS = [KEY_SYSTEM, KEY_ACCESSLEVEL]

from fdslib import restendpoint
import types
import random
import boto
from boto.s3 import connection
from fdslib import platformservice
import re
import humanize

def get_simple_re(pattern, flags=re.IGNORECASE):
    if pattern == None:
        return None
    pattern=pattern.replace('.','\.')
    pattern=pattern.replace('*','.*')
    if '*' not in pattern and '?' not in pattern:
        pattern = '.*(' + pattern + ').*'
    pattern= '^' + pattern + '$'
    return re.compile(pattern, flags)

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
    def __init__(self, data):
        self.__data = data
        self.__rest = None
        self.__s3rest = None
        self.__platform = None
        self.__token = None
        self.__services = None
        self.checkDefaults()

    def checkDefaults(self):
        defaults = {
            KEY_ACCESSLEVEL: AccessLevel.USER,
            KEY_HOST : '127.0.0.1',
            KEY_PORT : 7020,
            KEY_USER : 'admin',
            KEY_PASS : 'admin',
            KEY_GRIDOUTPUT : False
        }

        for key in defaults.keys():
            if None == self.getSystem(key):
                self.setSystem(key, defaults[key])

    def getRestApi(self):
        if self.__rest == None:
            self.__rest = restendpoint.RestEndpoint(self.getHost(), 7443, user=self.getUser(), password=self.getPass(), auth=False)
            self.__token = self.__rest.login(self.getUser(), self.getPass())
        return self.__rest

    def getS3Api(self):
        self.getRestApi()
        if self.__s3rest == None:
            if not self.__token:
                print '[WARN] : unable to login as {}'.format(self.getUser())
                self.__s3rest = None
            else:
                #print token
                self.__s3rest = boto.connect_s3(aws_access_key_id=self.getUser(),
                                                aws_secret_access_key=self.__token,
                                                host=self.getHost(),
                                                port=8443,
                                                calling_format=boto.s3.connection.OrdinaryCallingFormat())
        return self.__s3rest

    def setServiceApi(self, api):
        self.__services = api

    def getServiceId(self, pattern, onlyone = True):
        if self.__services == None:
            return None
        return self.__services.getServiceId(pattern, onlyone)

    def getPlatform(self):
        if self.__platform == None:
            self.__platform = platformservice.PlatSvc(1690, self.getHost(), self.getPort())
        return self.__platform

    def init(self):
        self.checkDefaults()

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

    def getUser(self):
        return str(self.get(KEY_USER, KEY_SYSTEM))

    def getPass(self):
        return str(self.get(KEY_PASS, KEY_SYSTEM))

    def getSystem(self, key):
        return self.get(key, KEY_SYSTEM)

    def setSystem(self, key, value):
        return self.set(key, value, KEY_SYSTEM)

    def getHost(self):
        return self.get(KEY_HOST, KEY_SYSTEM)

    def setHost(self, host):
        self.setSystem(KEY_HOST, host)

    def getPort(self):
        return int(self.get(KEY_PORT, KEY_SYSTEM))

    def setPort(self, port):
        self.setSystem(KEY_PORT, port)

    def getTableFormat(self):
        val = self.get(KEY_GRIDOUTPUT, KEY_SYSTEM)
        if type(val) == types.StringType:
            val = val.lower().strip();
        if val in ['1','yes',True,'true']:
            return 'grid'
        else:
            return 'simple'

def setupHistoryFile():
    '''
    stores and retrieves the command history specific to the user
    '''
    import os
    import readline
    histfile = os.path.join(os.path.expanduser("~"), ".fdsconsole_history.{}".format(os.geteuid()))
    try:
        readline.read_history_file(histfile)
    except IOError:
        pass
    import atexit
    atexit.register(readline.write_history_file, histfile)
