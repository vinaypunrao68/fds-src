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
import com.formationds.xdi.ConfigurationApi;
import com.google.gson.reflect.TypeToken;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.io.InputStreamReader;
import java.io.Reader;
import java.lang.reflect.Type;
import java.util.List;
import java.util.Map;

/**
 * @author ptinius
 */
public class IngestVolumeStats
  implements RequestHandler {

  private static final Type TYPE =
    new TypeToken<List<VolumeDatapoint>>() {
    }.getType();

  private final ConfigurationApi config;

  public IngestVolumeStats( final ConfigurationApi config ) {
    this.config = config;
  }

  @Override
  public Resource handle( Request request, Map<String, String> routeParameters )
    throws Exception {
    try( final Reader reader =
           new InputStreamReader( request.getInputStream(), "UTF-8" ) ) {

      final List<VolumeDatapoint> volumeDatapoints =
        ObjectModelHelper.toObject( reader, TYPE );
      long timestamp = 0L;
      for( final VolumeDatapoint datapoint : volumeDatapoints ) {

        datapoint.setVolumeId(
          String.valueOf(
            config.getVolumeId( datapoint.getVolumeName() ) ) );

        if( timestamp <= 0L ) {
          timestamp = datapoint.getTimestamp();
        }

        /*
         * syncing all the times to the first record. This will allow for us
         * to query for a time range and guarantee that all datapoints will
         * be aligned
         */
        datapoint.setTimestamp( timestamp );

        SingletonMetricsRepository.instance()
                                  .getMetricsRepository()
                                  .save( datapoint );
      }
    }

    return new JsonResource( new JSONObject().put( "status", "OK" ) );
  }
}
