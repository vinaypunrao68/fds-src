/**
 * Copyright (c) 2014 Formation Data Systems.
 * All rights reserved.
 */

package com.formationds.om.webkit.rest.snapshot;

import com.formationds.commons.model.SnapshotPolicy;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.ConfigurationApi;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.text.ParseException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class ListSnapshotPoliciesForVolume
  implements RequestHandler {
    private static final Logger logger =
        LoggerFactory.getLogger( ListSnapshotPoliciesForVolume.class );

    private static final String REQ_PARAM_VOLUME_ID = "volumeId";
    private ConfigurationApi config;

    public ListSnapshotPoliciesForVolume( final ConfigurationApi config ) {
        this.config = config;
    }

    @Override
    public Resource handle(final Request request, final Map<String, String> routeParameters)
            throws Exception {
        final long volumeId = requiredLong( routeParameters,
                                            REQ_PARAM_VOLUME_ID );

        final List<com.formationds.apis.SnapshotPolicy> _policies =
          config.listSnapshotPoliciesForVolume(volumeId);
        if( _policies == null || _policies.isEmpty() ) {
          return new JsonResource( new JSONArray( new ArrayList<>() ) );
        }

      final List<SnapshotPolicy> policies = new ArrayList<>();
    /*
     * process each thrift snapshot policy, and map it to the object model
     * version
     */
      _policies.stream()
               .forEach( ( p ) -> {
                 final SnapshotPolicy modelPolicy = new SnapshotPolicy();

                 modelPolicy.setId( p.getId() );
                 modelPolicy.setName( p.getPolicyName() );
                   try {
                       modelPolicy.setRecurrenceRule( p.getRecurrenceRule() );
                   } catch( ParseException e ) {
                       logger.error( "Failed to parse RRULE: " + p.getRecurrenceRule(),
                                     e);
                   }
                 modelPolicy.setRetention( p.getRetentionTimeSeconds() );
                 policies.add( modelPolicy );
               } );
      return new TextResource( ObjectModelHelper.toJSON( policies ) );
    }
}
