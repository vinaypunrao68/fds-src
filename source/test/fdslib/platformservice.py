#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import threading
import time

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.server import TServer, TNonblockingServer

from common.ttypes import *
from FDS_ProtocolInterface.ttypes import *
from svc_types.ttypes import *
from svc_types.constants import *

from SvcHandle import *
import FdspUtils
import atexit

log = logging.getLogger(__name__)

class WaitedCallback:
    def __init__(self):
        self.header = None
        self.payload = None
        self.cv = threading.Condition()

    def wait(self, timeout=None):
        self.cv.acquire()
        if self.payload == None:
            self.cv.wait(timeout)
        self.cv.release()
        return self.payload != None

    def __call__(self, header, payload):
        self.cv.acquire()
        self.header = header
        self.payload = payload
        self.cv.notifyAll()
        self.cv.release()

class PlatSvc(object):
    def __init__(self, basePort, omPlatIp='127.0.0.1', omPlatPort=7000, svcMap=None):
        self.reqLock = threading.Lock()
        self.nextReqId = 0
        self.reqCbs = {}
        # TODO(Rao): Get my svc uuid
        self.mySvcUuid = 64
        self.basePort = basePort
        # get the service map
        if not svcMap:
            self.svcMap = SvcMap(omPlatIp, omPlatPort)
        else:
            self.svcMap = svcMap
        # start the server
        self.serverSock = None
        self.server = None
        self.serverThread = None
        self.startServer()
        time.sleep(1)

        # register with OM
        try :
            self.registerService(basePort, omPlatIp, omPlatPort)
        except Exception, e:
            print e
            print 'failed to register with om'
            self.stop()

    def __del__(self):
        # This should stop the Thrift server when the main thread is destructed so that the daemon
        # process doesn't throw errors before it's killed.
        if hasattr(self, 'smClient'):
            serv = self.smClient()
            serv.stop_serv()

    def stop(self):
        try:
            if self.serverSock:
                self.serverSock.close()
        except:
            pass
        
    def registerService(self, basePort, omPlatIp, omPlatPort):
        """
        Run the registration protocol to register with the domain
        """
        svcinfo = SvcInfo();
        svcinfo.svc_id = SvcID(SvcUuid(self.mySvcUuid), 'Formation Console Service');
        svcinfo.svc_port = basePort;
        svcinfo.svc_type = FDSP_MgrIdType.FDSP_CONSOLE;
        svcinfo.svc_status = ServiceStatus.SVC_STATUS_ACTIVE;
        svcinfo.svc_auto_name = 'Formation Console Service';
        # TODO(Rao): Get IP
        svcinfo.ip = '127.0.0.1'
        svcinfo.incarnationNo = 0;
        svcinfo.name = 'Formation Console';
        svcinfo.props = {};
        # send registration message to OM
        self.svcMap.omSvc().registerService(svcinfo);

    def startServer(self):
        processor = PlatNetSvc.Processor(self)
        self.serverSock = TSocket.TServerSocket(port=self.basePort)
        tfactory = TTransport.TFramedTransportFactory()
        pfactory = TBinaryProtocol.TBinaryProtocolFactory()
        self.server = TServer.TSimpleServer(processor, self.serverSock, tfactory, pfactory)
        #self.server = TNonblockingServer.TNonblockingServer(processor, self.serverSock, tfactory, pfactory)
        self.serverThread = threading.Thread(target=self.serve)
        # TODO(Rao): This shouldn't be deamonized.  Without daemonizing running into
        self.serverThread.setDaemon(True)
        log.info("Starting server on {}".format(self.basePort));
        self.serverThread.start()

    def serve(self):
        log.info('About to serve')
        self.server.serve()
        log.info("Exiting server")

    def stop_server(self):
        self.stop()

    def sendAsyncReqToSvc(self, node, svc, msg, cb=None, timeout=None):
        targetUuid = self.svcMap.svc_uuid(node, svc)
        return self.sendAsyncSvcReq(targetUuid, msg, cb, timeout)

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
            log.info('mySvcUuid: {}, targetSvcUuid: {}, reqId: {}'.format(self.mySvcUuid, targetUuid, reqId))
            header = FdspUtils.newAsyncHeader(mySvcUuid=self.mySvcUuid,
                                              targetSvcUuid=targetUuid,
                                              reqId=reqId,
                                              msg=msg)
            payload = FdspUtils.serializeSvcMsg(msg)
            self.svcMap.clientBySvcId(targetUuid).asyncReqt(header, payload)
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
        log.info('mySvcUuid: {}, targetSvcUuid: {}, reqId: {}, error: {} payload:{}'.format(
            asyncHdr.msg_src_uuid.svc_uuid,
            asyncHdr.msg_dst_uuid.svc_uuid,
            asyncHdr.msg_src_id,
            asyncHdr.msg_code, payload))
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
