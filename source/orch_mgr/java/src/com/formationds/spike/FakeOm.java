package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_Service;
import FDS_ProtocolInterface.FDSP_SessionReqResp;
import org.apache.thrift.TException;
import org.apache.thrift.server.TServer;
import org.apache.thrift.server.TThreadPoolServer;
import org.apache.thrift.transport.TServerSocket;
import org.apache.thrift.transport.TServerTransport;
import org.apache.thrift.transport.TTransportException;

public class FakeOm {
    static class FakeSessionService implements FDSP_Service.Iface {
        @Override
        public FDSP_SessionReqResp EstablishSession(FDSP_MsgHdrType fdsp_msg) throws TException {
            System.out.println(fdsp_msg);
            return new FDSP_SessionReqResp(0, "poop");
        }
    }

    public static void main(String[] args) throws Exception {
        TServerTransport channel = new TServerSocket(8904);
        FDSP_Service.Processor<FakeSessionService> processor = new FDSP_Service.Processor<>(new FakeSessionService());
        TThreadPoolServer.Args serverArgs = new TThreadPoolServer.Args(channel).processor(processor);
        TServer server = new TThreadPoolServer(serverArgs);
        server.serve();
    }
}
