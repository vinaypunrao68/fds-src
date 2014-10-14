/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.rest.metrics;

import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.repository.SingletonMetricsRepository;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.apache.commons.io.IOUtils;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Map;

/**
 * @author ptinius
 */
public class IngestVolumeStats
  implements RequestHandler {
  private static final Logger LOG =
    LoggerFactory.getLogger( IngestVolumeStats.class );

  public IngestVolumeStats() {
    super();
  }

  @Override
  public Resource handle( Request request, Map<String, String> routeParameters )
    throws Exception {
    LOG.info( "METADATA RECEIVED!" );
    JSONArray array = new JSONArray( IOUtils.toString( request.getInputStream() ) );
    int x = 0;
    for( int i = 0;
         i < array.length();
         i++ ) {
      SingletonMetricsRepository.instance()
                                .getMetricsRepository()
                                .save(
                                  ObjectModelHelper.toObject( array.getJSONObject( i )
                                                                   .toString(),
                                                              VolumeDatapoint.class ) );
      x = i;
    }
    LOG.info( "METADATA RECORDS PROCESSED: " + x );
    return new JsonResource( new JSONObject().put( "status", "OK" ) );
  }
}
