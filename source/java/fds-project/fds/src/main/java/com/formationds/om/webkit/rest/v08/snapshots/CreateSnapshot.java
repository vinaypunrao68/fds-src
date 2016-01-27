/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.snapshots;

import com.formationds.client.v08.model.Snapshot;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.InputStreamReader;
import java.util.List;
import java.util.Map;

public class CreateSnapshot implements RequestHandler {

    private static final Logger logger = LoggerFactory.getLogger( CreateSnapshot.class );
    private static final String VOLUME_ARG = "volume_id";

    private ConfigurationApi configApi;

    public CreateSnapshot() {}

    @Override
    public Resource handle(final Request request,
                           final Map<String, String> routeParameters) throws Exception {

        long volumeId = requiredLong( routeParameters, VOLUME_ARG );

        logger.debug( "Create a snapshot for volume {}.", volumeId );

        Snapshot inputSnapshot = null;
        try ( final InputStreamReader reader = new InputStreamReader( request.getInputStream() ) ) {
            inputSnapshot = ObjectModelHelper.toObject( reader, Snapshot.class );
        }

        getConfigApi().createSnapshot( volumeId,
                                       inputSnapshot.getName(),
                                       inputSnapshot.getRetention().getSeconds(),
                                       0 );

        List<Snapshot> snapshots = (new ListSnapshots()).listSnapshots( volumeId );
        Snapshot mySnapshot = null;

        for ( Snapshot snapshot : snapshots ){
            if ( snapshot.getVolumeId().equals( volumeId ) &&
                    snapshot.getName().equals( inputSnapshot.getName() ) ){
                mySnapshot = snapshot;
                break;
            }
        }

        String jsonString = ObjectModelHelper.toJSON( mySnapshot );

        return new TextResource( jsonString );
    }

    private ConfigurationApi getConfigApi(){

        if ( configApi == null ){
            configApi = SingletonConfigAPI.instance().api();
        }

        return configApi;
    }
}
