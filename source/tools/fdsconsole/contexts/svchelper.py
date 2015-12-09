import sys

from SvcHandle import *
from fdslib import process
from fdslib import thrift_json
from tabulate import tabulate
from argh import *

from fdsconsole.context import Context
from fdsconsole.decorators import *
import pyfdsp

from operator import attrgetter
from operator import itemgetter

# Only do this next step if we're running from fdsconsole.py otherwise we'll suppress output for anything
# importing some of these files
if '__main__' in sys.modules and str(sys.modules['__main__']) == '<module \'__main__\' from \'./fdsconsole.py\'>':
    log = process.setup_logger('cli.log')

class ServiceMap:
    __serviceMap = None
    config = None

    @staticmethod
    def init(ip, port):
        #print 'ip: %s  - port: %s' % (ip,port)
        ServiceMap.__serviceMap = SvcMap(ip, int(port))

    @staticmethod
    def serviceMap():
        if ServiceMap.__serviceMap == None:
            ServiceMap.__serviceMap = ServiceMap.config.getPlatform().svcMap
        return ServiceMap.__serviceMap

    @staticmethod
    def omConfig():
        try:
            ServiceMap.check()
            return ServiceMap.serviceMap().omConfig()
        except Exception as e:
            print e
            raise Exception("unable to get a client connection")

    @staticmethod
    def check():
        try :
            om =  ServiceMap.serviceMap().omConfig()
            om.configurationVersion(1)
        except Exception as e:
            print e
            ServiceMap.refresh()

    @staticmethod
    def client(*args):
        try:
            ServiceMap.check()
            client = ServiceMap.serviceMap().client(*args)
        except Exception as e:
            log.exception(e)
            raise Exception("unable to get a client connection")
        return client

    @staticmethod
    def list(*args):
        try:
            ServiceMap.check()
            return ServiceMap.serviceMap().list(*args)
        except Exception as e:
            log.exception(e)
            raise Exception("unable to get a client connection")

    @staticmethod
    def refresh(*args):
        try :
            return ServiceMap.serviceMap().refresh(*args)
        except Exception as e:
            print e
            raise Exception("unable to get a client connection")
