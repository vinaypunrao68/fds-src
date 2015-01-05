/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om;

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import com.formationds.apis.AmService;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.om.helper.SingletonAmAPI;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.helper.SingletonConfiguration;
import com.formationds.om.helper.SingletonLegacyConfig;
import com.formationds.om.helper.SingletonXdi;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.om.webkit.WebKitImpl;
import com.formationds.security.Authenticator;
import com.formationds.security.Authorizer;
import com.formationds.security.DumbAuthorizer;
import com.formationds.security.FdsAuthenticator;
import com.formationds.security.FdsAuthorizer;
import com.formationds.security.NullAuthenticator;
import com.formationds.util.Configuration;
import com.formationds.util.libconfig.ParsedConfig;
import com.formationds.web.toolkit.WebApp;
import com.formationds.xdi.ConfigurationApi;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.XdiClientFactory;
import org.apache.commons.codec.binary.Hex;
import org.apache.log4j.Logger;

import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;

import static com.formationds.om.events.EventManager.INSTANCE;

public class Main {
    private static final Logger LOG = Logger.getLogger(Main.class);

    // key for managing the singleton EventManager.
    private final Object eventMgrKey = new Object();

    private WebApp webApp;
    private ConfigurationApi configCache;

    public static void main(String[] args) {

        try {

            Configuration cfg = new Configuration( "om-xdi", args );
            new Main(cfg).start(args);

        } catch (Throwable t) {

            LOG.fatal("Error starting OM", t);
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
        LOG.trace( "FDS-ROOT: " + System.getProperty( "fds-root" ) );

        LOG.trace("Starting native OM");
        NativeOm.startOm(args);

        LOG.trace( "Loading platform configuration." );
        ParsedConfig platformConfig = configuration.getPlatformConfig();

        // TODO: this is needed before bootstrapping the admin user but not sure if there is config required first.
        // alternatively, we could initialize it with an empty event notifier or disabled flag and not log the
        // initial first-time bootstrap of the admin user as an event.
        LOG.trace("Initializing repository event notifier.");
        INSTANCE.initEventNotifier(
            eventMgrKey,
            ( e ) -> SingletonRepositoryManager.instance()
                                               .getEventRepository()
                                               .save( e ) != null );

        if(FdsFeatureToggles.FIREBREAK_EVENT.isActive()) {

            LOG.trace("Firebreak events feature is enabled.  Initializing repository firebreak callback.");
            // initialize the firebreak event listener (callback from repository persist)
            INSTANCE.initEventListeners();

        } else {

            LOG.info("Firebreak events feature is disabled.");

        }

        XdiClientFactory clientFactory = new XdiClientFactory();
        configCache =
            new ConfigurationApi(
                clientFactory.remoteOmService( "localhost", 9090 ) );
        SingletonConfigAPI.instance().api( configCache );

        EnsureAdminUser.bootstrapAdminUser( configCache );

        AmService.Iface amService =
            clientFactory.remoteAmService( "localhost", 9988 );
        SingletonAmAPI.instance().api( amService );

        String omHost = "localhost";
        int omPort = platformConfig.defaultInt( "fds.om.config_port", 8903 );
        String webDir =
            platformConfig.defaultString( "fds.om.web_dir",
                                          "../lib/admin-webapp" );

        FDSP_ConfigPathReq.Iface legacyConfigClient =
            clientFactory.legacyConfig( omHost, omPort );
        SingletonLegacyConfig.instance()
                             .api( legacyConfigClient );

        byte[] keyBytes = Hex.decodeHex( platformConfig.lookup( "fds.aes_key" )
                                                       .stringValue()
                                                       .toCharArray() );
        SecretKey secretKey = new SecretKeySpec( keyBytes, "AES" );

        final boolean enforceAuthentication =
            platformConfig.defaultBoolean( "fds.authentication", true );
        Authenticator authenticator =
            enforceAuthentication ? new FdsAuthenticator( configCache,
                                                          secretKey )
                                  : new NullAuthenticator();

        Authorizer authorizer = enforceAuthentication
            ? new FdsAuthorizer( configCache )
            : new DumbAuthorizer();

        final Xdi xdi = new Xdi( amService,
                                 configCache,
                                 authenticator,
                                 authorizer,
                                 legacyConfigClient );
        SingletonXdi.instance()
                    .api( xdi );
        int httpPort = platformConfig.defaultInt( "fds.om.http_port", 7777 );
        int httpsPort = platformConfig.defaultInt( "fds.om.https_port", 7443 );


        if( FdsFeatureToggles.WEB_KIT.isActive() ) {

            new WebKitImpl( authorizer,
                            webDir,
                            httpPort,
                            httpsPort,
                            secretKey ).start( args );

        } else {

            dropwizard();

        }
    }

    private void dropwizard() {

    }
}

