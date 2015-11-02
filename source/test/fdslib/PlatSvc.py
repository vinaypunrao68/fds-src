#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import threading
import time

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.server import TServer

from common.ttypes import *
from svc_types.ttypes import *
from FDS_ProtocolInterface.ttypes import *
from svc_api.ttypes import *
from svc_api.constants import *

from SvcHandle import *
import FdspUtils

log = logging.getLogger(__name__)

class PlatSvc(object):
    def __init__(self, basePort, mySvcUuid=64, omPlatIp='127.0.0.1', omPlatPort=7000):
        self.reqLock = threading.Lock()
        self.nextReqId = 0
        self.reqCbs = {}
        # TODO(Rao): Get my svc uuid
        self.mySvcUuid = mySvcUuid
        self.basePort = basePort
        # get the service map
        self.svcMap = SvcMap(omPlatIp, omPlatPort)
        # start the server
        self.serverSock = None
        self.server = None
        self.serverThread = None
        self.startServer()
        time.sleep(1)
        # register with  domain
        self.registerWithDomain(basePort, omPlatIp, omPlatPort)
    
    def stop(self):
        if self.serverSock:
            self.serverSock.close()
        
    def registerWithDomain(self, basePort, omPlatIp, omPlatPort):
        """
        Run the registration protocol to register with the domain
        """
        # TODO(Rao): Get IP
        myIp = '127.0.0.1'
        # send registration message to om platform
        nodeInfoMsg = FdspUtils.newNodeInfoMsg(svcUuid=self.mySvcUuid,
                                     ip=myIp,
                                     port=basePort,
                                     svcType=FDSP_MgrIdType.FDSP_TEST_APP) 
        nodeInfoMsg.validate()
        self.svcMap.omPlat().client.notifyNodeInfo(nodeInfoMsg, False)
        # Broadcast my information to all platforms
        pmHandleList = self.svcMap.svcHandles('pm')
        for pmHandle in pmHandleList:
            pmHandle.client.notifyNodeInfo(nodeInfoMsg, False)

    def startServer(self):
        #handler = PlatNetSvcHandler()
        handler = self
        processor = PlatNetSvc.Processor(handler)
        self.serverSock = TSocket.TServerSocket(port=self.basePort)
        tfactory = TTransport.TFramedTransportFactory()
        pfactory = TBinaryProtocol.TBinaryProtocolFactory()
        self.server = TServer.TThreadedServer(processor, self.serverSock, tfactory, pfactory)
        self.serverThread = threading.Thread(target=self.serve)
        # TODO(Rao): This shouldn't be deamonized.  Without daemonizing running into
        # issues exiting
        self.serverThread.setDaemon(True)
        log.info("Starting server on {}".format(self.basePort));
        self.serverThread.start()

    def serve(self):
        self.server.serve()
        log.info("Exiting server")

    def sendAsyncSvcReq(self, nodeId, svc, msg, cb=None, timeout=None):
        targetUuid = self.svcMap.svc_uuid(nodeId, svc)
        self.sendAsyncSvcReq(targetUuid=targetUuid, msg=msg, cb=cb, timeout=timeout)

    def sendAsyncSvcReq(self, targetUuid, msg, cb=None, timeout=None):
        reqId = None
        # Incr req Id and store the response callback
        with self.reqLock:
            self.nextReqId += 1
            reqId = self.nextReqId
            # regiser the callback
            if cb:
                assert reqId not in self.reqCbs
                self.reqCbs[reqId] = cb
        # send the request
        try:
            log.debug('mySvcUuid: {}, targetSvcUuid: {}, reqId: {}'.format(self.mySvcUuid, targetUuid, reqId))
            header = FdspUtils.newAsyncHeader(mySvcUuid=self.mySvcUuid,
                                              targetSvcUuid=targetUuid,
                                              reqId=reqId,
                                              msg=msg)
            payload = FdspUtils.serializeSvcMsg(msg)
            self.svcMap.client(targetUuid).asyncReqt(header, payload)
        except Exception, e:
            # Invocation itself failed.  No need to invoke callback.
            # remove the callback and rethrow the exception
            log.exception(e)
            with self.reqLock:
                del self.reqCbs[reqId]
            raise

    ##
    # @brief Override from BaseAsyncSvc
    #
    # @param asyncHdr
    # @param payload
    #
    # @return 
    def asyncReqt(self, asyncHdr, payload):
        pass

    ##
    # @brief Override from BaseAsyncSvc.  Async response handler
    #
    # @param asyncHdr
    # @param payload
    #
    # @return 
    def asyncResp(self, asyncHdr, payload):
        log.debug('mySvcUuid: {}, targetSvcUuid: {}, reqId: {}, error: {}'.format(
            asyncHdr.msg_src_uuid.svc_uuid,
            asyncHdr.msg_dst_uuid.svc_uuid,
            asyncHdr.msg_src_id,
            asyncHdr.msg_code))
        cb = None
        # extract registered callback
        with self.reqLock:
            if asyncHdr.msg_src_id in self.reqCbs:
                cb = self.reqCbs[asyncHdr.msg_src_id]
                del self.reqCbs[asyncHdr.msg_src_id]
        # Invoke callback
        if cb:
            respMsg = None
            if asyncHdr.msg_code == 0:
                respMsg = FdspUtils.deserializeSvcMsg(asyncHdr, payload)
            cb(asyncHdr, respMsg)

    ##
    # @brief Override from PlatNetSvc
    #
    # @param nodeInfo
    # @param bcast
    #
    # @return 
    def notifyNodeInfo(self, nodeInfo, bcast):
        return []

    def svcMap(self):
        return self.svcMap
