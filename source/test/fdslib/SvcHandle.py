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
from apis import ConfigurationService

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

from FdsException import *

log = logging.getLogger(__name__)

class SvcHandle(object):
    def __init__(self, ip_str, port, svc_type):
        """
        Helper function for getting an async header
        @param ip_str: ip of the service
        @param port: port where the service is running
        @param svc_type: Thrift service type.  One of [SMSvc, DMSvc, etc.]
        """
        sock = TSocket.TSocket(ip_str, port)
        #transport = TTransport.TBufferedTransport(sock)
        transport = TTransport.TFramedTransport(sock)
        protocol = TBinaryProtocol.TBinaryProtocol(transport)
        transport.open()
        self.client = getattr(svc_type, 'Client')(protocol)

    def asyncHdr(th_type):
        """
        Helper function for getting an async header
        @param th_type: thrift type name as string
        """
        th_type_id = th_type + 'TypeId'
        # TODO(Rao): Fill in the correct values
        header = AsyncHdr(
            msg_chksum      =   123456,  # dummy file
            msg_code        =   123456, # more dummy
            msg_type_id     =   FDSPMsgTypeId._NAMES_TO_VALUES[th_type_id],
            msg_src_id      =   src_id_counter,
            msg_src_uuid    =   SvcUuid(int('cac0', 16)),
            msg_dst_uuid    =   SvcUuid(int('cac0', 16))
            )
        return header

class SvcMap(object):
    # Mapping of service types (as strings) to thrift communication clients
    svc_type_map = {
        'sm' : [FDSP_MgrIdType.FDSP_STOR_MGR,       SMSvc],
        'dm' : [FDSP_MgrIdType.FDSP_DATA_MGR,       DMSvc],
        'am' : [FDSP_MgrIdType.FDSP_STOR_HVISOR,    PlatNetSvc],
        'om' : [FDSP_MgrIdType.FDSP_ORCH_MGR,       ConfigurationService]
    }

    def __init__(self, ip, port):
        """
        @param ip: IP to fetch the domain map from.  Ideally this is OM ip
        @param port: port
        """
        self.svc_cache = {}
        self.domain_nodes = None
        self.ip = ip
        self.port = port
        self.refresh()

    def svcHandle(self, nodeid, svc):
        """
        @param nodeid: Node id.  For om, nodeid is 'om'
        Returns svc handle from cache.  If not found, creates a new service
        based on the ip,port from domain nodes map
        """
        k = self.svc_key(nodeid, svc)
        try:
            return self.svc_cache[k]
        except KeyError:
            # Client isn't in cache.  Lookup ip port
            (ip, port) = self.get_ip_port(nodeid, svc)
            # Create client connection and update cache
            self.svc_cache[k] = SvcHandle(ip, port, self.svc_type_map[svc][1])
            return self.svc_cache[k]

    def client(self, nodeid, svc):
        """
        Returns thrift client for the service
        """
        return self.svcHandle(nodeid, svc).client

    def omConfig(self):
        """
        Returns OM config handle from cache.  If not found, creates a new OM config
        based on the ip,port from domain nodes map
        """
        return self.svcHandle(None, 'om').client

    def refresh(self):
        """
        Clears svc cache
        Refetches domain map
        """
        try:
            self.svc_cache.clear()
            # TODO(Rao): Get this info from OM
            self.domain_nodes = SvcHandle(self.ip, self.port, SMSvc).client.getDomainNodes(None)
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
        """
        @nodeid: Node uuid.  If svc type is om*, then nodeid is irrelevant
        @svc: svc type string.  One of ['sm', 'dm', etc.]
        @return (ip, port) tuple for service running on nodeid
        """
        svc_type_enum = self.svc_type_map[svc][0]
        # We treat OM service specially as there is only one OM in the domain
        # We don't nodeid in this case
        if svc.startswith('om'):
            for node_elm in self.domain_nodes.dom_nodes:
                for svc_elm in node_elm.node_svc_list:
                    if svc_elm.svc_type == svc_type_enum:
                        # TODO(Rao): In the domain map exposed, there is no entry
                        # for om config.  For now hard code to om config port which
                        # is typically 8903
                        return (node_elm.node_addr, 9090)
        else:
            node_elm = next(x for x in self.domain_nodes.dom_nodes if x.node_base_uuid.svc_uuid == nodeid) 
            svc_elm = next(x for x in node_elm.node_svc_list if x.svc_type == svc_type_enum) 
            return (node_elm.node_addr, svc_elm.svc_port)

    def svc_key(self, nodeid, svc):
        """
        @nodeid: Node uuid
        @svc: svc type string.  One of ['sm', 'dm', etc.]
        @return key to store service handle in service map
        """
        return '{}{}'.format(nodeid, svc)

    def to_svc_type_str(self, svc_type_enum):
        """
        Converts service enum to type string.  One of [sm, dm, am, om, etc]
        """
        for k,v in self.svc_type_map.items():
            if v[0] == svc_type_enum:
                return k
        return None
