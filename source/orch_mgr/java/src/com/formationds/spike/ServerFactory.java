package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.*;
import org.apache.log4j.Logger;
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

public class ServerFactory {
    private static Logger LOG = Logger.getLogger(ServerFactory.class);

    private ConcurrentHashMap<TTransport, TProcessor> set;

    public ServerFactory() {
        set = new ConcurrentHashMap<>();
    }

    public void startControlPathServer(FDSP_ControlPathReq.Iface handler, int port) {
        new Thread(() -> {
            start(port, new FDSP_ControlPathReq.Processor(handler));
        }).start();
    }

    public void startOmControlPathReq(FDSP_OMControlPathReq.Iface handler, int port) {
        new Thread(() -> {
            start(port, new FDSP_OMControlPathReq.Processor(handler));
        }).start();
    }

    public void startOmControlPathResp(FDSP_OMControlPathResp.Iface handler, int port) {
        new Thread(() -> {
            start(port, new FDSP_OMControlPathResp.Processor(handler));
        }).start();
    }

    public void startConfigPathServer(FDSP_ConfigPathReq.Iface handler, int port) {
        new Thread(() -> {
            start(port, new FDSP_ConfigPathReq.Processor(handler));
        }).start();
    }

    public void startDataPathRespServer(FDSP_DataPathResp.Iface handler, int port) {
        new Thread(() -> {
            start(port, new FDSP_DataPathResp.Processor(handler));
        }).start();
    }

    private void start(int port, TProcessor processor) {
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
            LOG.info(String.format("Starting %s on port %d", processor.getClass().getName(), port));
            server.serve();
        } catch (TTransportException e) {
            throw new RuntimeException(e);
        }
    }

    static class Handshake implements FDSP_Service.Iface {
        @Override
        public FDSP_SessionReqResp EstablishSession(FDSP_MsgHdrType fdsp_msg) throws TException {
            return new FDSP_SessionReqResp(0, UUID.randomUUID().toString());
        }
    }

}
