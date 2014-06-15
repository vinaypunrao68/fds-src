#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import socket
import struct
import time
import logging

from FDS_ProtocolInterface.ttypes import *
from fds_service.ttypes import *
from fds_service.constants import *
from fds_service import SMSvc
from fds_service import DMSvc
from fds_service import PlatNetSvc

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
    svc_type_map = {
        'sm' : [FDSP_MgrIdType.FDSP_STOR_MGR,       SMSvc],
        'dm' : [FDSP_MgrIdType.FDSP_DATA_MGR,       DMSvc],
        'am' : [FDSP_MgrIdType.FDSP_STOR_HVISOR,    PlatNetSvc],
        'om' : [FDSP_MgrIdType.FDSP_ORCH_MGR,       None]
    }

    def __init__(self):
        self.svc_cache = {}
        self.domain_nodes = None

    def client(self, nodeid, svc):
        """
        Returns client handle from cache.  If not found, creates a new client
        based on the ip,port from domain nodes map
        """
        k = self.svc_key(nodeid, svc)
        try:
            return self.svc_cache[k]
        except KeyError:
            # Client isn't in cache.  Lookup ip port
            (ip, port) = self.get_ip_port(nodeid, svc)
            # Create client connection and update cache
            self.svc_cache[k] = SvcHandle(ip, port, self.svc_type_map[svc][1]).client
            return self.svc_cache[k]

    def refresh(self):
        """
        Clears svc cache
        Refetches domain map
        """
        try:
            self.svc_cache.clear()
            # TODO(Rao): Get this info from OM
            self.domain_nodes = SvcHandle('127.0.0.1', 7010, SMSvc).client.getDomainNodes(None)
        except:
            pass

    def list(self):
        """
        Returns service list in format [nodeid:svctype, ip, port]
        """
        l = []
        for n in self.domain_nodes.dom_nodes:
            nodeid = n.node_base_uuid.svc_uuid
            for s in n.node_svc_list:
                svc = self.to_svc_type_str(s.svc_type)
                node_svc = '{}:{}'.format(n.node_base_uuid.svc_uuid, svc)
                l.append([node_svc, n.node_addr, s.svc_port])
        return l

    def get_ip_port(self, nodeid, svc):
        svc_type_enum = self.svc_type_map[svc][0]
        node_elm = next(x for x in self.domain_nodes.dom_nodes if x.node_base_uuid.svc_uuid == nodeid) 
        svc_elm = next(x for x in node_elm.node_svc_list if x.svc_type == svc_type_enum) 
        return (node_elm.node_addr, svc_elm.svc_port)

    def svc_key(self, nodeid, svc):
        return '{}{}'.format(nodeid, svc)

    def to_svc_type_str(self, svc_type_enum):
        """
        Converts service enum to type string.  One of [sm, dm, am, om, etc]
        """
        for k,v in self.svc_type_map.items():
            if v[0] == svc_type_enum:
                return k
        return None
