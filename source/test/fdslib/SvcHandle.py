#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import socket
import struct
import time
import logging
import os
import sys

sys.path.append(os.path.join(os.getcwd(), 'fdslib'))
sys.path.append(os.path.join(os.getcwd(), 'fdslib/pyfdsp'))


from svc_types.ttypes import *
from common.ttypes import *
from FDS_ProtocolInterface.ttypes import *
from svc_api.ttypes import *
from svc_api.constants import *
from svc_api import PlatNetSvc
from sm_api import SMSvc
from dm_api import DMSvc
from config_api import ConfigurationService
from om_api import OMSvc
from svc_api import PlatNetSvc

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

class SvcMap(object):
    # Mapping of service types (as strings) to thrift communication clients
    svc_type_map = {
        'sm' : [FDSP_MgrIdType.FDSP_STOR_MGR,       SMSvc],
        'dm' : [FDSP_MgrIdType.FDSP_DATA_MGR,       DMSvc],
        'am' : [FDSP_MgrIdType.FDSP_ACCESS_MGR,    PlatNetSvc],
        #TODO(prem): add config service with different name
        #'om' : [FDSP_MgrIdType.FDSP_ORCH_MGR,       ConfigurationService],
        'om' : [FDSP_MgrIdType.FDSP_ORCH_MGR,       OMSvc],
        'pm' : [FDSP_MgrIdType.FDSP_PLATFORM,       PlatNetSvc]
    }

    def __init__(self, ip, port):
        """
        @param ip: IP to fetch the domain map from.  Ideally this is OM ip
        @param port: port
        """
        self.svc_cache = {}
        self.svc_tbl = {}
        self.services = None
        self.ip = ip
        self.port = int(port)
        self.refresh()

    def svcHandle(self, svc_uuid):
        try:
            return self.svc_cache[svc_uuid]
        except KeyError:
            # Create client connection and update cache
            type_name = self.toSvcTypeStr(self.svc_tbl[svc_uuid].svc_type)
            self.svc_cache[svc_uuid] = SvcHandle(ip = self.svc_tbl[svc_uuid].ip,
                                                 port = self.svc_tbl[svc_uuid].svc_port,
                                                 svc_type = self.svc_type_map[type_name][1])
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

    def client(self, svcid):
        """
        Returns thrift client for the service
        """
        return self.clientBySvcId(svcid)

    def clientBySvcId(self, svc_uuid):
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
        return SvcHandle(self.ip, 9090, ConfigurationService).client
        #return self.svcHandle(None, 'om').client

    def omSvc(self):
        """
        Returns OM platform service handle
        """
        # TODO(Rao): Impl
        return SvcHandle(self.ip, 7004, OMSvc).client

    def refresh(self):
        """
        Clears svc cache
        Refetches domain map
        Throws an exception if refresh fails.
        """
        self.svc_cache.clear()
        self.svc_tbl.clear()
        self.services = self.omSvc().getSvcMap(None)
        for s in self.services:
            svc_uuid = s.svc_id.svc_uuid.svc_uuid;
            self.svc_tbl[svc_uuid] = s;

    def __svc_status(self, val):
        if val == 0:
            return 'Invalid'
        elif val == 1:
            return 'Active'
        elif val == 2:
            return 'Inactive'
        else:
            return 'Error'


    def list(self):
        """
        Returns service list in format [nodeid:svctype, ip, port]
        """
        l = []
        for s in self.services:
            svc = self.toSvcTypeStr(s.svc_type)
            l.append([s.svc_id.svc_uuid.svc_uuid, svc, s.incarnationNo, s.ip, s.svc_port, self.__svc_status(s.svc_status)])
        return l

    def svcUuids(self, svc):
        """
        @svc: svc type string.  One of ['sm', 'dm', etc.]
        @return list of svc uuids that match svc type in the domain
        """
        l = []
        for k,v in self.svc_tbl.items():
            if v.name == svc:
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
