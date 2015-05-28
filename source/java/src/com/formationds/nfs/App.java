package com.formationds.nfs;

import org.apache.log4j.PropertyConfigurator;
import org.dcache.nfs.ExportFile;
import org.dcache.nfs.v3.MountServer;
import org.dcache.nfs.v3.NfsServerV3;
import org.dcache.nfs.v4.*;
import org.dcache.nfs.vfs.*;
import org.dcache.xdr.OncRpcProgram;
import org.dcache.xdr.OncRpcSvc;
import org.dcache.xdr.OncRpcSvcBuilder;

import java.io.File;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.Properties;

/**
 * Created by root on 5/11/15.
 */
public class App {
    public static void main(String[] args) throws Exception {
        initConsoleLogging("DEBUG");
        // create an instance of a filesystem to be exported
        VirtualFileSystem vfs = new MemoryVirtualFileSystem();
        VirtualFileSystem troll = (VirtualFileSystem) Proxy.newProxyInstance(
                App.class.getClassLoader(),
                new Class[]{VirtualFileSystem.class},
                new InvocationHandler() {
                    @Override
                    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
                        System.out.println("Invoking " + method.getName());
                        return method.invoke(vfs, args);
                    }
                }
        );

        // create the RPC service which will handle NFS requests
        OncRpcSvc nfsSvc = new OncRpcSvcBuilder()
                .withPort(2049)
                .withTCP()
                .withAutoPublish()
                .withWorkerThreadIoStrategy()
                .build();

        // specify file with export entries
        ExportFile exportFile = new ExportFile(new File("./exports"));

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

        Inode root = vfs.getRootInode();
        vfs.mkdir(root, "exports", null, Stat.S_IFDIR | 755);

        // start RPC service
        nfsSvc.start();

        System.in.read();
    }

    private static void initConsoleLogging(String loglevel) {
        Properties properties = new Properties();
        properties.put("log4j.rootCategory", "INFO, console");
        properties.put("log4j.appender.console", "org.apache.log4j.ConsoleAppender");
        properties.put("log4j.appender.console.layout", "org.apache.log4j.PatternLayout");
        properties.put("log4j.appender.console.layout.ConversionPattern", "%-4r [%t] %-5p %c %x - %m%n");
        properties.put("log4j.logger.com.formationds", loglevel);
        //properties.put("log4j.logger.com.formationds.web.toolkit.Dispatcher", "WARN");
        PropertyConfigurator.configure(properties);
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
