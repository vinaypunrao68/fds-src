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

log = process.setup_logger()

class ServiceMap:
    serviceMap = None
    @staticmethod
    def init(ip, port):
        #print 'ip: %s  - port: %s' % (ip,port)
        ServiceMap.serviceMap = SvcMap(ip, int(port))

    @staticmethod
    def omConfig():
        try :
            ServiceMap.check()
            client =  ServiceMap.serviceMap.omConfig()
        except:
            raise Exception("unable to get a client connection")
        return client
    
    @staticmethod
    def check():
        try :
            om =  ServiceMap.serviceMap.omConfig()
            om.configurationVersion(1)            
        except:
            ServiceMap.refresh()

    @staticmethod
    def client(*args):
        try:
            ServiceMap.check()
            client = ServiceMap.serviceMap.client(*args)
        except Exception as e:
            log.exception(e)
            raise Exception("unable to get a client connection")
        return client

    @staticmethod
    def list(*args):
        try:
            ServiceMap.check()
            return ServiceMap.serviceMap.list(*args)
        except Exception as e:
            log.exception(e)
            raise Exception("unable to get a client connection")

    @staticmethod
    def refresh(*args):
        try :
            return ServiceMap.serviceMap.refresh(*args)
        except:
            raise Exception("unable to get a client connection")
