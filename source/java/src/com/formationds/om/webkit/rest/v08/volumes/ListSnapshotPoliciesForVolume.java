/**
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.volumes;

import com.formationds.client.v08.converters.ExternalModelConverter;
import com.formationds.client.v08.model.SnapshotPolicy;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.protocol.ApiException;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class ListSnapshotPoliciesForVolume implements RequestHandler {
    
	private static final Logger logger = LoggerFactory.getLogger( ListSnapshotPoliciesForVolume.class );

    private static final String REQ_PARAM_VOLUME_ID = "volume_id";
    private ConfigurationApi configApi;

    public ListSnapshotPoliciesForVolume() {}

    @Override
    public Resource handle(final Request request, final Map<String, String> routeParameters) throws Exception {
    	
        final long volumeId = requiredLong(routeParameters,
                                           REQ_PARAM_VOLUME_ID);

        final List<SnapshotPolicy> policies = listSnapshotPoliciesForVolume( volumeId );
        
        String jsonString = ObjectModelHelper.toJSON( policies );
        
        return new TextResource( jsonString );
    }
    
    public List<SnapshotPolicy> listSnapshotPoliciesForVolume( long volumeId ) throws ApiException, TException{
        
    	final List<com.formationds.apis.SnapshotPolicy> internalPolicies = getConfigApi().listSnapshotPoliciesForVolume( volumeId );

        final List<SnapshotPolicy> policies = new ArrayList<>();
		/*
		 * process each thrift snapshot policy, and map it to the object model
		 * version
		 */
        internalPolicies.stream().forEach( (internalPolicy) -> {
        	
        	SnapshotPolicy externalPolicy = ExternalModelConverter.convertToExternalSnapshotPolicy( internalPolicy );
        	policies.add( externalPolicy );
        });    	
        
        return policies;
    }
    
    private ConfigurationApi getConfigApi(){
    	
    	if ( configApi == null ){
    		configApi = SingletonConfigAPI.instance().api();
    	}
    	
    	return configApi;
    }
}
