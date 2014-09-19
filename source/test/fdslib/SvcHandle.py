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
    def __init__(self, ip, port, svc_type):
        """
        Helper function for getting an async header
        @param ip: ip(string) of the service
        @param port: port where the service is running
        @param svc_type: Thrift service type.  One of [SMSvc, DMSvc, etc.]
        """
        sock = TSocket.TSocket(ip, port)
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

class SvcInfo(object):
    def __init__(self, node_id, svc_name, svc_enum, svc_type, ip, port):
        self.node_id = node_id
        self.svc_name = svc_name
        self.svc_enum = svc_enum
        self.svc_type = svc_type
        self.ip = ip
        self.port = port

class SvcMap(object):
    # Mapping of service types (as strings) to thrift communication clients
    svc_type_map = {
        'sm' : [FDSP_MgrIdType.FDSP_STOR_MGR,       SMSvc],
        'dm' : [FDSP_MgrIdType.FDSP_DATA_MGR,       DMSvc],
        'am' : [FDSP_MgrIdType.FDSP_STOR_HVISOR,    PlatNetSvc],
        'om' : [FDSP_MgrIdType.FDSP_ORCH_MGR,       ConfigurationService],
        'pm' : [FDSP_MgrIdType.FDSP_PLATFORM,       PlatNetSvc]
    }

    def __init__(self, ip, port):
        """
        @param ip: IP to fetch the domain map from.  Ideally this is OM ip
        @param port: port
        """
        self.svc_cache = {}
        self.svc_tbl = {}
        self.domain_nodes = None
        self.ip = ip
        self.port = int(port)
        self.refresh()

    def svcHandle(self, svc_uuid):
        try:
            return self.svc_cache[svc_uuid]
        except KeyError:
            # Create client connection and update cache
            self.svc_cache[svc_uuid] = SvcHandle(ip = self.svc_tbl[svc_uuid].ip,
                                                 port = self.svc_tbl[svc_uuid].port,
                                                 svc_type = self.svc_tbl[svc_uuid].svc_type)
            return self.svc_cache[svc_uuid]


    def svcHandles(self, svc):
        """
        @param svc: service type
        Iteraates all nodes return a list of matching svc type handles
        """
        l = []
        for k,v in self.svc_tbl.items():
            if v.svc_name == svc:
                handle = self.svcHandle(k)
                l.append(handle)
        return l

    def client(self, nodeid, svc):
        """
        Returns thrift client for the service
        """
        svc_uuid = self.svcUuid(nodeid, svc)
        return self.client(svc_uuid)

    def client(self, svc_uuid):
        """
        Returns thrift client for the service
        """
        return self.svcHandle(svc_uuid).client

    def omConfig(self):
        """
        Returns OM config handle from cache.  If not found, creates a new OM config
        based on the ip,port from domain nodes map
        """
        # TODO(Rao): Fix it
        return self.svcHandle(None, 'om').client

    def omPlat(self):
        """
        Returns OM platform service handle
        """
        # TODO(Rao): Impl
        return SvcHandle('127.0.0.1', 7000, PlatNetSvc)

    def refresh(self):
        """
        Clears svc cache
        Refetches domain map
        Throws an exception if refresh fails.
        """
        self.svc_cache.clear()
        self.svc_tbl.clear()
        self.domain_nodes = self.omPlat().client.getDomainNodes(None)
        for n in self.domain_nodes.dom_nodes:
            nodeid = n.node_base_uuid.svc_uuid
            for s in n.node_svc_list:
                svc_uuid = s.svc_id.svc_uuid.svc_uuid
                svc_name = self.toSvcTypeStr(s.svc_type)
                svc_enum = None
                svc_type = None
                if svc_name:
                    svc_enum = self.svc_type_map[svc_name][0]
                    svc_type = self.svc_type_map[svc_name][1]
                self.svc_tbl[svc_uuid] = SvcInfo(node_id = n.node_base_uuid.svc_uuid,
                                                 svc_name = svc_name,
                                                 svc_enum = svc_enum,
                                                 svc_type = svc_type,
                                                 ip = n.node_addr,
                                                 port = s.svc_port)

    def list(self):
        """
        Returns service list in format [nodeid:svctype, ip, port]
        """
        l = []
        for n in self.domain_nodes.dom_nodes:
            nodeid = n.node_base_uuid.svc_uuid
            for s in n.node_svc_list:
                svc = self.toSvcTypeStr(s.svc_type)
                l.append([n.node_base_uuid.svc_uuid, svc, n.node_addr, s.svc_port])
        return l

    def svcUuid(self, nodeid, svc):
        """
        @nodeid: Node uuid
        @svc: svc type string.  One of ['sm', 'dm', etc.]
        @return svc uuid of svc residing on nodeid or None if not found
        """
        for k,v in self.svc_tbl.items():
            if v.node_id == nodeid and v.svc_name == svc:
                return k
        return None

    def svcUuids(self, svc):
        """
        @svc: svc type string.  One of ['sm', 'dm', etc.]
        @return list of svc uuids that match svc type in the domain
        """
        l = []
        for k,v in self.svc_tbl.items():
            if v.svc_name == svc:
                l.append(k)
        return l

    def toSvcTypeStr(self, svc_type_enum):
        """
        Converts service enum to type string.  One of [sm, dm, am, om, etc]
        """
        for k,v in self.svc_type_map.items():
            if v[0] == svc_type_enum:
                return k
        return None
