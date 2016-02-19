/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.domain;

import com.formationds.apis.LocalDomainDescriptor;
import com.formationds.client.v08.converters.ExternalModelConverter;
import com.formationds.client.v08.model.Domain;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.InputStreamReader;
import java.util.List;
import java.util.Map;

import javax.servlet.http.HttpServletResponse;

public class MutateLocalDomain implements RequestHandler {

    private static final String DOMAIN_ARG = "domain_id";

    private static final Logger logger =
            LoggerFactory.getLogger( MutateLocalDomain.class );

    private ConfigurationApi configApi;

    public MutateLocalDomain() {}

    @Override
    public Resource handle( Request request, Map<String, String> routeParameters )
            throws Exception {

        int domainId = requiredInt( routeParameters, DOMAIN_ARG );

        Domain domain = null;
        try ( final InputStreamReader reader = new InputStreamReader( request.getInputStream(), "UTF-8" ) ) {
            domain = ObjectModelHelper.toObject( reader, Domain.class );
        }
        domain.setId( domainId );

        switch( domain.getState() ){
            case DOWN:
                shutdownDomain( domain );
                break;
            case UP:
                startDomain( domain );
                break;
            default:
                throw new ApiException( "A domain state must be provided.", ErrorCode.BAD_REQUEST );
        }

        Domain newDomain = getDomain( domain.getId() );

        String jsonString = ObjectModelHelper.toJSON( newDomain );

        return new TextResource( jsonString );
    }

    public Domain getDomain( long domainId ) throws ApiException, TException{

        List<LocalDomainDescriptor> localDomains = getConfigApi().listLocalDomains( 0 );

        for ( LocalDomainDescriptor localDomain : localDomains ){

            if ( localDomain.getId() == domainId ){

                Domain domain = ExternalModelConverter.convertToExternalDomain( localDomain );
                return domain;
            }
        }

        throw new ApiException( "No domain found by ID: " + domainId, ErrorCode.MISSING_RESOURCE );
    }

    public void shutdownDomain( Domain domain ) throws ApiException, TException{

        logger.info( "Shutting down domain: " + domain.getName() );

        int status = getConfigApi().shutdownLocalDomain( domain.getName() );

        if( status != 0 )
        {
            status= HttpServletResponse.SC_BAD_REQUEST;

            throw new ApiException( "Unable to shutdown domain at this time, try again in a few minutes",
                                    ErrorCode.INTERNAL_SERVER_ERROR );
        }
    }

    public void startDomain( Domain domain ) throws ApiException, TException {

        logger.info( "Starting domain: " + domain.getName() );

        getConfigApi().startupLocalDomain( domain.getName() );
    }

    private ConfigurationApi getConfigApi(){

        if ( configApi == null ){
            configApi = SingletonConfigAPI.instance().api();
        }

        return configApi;
    }
}

