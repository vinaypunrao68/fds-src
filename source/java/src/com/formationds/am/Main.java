package com.formationds.am;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.AmService;
import com.formationds.apis.ConfigurationService;
import com.formationds.commons.events.EventManager;
import com.formationds.nbd.*;
import com.formationds.om.repository.EventRepository;
import com.formationds.security.*;
import com.formationds.streaming.Streaming;
import com.formationds.util.Configuration;
import com.formationds.util.libconfig.ParsedConfig;
import com.formationds.web.toolkit.HttpConfiguration;
import com.formationds.web.toolkit.HttpsConfiguration;
import com.formationds.xdi.*;
import com.formationds.xdi.s3.S3Endpoint;
import com.formationds.xdi.swift.SwiftEndpoint;
import org.apache.commons.codec.binary.Hex;
import org.apache.log4j.Logger;
import org.apache.thrift.server.TNonblockingServer;
import org.apache.thrift.transport.TNonblockingServerSocket;
import org.apache.thrift.transport.TTransportException;
import org.eclipse.jetty.io.ArrayByteBufferPool;
import org.eclipse.jetty.io.ByteBufferPool;

import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;
import java.util.concurrent.ForkJoinPool;
import java.util.function.Function;

public class Main {
    private static Logger LOG = Logger.getLogger(Main.class);

    private Configuration configuration;

    // key for managing the singleton EventManager.
    private final Object eventMgrKey = new Object();

    public static void main(String[] args) {
        try {
            new Main().start(args);
        } catch (Throwable throwable) {
            LOG.fatal("Error starting AM", throwable);
            System.out.println(throwable.getMessage());
            System.out.flush();
            System.exit(-1);
        }
    }

    public void start(String[] args) throws Exception {
        configuration = new Configuration("xdi", args);
        ParsedConfig platformConfig = configuration.getPlatformConfig();
        byte[] keyBytes = Hex.decodeHex(platformConfig.lookup("fds.aes_key").stringValue().toCharArray());
        SecretKey secretKey = new SecretKeySpec(keyBytes, "AES");

        // TODO: this is needed before bootstrapping the admin user but not sure if there is config required first.
        // I want to consolidate event management in a single process, but can't do that currently since the
        // AM starts all of the data connectors other than the webapp driving the UI in the OM.
//        EventRepository eventRepository = new EventRepository();
//        EventManager.INSTANCE.initEventNotifier(eventMgrKey, (e) -> { return eventRepository.save(e) != null; });

        XdiClientFactory clientFactory = new XdiClientFactory();

        boolean useFakeAm = platformConfig.lookup("fds.am.memory_backend").booleanValue();
        String omHost = platformConfig.lookup("fds.am.om_ip").stringValue();
        int omConfigPort = 9090;
        int omLegacyConfigPort = platformConfig.lookup("fds.am.om_config_port").intValue();

        AmService.Iface am = useFakeAm ? new FakeAmService() :
                                         clientFactory.remoteAmService("localhost", 9988);

        ConfigurationApi configCache = new ConfigurationApi(clientFactory.remoteOmService(omHost, omConfigPort));
        boolean enforceAuth = platformConfig.lookup("fds.authentication").booleanValue();
        Authenticator authenticator = enforceAuth ? new FdsAuthenticator(configCache, secretKey) : new NullAuthenticator();
        Authorizer authorizer = enforceAuth ? new FdsAuthorizer(configCache) : new DumbAuthorizer();

        int nbdPort = platformConfig.lookup("fds.am.nbd_server_port").intValue();
        boolean nbdLoggingEnabled = platformConfig.defaultBoolean("fds.am.enable_nbd_log", false);
        boolean nbdBlockExclusionEnabled = platformConfig.defaultBoolean("fds.am.enable_nbd_block_exclusion", true);
        ForkJoinPool fjp = new ForkJoinPool(50);
        NbdServerOperations ops = new FdsServerOperations(am, configCache, fjp);
        if (nbdLoggingEnabled)
            ops = new LoggingOperationsWrapper(ops, "/fds/var/logs/nbd");
        if (nbdBlockExclusionEnabled)
            ops = new BlockExclusionWrapper(ops, 4096);
        NbdHost nbdHost = new NbdHost(nbdPort, ops);

        new Thread(() -> nbdHost.run(), "NBD thread").start();

        Xdi xdi = new Xdi(am, configCache, authenticator, authorizer, clientFactory.legacyConfig(omHost, omLegacyConfigPort));
        ByteBufferPool bbp = new ArrayByteBufferPool();
        AsyncAm asyncAm = new AsyncAm(clientFactory.makeAmAsyncPool("localhost", 9988), authorizer);
        Function<AuthenticationToken, XdiAsync> factory = (token) -> new XdiAsync(asyncAm, bbp, token, configCache);

        int s3HttpPort = platformConfig.defaultInt("fds.am.s3_http_port", 8000);
        int s3SslPort = platformConfig.defaultInt("fds.am.s3_https_port", 8443);

        HttpConfiguration httpConfiguration = new HttpConfiguration(s3HttpPort, "0.0.0.0");
        HttpsConfiguration httpsConfiguration = new HttpsConfiguration(s3SslPort, configuration);

        new Thread(() -> new S3Endpoint(xdi, factory, secretKey, httpsConfiguration, httpConfiguration).start(), "S3 service thread").start();

        startStreamingServer(8999, configCache);

        int swiftPort = platformConfig.lookup("fds.am.swift_port").intValue();
        new SwiftEndpoint(xdi, secretKey).start(swiftPort);
    }

    private void startStreamingServer(final int port, ConfigurationService.Iface configClient) {
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

        new Thread(runnable, "Statistics streaming thread").start();
    }

}

