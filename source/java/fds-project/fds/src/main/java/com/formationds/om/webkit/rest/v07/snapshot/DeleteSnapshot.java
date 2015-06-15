/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v07.snapshot;

import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Map;

public class DeleteSnapshot
  implements RequestHandler {

  private static final Logger logger =
          LoggerFactory.getLogger( DeleteSnapshot.class );

  private static final String REQ_PARAM_SNAPSHOT_NAME = "snapshotName";

  private final Authorizer authorizer;
  private final ConfigurationApi config;
  private final AuthenticationToken token;

  public DeleteSnapshot( final Authorizer authorizer,
                         final ConfigurationApi config,
                         final AuthenticationToken token ) {
    this.authorizer = authorizer;
    this.config = config;
    this.token = token;
  }

  @Override
  public Resource handle( Request request, Map<String, String> routeParameters )
      throws Exception {

    final String snapshotName = requiredString( routeParameters,
                                                REQ_PARAM_SNAPSHOT_NAME );
    logger.trace( "DELETE SNAPSHOT:: NAME: {}", snapshotName );
//    authorizer.ownsSnapshot( token, snapshotName );
    /*
     * internally snapshots are the same as volumes.
     */
    config.deleteVolume( "", snapshotName );

    return new JsonResource( new JSONObject().put( "status", "OK" ) );
  }
}
