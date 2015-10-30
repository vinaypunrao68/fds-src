/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v07.snapshot;

import com.formationds.protocol.svc.types.ResourceState;
import com.formationds.commons.model.SnapshotPolicy;
import com.formationds.commons.model.builder.SnapshotPolicyBuilder;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.InputStreamReader;
import java.io.Reader;
import java.util.Map;

public class EditSnapshotPolicy
  implements RequestHandler {
  private static final Logger logger =
    LoggerFactory.getLogger( EditSnapshotPolicy.class );

    private com.formationds.util.thrift.ConfigurationApi config;

    public EditSnapshotPolicy(com.formationds.util.thrift.ConfigurationApi config) {
        this.config = config;
    }

    @Override
    public Resource handle(final Request request,
                           final Map<String, String> routeParameters)
        throws Exception {
        long policyId;
        try (final Reader reader =
                 new InputStreamReader(request.getInputStream(), "UTF-8")) {
            final SnapshotPolicy policy =
                ObjectModelHelper.toObject(reader,
                                           SnapshotPolicy.class);

            logger.trace("EDIT::" + policy.toString());

            final com.formationds.apis.SnapshotPolicy apisPolicy =
                new com.formationds.apis.SnapshotPolicy(policy.getId(),
                                                        policy.getName(),
                                                        policy.getRecurrenceRule().toString(),
                                                        policy.getRetention(),
                                                        ResourceState.Active,
                                                   policy.getTimelineTime() );
        logger.trace( ObjectModelHelper.toJSON( apisPolicy ) );
        policyId = config.createSnapshotPolicy( apisPolicy );
      }

      final SnapshotPolicy updatedPolicy =
        new SnapshotPolicyBuilder().withId( policyId ).build();

      return new TextResource( updatedPolicy.toJSON() );
    }
}
