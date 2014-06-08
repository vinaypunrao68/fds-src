#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import socket
import struct
import time
import logging

from FDS_ProtocolInterface.ttypes import *
from FDS_ProtocolInterface.constants import *
from FDS_ProtocolInterface import FDSP_ConfigPathReq
from FDS_ProtocolInterface import FDSP_DataPathReq
from FDS_ProtocolInterface import FDSP_Service

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

from FdsException import *

log = logging.getLogger(__name__)

class FdsService(object):
    def __init__(self, fdsp_svc_info):
        """
        TODO(Rao): currently serive_cfg is same as node_cfg (node config)
        """
        self.service_cfg = fdsp_svc_info
        self.rpc_endpoint = None

    @staticmethod
    def connect_rpc_endpoint(ip_int, port, thrift_svc):
        ip_str = socket.inet_ntoa(struct.pack("!I", ip_int))
        sock = TSocket.TSocket(ip_str, port)
        #transport = TTransport.TBufferedTransport(sock)
        transport = TTransport.TFramedTransport(sock)
        protocol = TBinaryProtocol.TBinaryProtocol(transport)
        transport.open()

        # First establish a session
        session_if = FDSP_Service.Client(protocol)
        session_req = FDSP_MsgHdrType()

        session_req.src_node_name = sock.handle.getpeername()[0]
        session_req.src_port = sock.handle.getpeername()[1]
        session_info = session_if.EstablishSession(session_req)

        # Get the client endpoint
        client = getattr(thrift_svc, 'Client')(protocol)
        return client

    def get_rpc_endpoint(self):
        """
        @return thrift rpc endpoint
        """
        return self.rpc_endpoint

    def monitor_log_file(self, reg_ex, wait_time):
        pass


class SMService(FdsService):
    def __init__(self, fdsp_svc_info):
        super(SMService, self).__init__(fdsp_svc_info)
        self.rpc_endpoint = FdsService.connect_rpc_endpoint(fdsp_svc_info.ip_lo_addr,
                                                           fdsp_svc_info.data_port,
                                                           FDSP_DataPathReq)

    def wait_for_healthy_state(self, timeout_s=None):
        """
        Waits until sm service is fully healthy.
        For now we will monitor the log file.  Ideally we will thrift endpoint
        to do the monitoring.
        """
        log.info("Waiting for SM healthy state...")
        sleep_time = 10
        # polling loop
        while True:
            stats = self.rpc_endpoint.GetTokenMigrationStats(FDSP_MsgHdrType())
            if stats.completed != 0 and stats.inflight == 0 and stats.pending == 0:
                log.info('SM is healthy.  completed tokens:{}'.format(stats.completed))
                break
            time.sleep(sleep_time)
            if timeout_s:
                timeout_s -= sleep_time
                if timeout_s <= 0:
                    raise TimeoutException()


class DMService(FdsService):
    def __init__(self, fdsp_svc_info):
        super(DMService, self).__init__(fdsp_svc_info)
        pass

class AMService(FdsService):
    def __init__(self, fdsp_svc_info):
        super(AMService, self).__init__(fdsp_svc_info)

class OMService(FdsService):
    def __init__(self, fdsp_svc_info):
        super(OMService, self).__init__(fdsp_svc_info)
        self.rpc_endpoint = FdsService.connect_rpc_endpoint(fdsp_svc_info.ip_lo_addr,
                                                           fdsp_svc_info.control_port,
                                                           FDSP_ConfigPathReq)
