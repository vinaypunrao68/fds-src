/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.rest.snapshot;

import FDS_ProtocolInterface.ResourceState;
import com.formationds.commons.model.SnapshotPolicy;
import com.formationds.commons.model.builder.SnapshotPolicyBuilder;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.ConfigurationApi;
import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.InputStreamReader;
import java.io.Reader;
import java.util.Map;

public class EditSnapshotPolicy
  implements RequestHandler {
  private static final transient Logger logger =
    LoggerFactory.getLogger( EditSnapshotPolicy.class );

    private ConfigurationApi config;

    public EditSnapshotPolicy( ConfigurationApi config ) {
        this.config = config;
    }

    @Override
    public Resource handle(final Request request,
                           final Map<String, String> routeParameters)
            throws Exception {
      long policyId;
      try( final Reader reader =
            new InputStreamReader( request.getInputStream(), "UTF-8" ) ) {
        final SnapshotPolicy policy =
          ObjectModelHelper.toObject( reader,
                                      SnapshotPolicy.class );


        logger.trace( "EDIT::" + policy.toString() );

        final com.formationds.apis.SnapshotPolicy apisPolicy =
          new com.formationds.apis.SnapshotPolicy( policy.getId(),
                                                   policy.getName(),
                                                   policy.getRecurrenceRule().toString(),
                                                   policy.getRetention(),
                                                   ResourceState.Active );
        logger.trace( ObjectModelHelper.toJSON( apisPolicy ) );
        policyId = config.createSnapshotPolicy( apisPolicy );
      }

      final SnapshotPolicy updatedPolicy =
        new SnapshotPolicyBuilder().withId( policyId ).build();

      return new TextResource( updatedPolicy.toJSON() );
    }
}
