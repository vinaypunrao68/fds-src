package com.formationds.nfs;

import com.formationds.commons.util.thread.ThreadFactories;
import com.formationds.util.Configuration;
import com.formationds.util.DebugWebapp;
import com.formationds.util.ServerPortFinder;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.RealAsyncAm;
import com.formationds.xdi.XdiClientFactory;
import com.formationds.xdi.XdiConfigurationApi;
import org.apache.log4j.Logger;
import org.dcache.nfs.ExportFile;
import org.dcache.nfs.v3.MountServer;
import org.dcache.nfs.v4.DeviceManager;
import org.dcache.nfs.v4.MDSOperationFactory;
import org.dcache.nfs.v4.NFSServerV41;
import org.dcache.nfs.vfs.VirtualFileSystem;
import org.dcache.xdr.OncRpcProgram;
import org.dcache.xdr.OncRpcSvc;
import org.dcache.xdr.OncRpcSvcBuilder;

import java.io.IOException;
import java.util.concurrent.SynchronousQueue;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;


public class NfsServer {
    private static final Logger LOG = Logger.getLogger(NfsServer.class);

    public static void main(String[] args) throws Exception {
        if (args.length != 2) {
            System.out.println("Usasge: NfsServer omhost amhost");
            System.exit(-1);
        }
        String omHost = args[0];
        String amHost = args[1];
        Configuration platformConfig = new Configuration("NFS", new String[]{"--console"});
        NfsConfiguration nfsConfiguration = platformConfig.getNfsConfig();
        XdiClientFactory clientFactory = new XdiClientFactory();
        XdiConfigurationApi config = new XdiConfigurationApi(clientFactory.remoteOmService(omHost, 9090));
        config.startCacheUpdaterThread(1000);

        AsyncAm asyncAm = new RealAsyncAm(amHost, 8899, new ServerPortFinder().findPort("NFS", 10000));
        asyncAm.start();
        new NfsServer().start(nfsConfiguration, config, asyncAm, 2049);
        System.in.read();
    }

    public void start(NfsConfiguration nfsConfiguration, XdiConfigurationApi config, AsyncAm asyncAm, int serverPort) throws IOException {
        Counters counters = new Counters();
        if (nfsConfiguration.activateStats()) {
            Thread t = new Thread(() -> new DebugWebapp().start(5555, asyncAm, config, counters));
            t.setName("NFS statistics webapp");
            t.start();
        }
        LOG.info("Starting NFS server - " + nfsConfiguration.toString());
        DynamicExports dynamicExports = new DynamicExports(config);
        dynamicExports.start();
        ExportFile exportFile = dynamicExports.exportFile();

//        VirtualFileSystem vfs = new MemoryVirtualFileSystem();
//        VirtualFileSystem vfs = new AmVfs(asyncAm, config, dynamicExports);
        VirtualFileSystem vfs = new BlockyVfs(asyncAm, dynamicExports, counters);

        // create the RPC service which will handle NFS requests
        ThreadFactory factory = ThreadFactories.newThreadFactory("nfs-rpcsvc", true);
        ThreadPoolExecutor executor = new ThreadPoolExecutor(nfsConfiguration.getThreadPoolSize(),
                nfsConfiguration.getThreadPoolSize(),
                10, TimeUnit.MINUTES,
                //new LinkedBlockingQueue<>(nfsConfiguration.getWorkQueueSize()),
                new SynchronousQueue<>(),
                factory,
                //new BlockingRejectedExecutionHandler(nfsConfiguration.getIncomingRequestTimeoutSeconds(), TimeUnit.SECONDS));
                new ThreadPoolExecutor.CallerRunsPolicy());
        OncRpcSvc nfsSvc = new OncRpcSvcBuilder()
                .withPort(serverPort)
                .withTCP()
                .withAutoPublish()
                .withWorkerThreadIoStrategy()
                .withWorkerThreadExecutionService(executor)
                .build();

        // create NFS v4.1 server
        NFSServerV41 nfs4 = new NFSServerV41(
                new MDSOperationFactory(),
                new DeviceManager(),
                vfs,
                exportFile);

        // create NFS v3 and mountd servers
        CustomNfsV3Server nfs3 = new CustomNfsV3Server(exportFile, vfs);
        MountServer mountd = new MountServer(exportFile, vfs);

        // register NFS servers at portmap service
        nfsSvc.register(new OncRpcProgram(100003, 4), nfs4);
        nfsSvc.register(new OncRpcProgram(100003, 3), nfs3);
        nfsSvc.register(new OncRpcProgram(100005, 3), mountd);

        // start RPC service
        nfsSvc.start();
    }


}

// Stat
// Base IO
// mkdir
// Permissions
// Ownership
// Time
// FileId mapping
// Other file types (named pipes, fifos, block descriptors, char descriptors, etc)
