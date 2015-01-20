/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om;

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import com.formationds.apis.AmService;
import com.formationds.apis.ConfigurationService;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.om.events.EventManager;
import com.formationds.om.helper.SingletonAmAPI;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.helper.SingletonConfiguration;
import com.formationds.om.helper.SingletonLegacyConfig;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.om.snmp.SnmpManager;
import com.formationds.om.snmp.TrapSend;
import com.formationds.om.webkit.WebKitImpl;
import com.formationds.security.Authenticator;
import com.formationds.security.Authorizer;
import com.formationds.security.DumbAuthorizer;
import com.formationds.security.FdsAuthenticator;
import com.formationds.security.FdsAuthorizer;
import com.formationds.security.NullAuthenticator;
import com.formationds.util.Configuration;
import com.formationds.util.libconfig.Assignment;
import com.formationds.util.libconfig.ParsedConfig;
import com.formationds.util.thrift.AmServiceClientFactory;
import com.formationds.util.thrift.ConfigServiceClientFactory;
import com.formationds.util.thrift.ThriftClientFactory;
import org.apache.commons.codec.binary.Hex;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;

public class Main {
    private static final Logger logger = LoggerFactory.getLogger( Main.class );

    // key for managing the singleton EventManager.
    private final Object eventMgrKey = new Object();

    public static void main(String[] args) {

        try {

            Configuration cfg = new Configuration( "om-xdi", args );
            new Main(cfg).start(args);

        } catch (Throwable t) {

            logger.error( "Error starting OM", t );
            System.out.println(t.getMessage());
            System.out.flush();
            System.exit(-1);

        }
    }

    public Main(Configuration cfg) {
        SingletonConfiguration.instance().setConfig(cfg);
    }

    public void start( String[] args )
        throws Exception {

        final Configuration configuration =
            SingletonConfiguration.instance()
                                  .getConfig();

        System.setProperty("fds-root", configuration.getFdsRoot());
        logger.trace( "FDS-ROOT: " + System.getProperty( "fds-root" ) );

        logger.trace("Starting native OM");
        NativeOm.startOm( args );

        logger.trace( "Loading platform configuration." );
        ParsedConfig platformConfig = configuration.getPlatformConfig();

        logger.trace( "Initializing repository event notifier." );
        EventManager.INSTANCE
                    .initEventNotifier(
                                        eventMgrKey,
                                        ( e ) -> {

                                            Assignment snmpTarget =
                                                platformConfig.lookup( TrapSend.SNMP_TARGET_KEY );
                                            if( snmpTarget.getValue().isPresent() ) {
                                                System.setProperty( TrapSend.SNMP_TARGET_KEY,
                                                                    snmpTarget.stringValue() );
                                                SnmpManager.instance()
                                                           .notify( e );
                                            } else {
                                                SnmpManager.instance()
                                                           .disable( SnmpManager.DisableReason
                                                                         .MISSING_CONFIG );
                                            }

                                            return (SingletonRepositoryManager.instance()
                                                                              .getEventRepository()
                                                                              .save( e ) != null);
                                        }
                    );

        if(FdsFeatureToggles.FIREBREAK_EVENT.isActive()) {

            logger.trace("Firebreak events feature is enabled.  Initializing repository firebreak callback.");
            // initialize the firebreak event listener (callback from repository persist)
            EventManager.INSTANCE.initEventListeners();

        } else {

            logger.info("Firebreak events feature is disabled.");

        }

        // TODO: should there be an OM property for the am host?
        String amHost = platformConfig.defaultString("fds.xdi.am_host", "localhost");

        // TODO: Should OM have a specific AM instance id configuration? how does OM handle multi-AM?
        int amInstanceId = platformConfig.defaultInt( "fds.am.instanceId", 0 );

        // TODO: the base service port needs to configurable in platform.conf
        int amServicePortBase = 9988;
        int amServicePort = amServicePortBase + amInstanceId;

        // TODO: this needs to be configurable in platform.conf
        int omConfigPort = 9090;

        ThriftClientFactory<ConfigurationService.Iface> configApiFactory =
            ConfigServiceClientFactory.newConfigService("localhost", omConfigPort);

        final OmConfigurationApi configCache = new OmConfigurationApi(configApiFactory);
        SingletonConfigAPI.instance().api( configCache );

        EnsureAdminUser.bootstrapAdminUser( configCache );

        AmService.Iface amService = AmServiceClientFactory.newAmService(amHost, amServicePort).getClient();
        SingletonAmAPI.instance().api( amService );

        String omHost = "localhost";
        int omLegacyConfigPort = platformConfig.defaultInt( "fds.om.config_port", 8903 );
        String webDir = platformConfig.defaultString( "fds.om.web_dir",
                                                      "../lib/admin-webapp" );

        FDSP_ConfigPathReq.Iface legacyConfigClient =
            ConfigServiceClientFactory.newLegacyConfigService(omHost,
                                                              omLegacyConfigPort)
                                      .getClient();

        SingletonLegacyConfig.instance()
                             .api( legacyConfigClient );


        char[] aesKey = platformConfig.defaultString( "fds.aes_key", "" ).toCharArray();
        if (aesKey.length == 0) {
            throw new RuntimeException( "The required configuration key 'fds.aes.key' was not found." );
        }
        byte[] keyBytes = Hex.decodeHex( aesKey );
        SecretKey secretKey = new SecretKeySpec( keyBytes, "AES" );

        final boolean enforceAuthentication = platformConfig.defaultBoolean( "fds.authentication", true );
        Authenticator authenticator =
            enforceAuthentication ? new FdsAuthenticator( configCache,
                                                          secretKey )
                                  : new NullAuthenticator();

        Authorizer authorizer = enforceAuthentication
            ? new FdsAuthorizer( configCache )
            : new DumbAuthorizer();

        int httpPort = platformConfig.defaultInt( "fds.om.http_port", 7777 );
        int httpsPort = platformConfig.defaultInt( "fds.om.https_port", 7443 );

        // TODO: pass om host/port (or url) to stat stream registration handler
        // create and start the statistics stream registration handler to manage stat
        // stream register/deregister requests
        configCache.startStatStreamRegistrationHandler();

        if( FdsFeatureToggles.WEB_KIT.isActive() ) {

            logger.info( "Web toolkit enabled" );
            new WebKitImpl( authenticator,
                            authorizer,
                            webDir,
                            httpPort,
                            httpsPort,
                            secretKey ).start( );

        } else {

            logger.info( "Web toolkit disabled" );

        }
    }
}

