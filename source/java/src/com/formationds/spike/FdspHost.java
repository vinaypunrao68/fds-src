package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.*;
import com.formationds.util.Configuration;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;
import org.apache.thrift.TProcessor;
import org.apache.thrift.TProcessorFactory;
import org.apache.thrift.server.TNonblockingServer;
import org.apache.thrift.server.TServer;
import org.apache.thrift.transport.TNonblockingServerSocket;
import org.apache.thrift.transport.TTransport;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.UUID;

class DebugServer implements InvocationHandler {
    private static final Logger LOG = Logger.getLogger(DebugServer.class);

    @Override
    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
        LOG.info("Proxy for " + proxy.getClass().getName() + "Received method call for " + method.getName());
        return 42;
    }
}

public class FdspHost {
    public static void main(String[] args) throws Exception {
        FDSP_ConfigPathReq.Iface configHandler = (FDSP_ConfigPathReq.Iface) Proxy.newProxyInstance(FdspHost.class.getClassLoader(), new Class[]{FDSP_ConfigPathReq.Iface.class}, new DebugServer());
        FDSP_ConfigPathReq.Processor<FDSP_ConfigPathReq.Iface> configProcessor = new FDSP_ConfigPathReq.Processor<>(configHandler);

        new Thread(() -> {
            try {
                new FdspHost(8903, configProcessor).start();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }).start();

        FDSP_OMControlPathReq.Iface controlHandler = (FDSP_OMControlPathReq.Iface) Proxy.newProxyInstance(FdspHost.class.getClassLoader(), new Class[]{FDSP_OMControlPathReq.Iface.class}, new DebugServer());
        FDSP_OMControlPathReq.Processor<FDSP_OMControlPathReq.Iface> controlProcessor = new FDSP_OMControlPathReq.Processor<>(controlHandler);
        new FdspHost(8904, controlProcessor).start();
    }

    private int port;
    private TProcessor processor;
    private static final Logger LOG = Logger.getLogger(FdspHost.class);

    public FdspHost(int port, TProcessor processor) {
        new Configuration("foo", new String[] {"--console"});
        this.port = port;
        this.processor = processor;
    }

    public void start() throws Exception {
        TNonblockingServerSocket transport = new TNonblockingServerSocket(port);
        TServer server = new TNonblockingServer(new TNonblockingServer.Args(transport).processorFactory(new AssDonutWarden(processor)));
        server.serve();
    }

    class AssDonutWarden extends TProcessorFactory {
        private boolean hasDonutUpTheAss;
        private TProcessor realProcessor;

        AssDonutWarden(TProcessor realProcessor) {
            super(realProcessor);
            this.realProcessor = realProcessor;
            hasDonutUpTheAss = false;
        }

        @Override
        public TProcessor getProcessor(TTransport trans) {
            LOG.info("Got a request, host " + port + ", this: " + this + ", armed: " + hasDonutUpTheAss);
            if (!hasDonutUpTheAss) {
                hasDonutUpTheAss = true;
                return new FDSP_Service.Processor<FDSP_Service.Iface>(new SayHello());
            } else {
                return realProcessor;
            }
        }
    }

    class SayHello implements FDSP_Service.Iface {
        @Override
        public FDSP_SessionReqResp EstablishSession(FDSP_MsgHdrType fdsp_msg) throws TException {
            return new FDSP_SessionReqResp(0, UUID.randomUUID().toString());
        }
    }
}
