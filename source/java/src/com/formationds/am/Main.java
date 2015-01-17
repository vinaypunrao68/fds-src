package com.formationds.am;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.AmService;
import com.formationds.apis.AsyncAmServiceRequest;
import com.formationds.apis.ConfigurationService;
import com.formationds.security.*;
import com.formationds.streaming.Streaming;
import com.formationds.util.Configuration;
import com.formationds.util.libconfig.ParsedConfig;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.util.thrift.OMConfigServiceClient;
import com.formationds.util.thrift.OMConfigServiceRestClientImpl;
import com.formationds.util.thrift.OMConfigurationServiceProxy;
import com.formationds.xdi.XdiClientFactory;
import com.formationds.web.toolkit.HttpConfiguration;
import com.formationds.web.toolkit.HttpsConfiguration;
import com.formationds.xdi.*;
import com.formationds.xdi.s3.S3Endpoint;
import com.formationds.xdi.swift.SwiftEndpoint;
import joptsimple.OptionParser;
import joptsimple.OptionSet;
import org.apache.commons.codec.binary.Hex;
import org.apache.log4j.Logger;
import org.apache.thrift.server.TNonblockingServer;
import org.apache.thrift.transport.TNonblockingServerSocket;
import org.apache.thrift.transport.TTransportException;
import org.eclipse.jetty.io.ArrayByteBufferPool;
import org.eclipse.jetty.io.ByteBufferPool;

import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;
import java.util.function.Function;

public class Main {
    public static final int AM_BASE_RESPONSE_PORT = 9876;
    private static Logger LOG = Logger.getLogger(Main.class);
    // key for managing the singleton EventManager.
    private final Object eventMgrKey = new Object();
    private Configuration configuration;

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

        // Get the instance ID from either the config file or cmd line
        int amInstanceId = platformConfig.lookup("fds.am.instanceId").intValue();
        OptionParser parser = new OptionParser();
        parser.allowsUnrecognizedOptions();
        parser.accepts("fds.am.instanceId").withRequiredArg().ofType(Integer.class);
        OptionSet options = parser.parse(args);
        if (options.has("fds.am.instanceId")) {
            amInstanceId = (int)options.valueOf("fds.am.instanceId");
        }

        int amResponsePort = AM_BASE_RESPONSE_PORT + amInstanceId;
        System.out.println("My instance id " + amInstanceId);

        XdiClientFactory clientFactory = new XdiClientFactory(amResponsePort);

        boolean useFakeAm = platformConfig.lookup("fds.am.memory_backend").booleanValue();
        String omHost = platformConfig.lookup("fds.am.om_ip").stringValue();
        Integer omHttpPort = platformConfig.defaultInt("fds.om.http_port", 7777);
        Integer omHttpsPort = platformConfig.defaultInt("fds.om.http_port", 7443);

        int omConfigPort = 9090;

        String amHost = platformConfig.lookup("fds.xdi.am_host").stringValue();

        AmService.Iface am = useFakeAm ? new FakeAmService() :
                clientFactory.remoteAmService(amHost, 9988 + amInstanceId);

        XdiConfigurationApi configCache = new XdiConfigurationApi(clientFactory.remoteOmService(omHost, omConfigPort));
        boolean enforceAuth = platformConfig.lookup("fds.authentication").booleanValue();
        Authenticator authenticator = enforceAuth ? new FdsAuthenticator(configCache, secretKey) : new NullAuthenticator();
        Authorizer authorizer = enforceAuth ? new FdsAuthorizer(configCache) : new DumbAuthorizer();

        // Create an OM REST Client and wrap the XdiConfigurationApi in the OM ConfigService Proxy.
        // This will result XDI create/delete Volume requests to redirect to the OM REST Client.
        // At this time all other requests continue to go through the XdiConfigurationApi (though
        // we may need to modify it so other requests are intercepted and redirected through OM REST
        // client in the future)
        OMConfigServiceClient omConfigServiceRestClient =
            new OMConfigServiceRestClientImpl(secretKey, "http", omHost, omHttpPort );
        ConfigurationApi omCachedConfigProxy =
            OMConfigurationServiceProxy.newOMConfigProxy( omConfigServiceRestClient,
                                                          configCache);

        Xdi xdi = new Xdi(am, omCachedConfigProxy, authenticator, authorizer);
        ByteBufferPool bbp = new ArrayByteBufferPool();

        AsyncAmServiceRequest.Iface oneWayAm = clientFactory.remoteOnewayAm(amHost, 8899);
        AsyncAm asyncAm = useFakeAm ? new FakeAsyncAm() : new RealAsyncAm(oneWayAm, amResponsePort);
        asyncAm.start();

        // TODO: should XdiAsync use omCachedConfigProxy too?
        Function<AuthenticationToken, XdiAsync> factory = (token) -> new XdiAsync(asyncAm,
                                                                                  bbp,
                                                                                  token,
                                                                                  authorizer,
                                                                                  configCache);

        int s3HttpPort = platformConfig.defaultInt("fds.am.s3_http_port", 8000);
        int s3SslPort = platformConfig.defaultInt("fds.am.s3_https_port", 8443);
        s3HttpPort += amInstanceId;
        s3SslPort += amInstanceId;

        HttpConfiguration httpConfiguration = new HttpConfiguration(s3HttpPort, "0.0.0.0");
        HttpsConfiguration httpsConfiguration = new HttpsConfiguration(s3SslPort, configuration);

        new Thread(() -> new S3Endpoint(xdi, factory, secretKey, httpsConfiguration, httpConfiguration).start(), "S3 service thread").start();

        startStreamingServer(8999 + amInstanceId, configCache);

        int swiftPort = platformConfig.lookup("fds.am.swift_port").intValue();
        swiftPort += amInstanceId;
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

