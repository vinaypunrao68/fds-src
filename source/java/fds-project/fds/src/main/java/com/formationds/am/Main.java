/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
package com.formationds.am;

import com.formationds.apis.ConfigurationService;
import com.formationds.commons.libconfig.Assignment;
import com.formationds.commons.libconfig.ParsedConfig;
import com.formationds.nfs.*;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.security.*;
import com.formationds.streaming.Streaming;
import com.formationds.util.Configuration;
import com.formationds.util.ServerPortFinder;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.util.thrift.OMConfigurationServiceProxy;
import com.formationds.web.toolkit.HttpConfiguration;
import com.formationds.web.toolkit.HttpsConfiguration;
import com.formationds.xdi.*;
import com.formationds.xdi.contracts.ConnectorConfig;
import com.formationds.xdi.contracts.ConnectorData;
import com.formationds.xdi.contracts.transport.ConnectorConfigMessageHandler;
import com.formationds.xdi.contracts.transport.ConnectorDataMessageHandler;
import com.formationds.xdi.contracts.transport.TransportServer;
import com.formationds.xdi.contracts.transport.pipe.NamedPipeServer;
import com.formationds.xdi.experimental.XdiConnector;
import com.formationds.xdi.s3.S3Endpoint;
import com.formationds.xdi.swift.SwiftEndpoint;
import com.nurkiewicz.asyncretry.AsyncRetryExecutor;
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
import java.io.IOException;
import java.net.ConnectException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;
import java.util.function.Supplier;

public class Main {
    private static Logger LOG = Logger.getLogger(Main.class);

    public static void main(String[] args) {
        System.setProperty("org.eclipse.jetty.http.HttpParser.STRICT", "true"); // disable jetty header modification
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
        final Configuration configuration = new Configuration("xdi", args);
        ParsedConfig platformConfig = configuration.getPlatformConfig();
        final Assignment aseKey = platformConfig.lookup("fds.aes_key");
        if (!aseKey.getValue().isPresent()) {

            throw new RuntimeException(
                    "The specified configuration key 'fds.aes.key' was not found.");

        }

        byte[] keyBytes = Hex.decodeHex(aseKey.stringValue()
                .toCharArray());

        SecretKey secretKey = new SecretKeySpec(keyBytes, "AES");

        // Get the platform from either the config file or cmd line
        final Assignment platPort = platformConfig.lookup("fds.pm.platform_port");
        int pmPort;
        if (!platPort.getValue().isPresent()) {
            OptionParser parser = new OptionParser();
            parser.allowsUnrecognizedOptions();
            parser.accepts("fds.pm.platform_port")
                    .withRequiredArg()
                    .ofType(Integer.class);
            OptionSet options = parser.parse(args);
            if (options.has("fds.pm.platform_port")) {

                pmPort = (int) options.valueOf("fds.pm.platform_port");

            } else {

                throw new RuntimeException(
                        "The specified configuration key 'fds.pm.platform_port' was not found.");
            }

        } else {

            pmPort = platPort.intValue();

        }

        int amResponsePortOffset = platformConfig.defaultInt("fds.am.am_base_response_port_offset", 2876);
        int amResponsePort = new ServerPortFinder().findPort("Async AM response port", pmPort + amResponsePortOffset);
        LOG.debug("PM port " + pmPort +
                " my port " + amResponsePort);

        String amHost = platformConfig.defaultString("fds.xdi.am_host", "localhost");
        boolean useFakeAm = platformConfig.defaultBoolean( "fds.am.memory_backend", false );

        String omHost = configuration.getOMIPAddress();
        // TODO: we should probably be using https port
        int omHttpPort = platformConfig.defaultInt( "fds.om.http_port", 7777 );
        int omConfigPort = platformConfig.defaultInt( "fds.om.config_port", 9090 );

        int xdiServicePortOffset = platformConfig.defaultInt("fds.am.xdi_service_port_offset", 1899);
        int streamingPortOffset = platformConfig.defaultInt("fds.am.streaming_port_offset", 1911);

        XdiStaticConfiguration xdiStaticConfig = configuration.getXdiStaticConfig( pmPort );

        // Create an OM REST Client and wrap the XdiConfigurationApi in the OM ConfigService Proxy.
        // This will result XDI create/delete Volume requests to redirect to the OM REST Client.
        // At this time all other requests continue to go through the XdiConfigurationApi (though
        // we may need to modify it so other requests are intercepted and redirected through OM REST
        // client in the future)

        LOG.debug( "Attempting to connect to REST configuration API, on " +
                   omHost + ":" + omHttpPort + "." );
        final AsyncRetryExecutor asyncRetryExecutor =
            new AsyncRetryExecutor(
                Executors.newSingleThreadScheduledExecutor( ) );

        asyncRetryExecutor.withExponentialBackoff( 500, 2 )
                          .withMaxDelay( 10_000 )
                          .withUniformJitter( )
                          .retryInfinitely()
                          .retryOn( ExecutionException.class )
                          .retryOn( RuntimeException.class )
                          .retryOn( TTransportException.class )
                          .retryOn( ConnectException.class );

        LOG.debug( "Attempting to connect to configuration API, on " +
                   omHost + ":" + omHttpPort + "." );
        final CompletableFuture<ConfigurationApi> cfgApicompletableFuture =
            asyncRetryExecutor.getWithRetry(
                ( ) -> {
                    return OMConfigurationServiceProxy.newOMConfigProxy( secretKey,
                                                                         "http",
                                                                         omHost,
                                                                         omHttpPort,
                                                                         omConfigPort );
                } );

        final ConfigurationApi omCachedConfigProxy = cfgApicompletableFuture.get();
        if( omCachedConfigProxy == null )
        {
            throw new RuntimeException( "Failed to connect to " +
                                        omHost + ":" + omConfigPort );
        }

        LOG.debug( "Attempting to connect to XDI configuration API, on " +
                   omHost + ":" + omHttpPort + "." );
        final CompletableFuture<XdiConfigurationApi> XdicompletableFuture =
            asyncRetryExecutor.getWithRetry(
                ( ) -> {
                    return new XdiConfigurationApi( omCachedConfigProxy );
                } );

        final XdiConfigurationApi configCache = XdicompletableFuture.get();
        if( configCache == null )
        {
            throw new RuntimeException( "Failed to populate XDI config cache" );
        }

        // initialize darth vader.  Note that this is kinda circular and sucks!
        // The v08 OM Rest Client uses the ExternalModelConverter, which accesses
        // the config API through this singleton, but it ultimately references back
        // to to OMConfigurationServiceProxy.
        SingletonConfigAPI.instance().api( configCache );
        
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

        AsyncAm asyncAm = useFakeAm ?
                new FakeAsyncAm() :
                new RealAsyncAm(amHost, pmPort + xdiServicePortOffset, amResponsePort, xdiStaticConfig.getAmTimeout());
        asyncAm.start();

        // TODO: should XdiAsync use omCachedConfigProxy too?
        Supplier<AsyncStreamer> factory = () -> new AsyncStreamer(asyncAm,
                configCache);

        Xdi xdi = new Xdi(configCache, authenticator, authorizer, asyncAm, factory);


        int s3HttpPort = platformConfig.defaultInt("fds.am.s3_http_port_offset", 1000);
        int s3SslPort = platformConfig.defaultInt("fds.am.s3_https_port_offset", 1443);
        s3HttpPort += pmPort;   // remains 8000 for default platform port
        s3SslPort += pmPort;    // remains 8443 for default platform port

        HttpConfiguration httpConfiguration = new HttpConfiguration(s3HttpPort, "0.0.0.0");
        HttpsConfiguration httpsConfiguration = new HttpsConfiguration(s3SslPort,
                configuration);

        int s3MaxConcurrentRequests = platformConfig.defaultInt("fds.am.s3_max_concurrent_requests", 500);
        new Thread(() -> new S3Endpoint(xdi,
                factory,
                secretKey,
                httpsConfiguration,
                httpConfiguration,
                s3MaxConcurrentRequests).start(), "S3 service thread").start();

        // Experimental: XDI server
        IoOps ioOps = new DeferredIoOps(new AmOps(asyncAm), v -> {
            try {
                return configCache.statVolume(XdiVfs.DOMAIN, v).getPolicy().getMaxObjectSizeInBytes();
            } catch (Exception e) {
                throw new IOException(e);
            }
        });
        // ioOps = new MemoryIoOps();
        XdiConnector connector = new XdiConnector(configCache, ioOps);

        // start XDI connector servers -- add this to config?
        TransportServer xdiConfigServer = createXdiConfigServer(connector, Executors.newFixedThreadPool(4));
        monitorNamedPipePath(xdiConfigServer, Paths.get("/tmp/xdi/config"));

        TransportServer xdiDataServer = createXdiDataServer(connector, Executors.newFixedThreadPool(16));
        monitorNamedPipePath(xdiDataServer, Paths.get("/tmp/xdi/data"));

        // Default NFS port is 2049, or 7000 - 4951
        new NfsServer().start(xdiStaticConfig, configCache, asyncAm, pmPort - 4951);
        startStreamingServer(pmPort + streamingPortOffset, configCache);
        int swiftPort = platformConfig.defaultInt("fds.am.swift_port_offset", 2999);
        swiftPort += pmPort;  // remains 9999 for default platform port
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

    private void monitorNamedPipePath(TransportServer server, Path path) {
        NamedPipeServer namedPipeServer = new NamedPipeServer(server, path);
        Thread thread = new Thread(namedPipeServer::open);
        thread.setDaemon(true);
        thread.start();
    }

    private TransportServer createXdiDataServer(ConnectorData connectorData, Executor runner) {
        ConnectorDataMessageHandler handler = new ConnectorDataMessageHandler(connectorData);
        return new TransportServer(handler, runner);
    }

    private TransportServer createXdiConfigServer(ConnectorConfig connectorConfig, Executor runner) {
        ConnectorConfigMessageHandler handler = new ConnectorConfigMessageHandler(connectorConfig);
        return new TransportServer(handler, runner);
    }
}

