package com.formationds.sc;

import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.platform.svclayer.SvcServer;
import com.formationds.protocol.commonConstants;
import com.formationds.protocol.svc.PlatNetSvc;
import org.apache.thrift.TMultiplexedProcessor;
import org.apache.thrift.server.TNonblockingServer;
import org.apache.thrift.transport.TNonblockingServerSocket;
import org.apache.thrift.transport.TTransportException;


public class PlatNetSvcHost implements AutoCloseable {

    private PlatNetSvc.Iface impl;
    private int port;
    private SvcServer<PlatNetSvc.Iface> server;
    private Thread serviceThread;

    public PlatNetSvcHost(PlatNetSvc.Iface impl, int port) {
        this.impl = impl;
        this.port = port;
    }

    public synchronized void open() throws TTransportException {
        if(server != null)
            return;

        TNonblockingServerSocket socket = new TNonblockingServerSocket(port);
        PlatNetSvc.Processor<PlatNetSvc.Iface> processor = new PlatNetSvc.Processor<>(impl);
        TNonblockingServer.Args args;
        if (FdsFeatureToggles.THRIFT_MULTIPLEXED_SERVICES.isActive()) {
            // Multiplexed
            // If the given implementation is a subclass of PlatNetSvc.Iface,
            // this is just not going to work. The Thrift service name will
            // be wrong.
            TMultiplexedProcessor mp = new TMultiplexedProcessor();
            mp.registerProcessor(commonConstants.PLATNET_SERVICE_NAME, processor);
            args = new TNonblockingServer.Args(socket).processor(mp);
        } else {
            // Non-multiplexed
            args = new TNonblockingServer.Args(socket).processor(processor);
        }

        TNonblockingServer server = new TNonblockingServer(args);
        serviceThread = new Thread(server::serve);
        serviceThread.setName("PlatNetSvc server on port " + port);
        serviceThread.start();
    }

    @Override
    public synchronized void close() throws Exception {
        if(server != null) {
            server.stop();
            serviceThread.join();
            server = null;
            serviceThread = null;
        };
    }
}
