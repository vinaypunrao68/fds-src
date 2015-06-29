package com.formationds.nfs;

import com.formationds.apis.AsyncXdiServiceRequest;
import com.formationds.util.Configuration;
import com.formationds.util.ServerPortFinder;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.RealAsyncAm;
import com.formationds.xdi.XdiClientFactory;
import com.formationds.xdi.XdiConfigurationApi;
import org.dcache.nfs.ExportFile;
import org.dcache.nfs.v3.MountServer;
import org.dcache.nfs.v3.NfsServerV3;
import org.dcache.nfs.v4.*;
import org.dcache.nfs.vfs.*;
import org.dcache.xdr.OncRpcProgram;
import org.dcache.xdr.OncRpcSvc;
import org.dcache.xdr.OncRpcSvcBuilder;

import java.io.File;


public class NfsServer {
    public static void main(String[] args) throws Exception {
        new Configuration("NFS", new String[] {"--console"});
        XdiClientFactory clientFactory = new XdiClientFactory();
        XdiConfigurationApi config = new XdiConfigurationApi(clientFactory.remoteOmService("localhost", 9090));
        config.startCacheUpdaterThread(1000);

        AsyncAm asyncAm = new RealAsyncAm("localhost", 8899, new ServerPortFinder().findPort("NFS", 10000));
        asyncAm.start();

        // specify file with export entries
        ExportFile exportFile = new ExportFile(new File("./exports"));
        ExportResolver resolver = new ExportResolver(exportFile);

//        VirtualFileSystem vfs = new MemoryVirtualFileSystem();
        VirtualFileSystem vfs = new AmVfs(asyncAm, config, resolver);

        // create the RPC service which will handle NFS requests
        OncRpcSvc nfsSvc = new OncRpcSvcBuilder()
                .withPort(2049)
                .withTCP()
                .withAutoPublish()
                .withWorkerThreadIoStrategy()
                .build();



        // create NFS v4.1 server
        NFSServerV41 nfs4 = new NFSServerV41(
                new MDSOperationFactory(),
                new DeviceManager(),
                vfs,
                exportFile);

        // create NFS v3 and mountd servers
        NfsServerV3 nfs3 = new NfsServerV3(exportFile, vfs);
        MountServer mountd = new MountServer(exportFile, vfs);

        // register NFS servers at portmap service
        nfsSvc.register(new OncRpcProgram(100003, 4), nfs4);
        nfsSvc.register(new OncRpcProgram(100003, 3), nfs3);
        nfsSvc.register(new OncRpcProgram(100005, 3), mountd);

        // start RPC service
        nfsSvc.start();

        System.in.read();
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