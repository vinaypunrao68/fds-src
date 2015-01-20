package com.formationds.am;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.AmService;
import com.formationds.apis.AsyncAmServiceRequest;
import com.formationds.apis.ConfigurationService;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authenticator;
import com.formationds.security.Authorizer;
import com.formationds.security.DumbAuthorizer;
import com.formationds.security.FdsAuthenticator;
import com.formationds.security.FdsAuthorizer;
import com.formationds.security.NullAuthenticator;
import com.formationds.streaming.Streaming;
import com.formationds.util.Configuration;
import com.formationds.util.libconfig.Assignment;
import com.formationds.util.libconfig.ParsedConfig;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.util.thrift.OMConfigServiceClient;
import com.formationds.util.thrift.OMConfigServiceRestClientImpl;
import com.formationds.util.thrift.OMConfigurationServiceProxy;
import com.formationds.xdi.XdiClientFactory;
import com.formationds.web.toolkit.HttpConfiguration;
import com.formationds.web.toolkit.HttpsConfiguration;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.FakeAmService;
import com.formationds.xdi.FakeAsyncAm;
import com.formationds.xdi.RealAsyncAm;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.XdiAsync;
import com.formationds.xdi.XdiConfigurationApi;
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
        final Configuration configuration = new Configuration( "xdi", args );
        ParsedConfig platformConfig = configuration.getPlatformConfig();
        final Assignment aseKey = platformConfig.lookup("fds.aes_key");
        if( !aseKey.getValue().isPresent() ) {

            throw new RuntimeException(
                "The specified configuration key 'fds.aes.key' was not found." );

        }

        byte[] keyBytes = Hex.decodeHex( aseKey.stringValue()
                                               .toCharArray() );

        SecretKey secretKey = new SecretKeySpec( keyBytes, "AES" );

        // Get the instance ID from either the config file or cmd line
        final Assignment amInstance = platformConfig.lookup("fds.am.instanceId");
        int amInstanceId;
        if( !amInstance.getValue().isPresent() ) {

            OptionParser parser = new OptionParser();
            parser.allowsUnrecognizedOptions();
            parser.accepts( "fds.am.instanceId" )
                  .withRequiredArg()
                  .ofType( Integer.class );
            OptionSet options = parser.parse(args);
            if( options.has( "fds.am.instanceId" ) ) {

                amInstanceId = ( int ) options.valueOf( "fds.am.instanceId" );

            } else {

                throw new RuntimeException(
                    "The specified configuration key 'fds.am.instanceId' was not found." );
            }

        } else {

            amInstanceId = amInstance.intValue();

        }

        int amResponsePort = AM_BASE_RESPONSE_PORT + amInstanceId;
        LOG.debug( "My instance id " + amInstanceId +
                   " port " + amResponsePort );

        XdiClientFactory clientFactory = new XdiClientFactory(amResponsePort);

        String amHost = platformConfig.defaultString("fds.xdi.am_host", "localhost");
        boolean useFakeAm   = platformConfig.defaultBoolean( "fds.am.memory_backend", false );
        String  omHost      = platformConfig.defaultString("fds.am.om_ip", "localhost");
        Integer omHttpPort  = platformConfig.defaultInt("fds.om.http_port", 7777);
        Integer omHttpsPort = platformConfig.defaultInt("fds.om.https_port", 7443);

        // TODO: this needs to be configurable in platform.conf
        int omConfigPort = 9090;

        // TODO: the base service port needs to configurable in platform.conf
        int amServicePortBase = 9988;
        int amServicePort = amServicePortBase + amInstanceId;

        AmService.Iface am = useFakeAm ? new FakeAmService() :
                clientFactory.remoteAmService(amHost, amServicePort);

        // Create an OM REST Client and wrap the XdiConfigurationApi in the OM ConfigService Proxy.
        // This will result XDI create/delete Volume requests to redirect to the OM REST Client.
        // At this time all other requests continue to go through the XdiConfigurationApi (though
        // we may need to modify it so other requests are intercepted and redirected through OM REST
        // client in the future)
        OMConfigServiceClient omConfigServiceRestClient =
            new OMConfigServiceRestClientImpl(secretKey, "http", omHost, omHttpPort );
        ConfigurationApi omCachedConfigProxy =
            OMConfigurationServiceProxy.newOMConfigProxy( omConfigServiceRestClient,
                                                          clientFactory.remoteOmService( omHost,
                                                                                         omConfigPort ) );

        XdiConfigurationApi configCache = new XdiConfigurationApi( omCachedConfigProxy );

        // TODO: make cache update check configurable.
        // The config cache has been modified so that it captures all events that come through
        // the XDI apis.  However, it does not capture events that go direct through the
        // OM REST Client (i.e. UI or curl commands to OM Java web address), so we
        // still need to monitor the config service and update the cache on version changes.
        // The other alternative is to modify all of the XDI config cache methods to do a
        // refresh on cache miss.
        long configCacheUpdateIntervalMillis = 10000;
        configCache.startCacheUpdaterThread(configCacheUpdateIntervalMillis);

        boolean enforceAuth = platformConfig.lookup("fds.authentication").booleanValue();
        Authenticator authenticator = enforceAuth ?
                                      new FdsAuthenticator(configCache, secretKey) :
                                      new NullAuthenticator();
        Authorizer authorizer = enforceAuth ?
                                new FdsAuthorizer(configCache) :
                                new DumbAuthorizer();

        Xdi xdi = new Xdi(am, configCache, authenticator, authorizer);
        ByteBufferPool bbp = new ArrayByteBufferPool();

        AsyncAmServiceRequest.Iface oneWayAm = clientFactory.remoteOnewayAm(amHost, 8899);
        AsyncAm asyncAm = useFakeAm ?
                          new FakeAsyncAm() :
                          new RealAsyncAm(oneWayAm, amResponsePort);
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
        HttpsConfiguration httpsConfiguration = new HttpsConfiguration(s3SslPort,
                                                                       configuration );

        new Thread(() -> new S3Endpoint(xdi,
                                        factory,
                                        secretKey,
                                        httpsConfiguration,
                                        httpConfiguration).start(), "S3 service thread").start();

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

