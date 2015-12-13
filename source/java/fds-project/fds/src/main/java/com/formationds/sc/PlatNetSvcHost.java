package com.formationds.sc;

import com.formationds.platform.svclayer.SvcServer;
import com.formationds.protocol.svc.PlatNetSvc;
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
        TNonblockingServer.Args args = new TNonblockingServer.Args(socket)
                .processor(processor);

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
