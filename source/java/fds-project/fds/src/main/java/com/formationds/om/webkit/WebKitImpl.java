/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit;

import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.om.helper.SingletonConfiguration;
import com.formationds.om.webkit.rest.LandingPage;
import com.formationds.security.Authenticator;
import com.formationds.security.Authorizer;
import com.formationds.web.toolkit.HttpConfiguration;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.HttpsConfiguration;
import com.formationds.web.toolkit.WebApp;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.crypto.SecretKey;

/**
 * @author ptinius
 */
public class WebKitImpl {

    private static final Logger logger =
        LoggerFactory.getLogger( WebKitImpl.class );

    private WebApp webApp;

    private final String webDir;
    private final int httpPort;
    private final int httpsPort;
    private final Authenticator authenticator;
    private final Authorizer authorizer;
    private final SecretKey secretKey;

    public WebKitImpl( final Authenticator authenticator,
                       final Authorizer authorizer,
                       final String webDir,
                       final int httpPort,
                       final int httpsPort,
                       final SecretKey secretKey ) {

        this.authenticator = authenticator;
        this.authorizer = authorizer;
        this.webDir = webDir;
        this.httpPort = httpPort;
        this.httpsPort = httpsPort;
        this.secretKey = secretKey;

    }

    /**
     * TODO:  Remove
     *
     * This is a way for us to be able to serve both the new, and old API versions simultaneously
     *
     * @return
     */
    public WebApp getWebApp(){
    	return this.webApp;
    }

    public void start( ) {

        webApp = new OmWebApp( webDir );
        webApp.route( HttpMethod.GET, "", ( ) -> new LandingPage( webDir ) );

        if ( FdsFeatureToggles.REST_07.isActive() ){

            logger.info( "Initializing REST API v07..." );
            AbstractApiDefinition rest07 =
                new com.formationds.om.webkit.rest.v07.ApiDefinition(
                    this.authenticator,
                    this.authorizer,
                    this.secretKey,
                    webApp );

            rest07.configure();
            logger.info( "Completed initializing REST API v07." );

        }

        logger.info( "Initializing REST API v08..." );
        AbstractApiDefinition rest08 =
            new com.formationds.om.webkit.rest.v08.ApiDefinition(
                this.authenticator,
                this.authorizer,
                this.secretKey,
                webApp );

        rest08.configure();
        logger.info( "Completed initializing REST API v08." );

        logger.info( "Starting web app");
        webApp.start(
            new HttpConfiguration( httpPort ),
            new HttpsConfiguration( httpsPort,
                                    SingletonConfiguration.instance()
                                                          .getConfig() ) );
    }
}
