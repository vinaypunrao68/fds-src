#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import socket
import struct
import time
import logging

from fds_service.ttypes import *
from fds_service.constants import *
from fds_service import SMSvc

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

from FdsException import *

log = logging.getLogger(__name__)

class SvcHandle(object):
    def __init__(self, ip_str, port, svc_type):
        sock = TSocket.TSocket(ip_str, port)
        #transport = TTransport.TBufferedTransport(sock)
        transport = TTransport.TFramedTransport(sock)
        protocol = TBinaryProtocol.TBinaryProtocol(transport)
        transport.open()
        self.client = getattr(svc_type, 'Client')(protocol)

class SvcMap(object):
    def __init__(self):
        self.svc_map = {}

    def client(self, node, svc):
        # TODO(Rao): Change this to return from service cache
        return SvcHandle('127.0.0.1', 7010, SMSvc).client
