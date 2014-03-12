package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_Service;
import FDS_ProtocolInterface.FDSP_SessionReqResp;
import org.apache.thrift.TException;
import org.apache.thrift.TProcessor;
import org.apache.thrift.TProcessorFactory;
import org.apache.thrift.protocol.TProtocol;
import org.apache.thrift.server.TServer;
import org.apache.thrift.server.TThreadPoolServer;
import org.apache.thrift.transport.TServerSocket;
import org.apache.thrift.transport.TServerTransport;
import org.apache.thrift.transport.TTransport;
import org.apache.thrift.transport.TTransportException;

import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;

public class FdspServer<T extends TProcessor> {
    static class Handshake implements FDSP_Service.Iface {
        @Override
        public FDSP_SessionReqResp EstablishSession(FDSP_MsgHdrType fdsp_msg) throws TException {
            return new FDSP_SessionReqResp(0, UUID.randomUUID().toString());
        }
    }

    private ConcurrentHashMap<TTransport, TProcessor> set;

    public FdspServer() {
        set = new ConcurrentHashMap<>();
    }

    public void start(int port, T processor) {
        try {
            TServerTransport channel = new TServerSocket(port);
            FDSP_Service.Processor<Handshake> serviceProcessor = new FDSP_Service.Processor<Handshake>(new Handshake());

            TThreadPoolServer.Args serverArgs = new TThreadPoolServer.Args(channel)
                    .processorFactory(new TProcessorFactory(null) {
                        @Override
                        public TProcessor getProcessor(TTransport trans) {
                            return new TProcessor() {
                                @Override
                                public boolean process(TProtocol in, TProtocol out) throws TException {
                                    TProcessor p = set.putIfAbsent(trans, processor);
                                    if (p == null) {
                                        return serviceProcessor.process(in, out);
                                    } else {
                                        return p.process(in, out);
                                    }
                                }
                            };
                        }
                    });
            TServer server = new TThreadPoolServer(serverArgs);
            server.serve();
        } catch (TTransportException e) {
            throw new RuntimeException(e);
        }
    }
}
