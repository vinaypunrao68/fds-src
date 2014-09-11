import sys

from SvcHandle import *
from fdslib import process
from fdslib import thrift_json
from tabulate import tabulate
from argh import *

from fdsconsole.context import Context
from fdsconsole.decorators import *
from FDS_ProtocolInterface.ttypes import *
from pyfdsp.snapshot.ttypes import *

log = process.setup_logger(file = 'console.log')

class ServiceMap:
    serviceMap = None
    @staticmethod
    def init(ip, port):
        #print 'ip: %s  - port: %s' % (ip,port)
        ServiceMap.serviceMap = SvcMap(ip, int(port))

    @staticmethod
    def omConfig():
        try :
            client =  ServiceMap.serviceMap.omConfig()
        except:
            raise Exception("unable to get a client connection")
        return client

    @staticmethod
    def client(*args):
        try:
            client = ServiceMap.serviceMap.client(*args)
        except:
            raise Exception("unable to get a client connection")
        return client

    @staticmethod
    def list(*args):
        return ServiceMap.serviceMap.list(*args)

    @staticmethod
    def refresh(*args):
        return ServiceMap.serviceMap.refresh(*args)
