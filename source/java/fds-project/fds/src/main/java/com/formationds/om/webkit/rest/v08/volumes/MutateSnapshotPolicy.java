/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.volumes;

import java.io.InputStreamReader;
import java.util.List;
import java.util.Map;

import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.formationds.client.v08.converters.ExternalModelConverter;
import com.formationds.client.v08.model.SnapshotPolicy;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.protocol.ApiException;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

public class MutateSnapshotPolicy implements RequestHandler{

    private static final Logger logger = LoggerFactory.getLogger( MutateSnapshotPolicy.class );
    private static final String VOLUME_ARG = "volume_id";
    private static final String POLICY_ARG = "policy_id";

    private ConfigurationApi configApi;

    public MutateSnapshotPolicy(){}

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters)
            throws Exception {

        long volumeId = requiredLong( routeParameters, VOLUME_ARG );
        long policyId = requiredLong( routeParameters, POLICY_ARG );

        SnapshotPolicy policy = null;
        try (final InputStreamReader reader = new InputStreamReader( request.getInputStream() )) {
            policy = ObjectModelHelper.toObject( reader, SnapshotPolicy.class );
        }
        policy.setId( policyId );

        mutatePolicy( policy );

        List<SnapshotPolicy> policies = (new ListSnapshotPoliciesForVolume()).listSnapshotPoliciesForVolume( volumeId );
        SnapshotPolicy newPolicy = null;

        for ( SnapshotPolicy externalPolicy : policies ){
            if ( externalPolicy.getId().equals( policyId ) ){
                newPolicy = externalPolicy;
                break;
            }
        }

        String jsonString = ObjectModelHelper.toJSON( newPolicy );

        return new TextResource( jsonString );
    }

    public void mutatePolicy( SnapshotPolicy policy ) throws ApiException, TException {

        logger.debug( "Editing snapshot policy: {}." , policy.getId() );

        logger.trace( ObjectModelHelper.toJSON( policy ) );

        com.formationds.apis.SnapshotPolicy internalPolicy = ExternalModelConverter.convertToInternalSnapshotPolicy( policy );
        getConfigApi().createSnapshotPolicy( internalPolicy );
    }

    private ConfigurationApi getConfigApi(){

        if ( configApi == null ){
            configApi = SingletonConfigAPI.instance().api();
        }

        return configApi;
    }

}
