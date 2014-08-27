package com.formationds.am;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.AmService;
import com.formationds.apis.ConfigurationService;
import com.formationds.fdsp.LegacyClientFactory;
import com.formationds.nbd.*;
import com.formationds.security.*;
import com.formationds.streaming.Streaming;
import com.formationds.util.Configuration;
import com.formationds.util.libconfig.ParsedConfig;
import com.formationds.xdi.*;
import com.formationds.xdi.s3.S3Endpoint;
import com.formationds.xdi.swift.SwiftEndpoint;
import org.apache.log4j.Logger;
import org.apache.thrift.server.TNonblockingServer;
import org.apache.thrift.transport.TNonblockingServerSocket;
import org.apache.thrift.transport.TTransportException;
import org.eclipse.jetty.io.ArrayByteBufferPool;
import org.eclipse.jetty.io.ByteBufferPool;

import java.util.concurrent.ForkJoinPool;

public class Main {
    private static Logger LOG = Logger.getLogger(Main.class);

    public static void main(String[] args) throws Exception {
        Configuration configuration = new Configuration("xdi", args);
        try {
            ParsedConfig amParsedConfig = configuration.getPlatformConfig();
            NativeAm.startAm(args);
            Thread.sleep(200);

            XdiClientFactory clientFactory = new XdiClientFactory();
            LegacyClientFactory legacyClientFactory = new LegacyClientFactory();

            boolean useFakeAm = amParsedConfig.lookup("fds.am.memory_backend").booleanValue();
            String omHost = amParsedConfig.lookup("fds.am.om_ip").stringValue();
            int omConfigPort = 9090;
            int omLegacyConfigPort = amParsedConfig.lookup("fds.am.om_config_port").intValue();

            AmService.Iface am = useFakeAm ? new FakeAmService() :
                    clientFactory.remoteAmService("localhost", 9988);
                    //clientFactory.remoteAmService("localhost", 4242);

            CachingConfigurationService config = new CachingConfigurationService(clientFactory.remoteOmService(omHost, omConfigPort));

            boolean enforceAuthorization = amParsedConfig.lookup("fds.am.authentication").booleanValue();
            Authenticator authenticator = enforceAuthorization? new FdsAuthenticator(config) : new NullAuthenticator();

            int nbdPort = amParsedConfig.lookup("fds.am.nbd_server_port").intValue();
            boolean nbdLoggingEnabled =  amParsedConfig.defaultBoolean("fds.am.enable_nbd_log", false);
            boolean nbdBlockExclusionEnabled = amParsedConfig.defaultBoolean("fds.am.enable_nbd_block_exclusion", true);
            ForkJoinPool fjp = new ForkJoinPool(50);
            NbdServerOperations ops = new FdsServerOperations(am, config, fjp);
            if(nbdLoggingEnabled)
                ops = new LoggingOperationsWrapper(ops, "/fds/var/logs/nbd");
            if(nbdBlockExclusionEnabled)
                ops = new BlockExclusionWrapper(ops, 4096);
            NbdHost nbdHost = new NbdHost(nbdPort, ops);
            
            new Thread(() -> nbdHost.run()).start();

            Authorizer authorizer = new DumbAuthorizer();
            Xdi xdi = new Xdi(am, config, authenticator, authorizer, legacyClientFactory.configPathClient(omHost, omLegacyConfigPort));
            ByteBufferPool bbp = new ArrayByteBufferPool();
            XdiAsync xdiAsync = new XdiAsync(clientFactory.remoteAmServiceAsync("localhost", 9988), clientFactory.remoteOmServiceAsync(omHost, omConfigPort), bbp);

            int s3Port = amParsedConfig.lookup("fds.am.s3_port").intValue();
            new Thread(() -> new S3Endpoint(xdi, xdiAsync).start(s3Port)).start();

            startStreamingServer(8999, config);

            int swiftPort = amParsedConfig.lookup("fds.am.swift_port").intValue();
            new SwiftEndpoint(xdi).start(swiftPort);
        } catch (Throwable throwable) {
            LOG.fatal("Error starting AM", throwable);
            System.out.println(throwable.getMessage());
            System.out.flush();
            System.exit(-1);
        }
    }

    private static void startStreamingServer(final int port, ConfigurationService.Iface configClient) {
        StatisticsPublisher statisticsPublisher = new StatisticsPublisher(configClient);

        Runnable runnable = () -> {
            try {
                TNonblockingServerSocket transport = new TNonblockingServerSocket(port);
                TNonblockingServer.Args args = new TNonblockingServer.Args(transport)
                        .processor(new Streaming.Processor<Streaming.Iface>(statisticsPublisher));
                TNonblockingServer server = new TNonblockingServer(args);
                server.serve();
            } catch (TTransportException e) {
                throw new RuntimeException(e);
            }
        };

        new Thread(runnable).start();
    }

}

