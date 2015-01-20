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

        // TODO: this is needed before bootstrapping the admin user but not sure if there is config required first.
        // I want to consolidate event management in a single process, but can't do that currently since the
        // AM starts all of the data connectors other than the webapp driving the UI in the OM.
//        EventRepository eventRepository = new EventRepository();
//        EventManager.INSTANCE.initEventNotifier(eventMgrKey, (e) -> { return eventRepository.save(e) != null; });

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

        final boolean useFakeAm =
            platformConfig.defaultBoolean( "fds.am.memory_backend", false );
        final String omHost =
            platformConfig.defaultString( "fds.am.om_ip", "localhost" );
        // TODO should we make this configurable?
        final int omConfigPort = 9090;
        int omLegacyConfigPort =
            platformConfig.defaultInt( "fds.am.om_config_port", 8903 );

        final String amHost =
            platformConfig.defaultString( "fds.xdi.am_host", "localhost" );
        final int amServicePort =
            platformConfig.defaultInt( "fds.am.om_ip_port", 9988 );

        AmService.Iface am =
            useFakeAm ? new FakeAmService()
                      : clientFactory.remoteAmService( amHost,
                                                       amServicePort + amInstanceId );

        XdiConfigurationApi configCache = new XdiConfigurationApi(clientFactory.remoteOmService(omHost, omConfigPort));
        boolean enforceAuth = platformConfig.defaultBoolean( "fds.authentication", false );
        Authenticator authenticator = enforceAuth ? new FdsAuthenticator(configCache, secretKey) : new NullAuthenticator();
        Authorizer authorizer = enforceAuth ? new FdsAuthorizer(configCache) : new DumbAuthorizer();

        Xdi xdi = new Xdi(am, configCache, authenticator, authorizer, clientFactory.legacyConfig(omHost, omLegacyConfigPort));
        ByteBufferPool bbp = new ArrayByteBufferPool();

        AsyncAmServiceRequest.Iface oneWayAm = clientFactory.remoteOnewayAm(amHost, 8899);
        AsyncAm asyncAm = useFakeAm ? new FakeAsyncAm() : new RealAsyncAm(oneWayAm, amResponsePort);
        asyncAm.start();

        Function<AuthenticationToken, XdiAsync> factory = (token) -> new XdiAsync(asyncAm, bbp, token, authorizer, configCache);

        int s3HttpPort = platformConfig.defaultInt("fds.am.s3_http_port", 8000);
        int s3SslPort = platformConfig.defaultInt("fds.am.s3_https_port", 8443);
        s3HttpPort += amInstanceId;
        s3SslPort += amInstanceId;

        HttpConfiguration httpConfiguration = new HttpConfiguration(s3HttpPort, "0.0.0.0");
        HttpsConfiguration httpsConfiguration = new HttpsConfiguration(s3SslPort,
                                                                       configuration );

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

