/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.metrics;

import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.google.gson.reflect.TypeToken;

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

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

  private static final Logger logger =
    LoggerFactory.getLogger( IngestVolumeStats.class );
	
  private static final Type TYPE =
    new TypeToken<List<VolumeDatapoint>>() {
    }.getType();

  private final ConfigurationApi config;

  public IngestVolumeStats(final ConfigurationApi config) {
    this.config = config;
  }

  @Override
  public Resource handle(Request request, Map<String, String> routeParameters)
      throws Exception {
    try (final Reader reader =
             new InputStreamReader(request.getInputStream(), "UTF-8")) {

      final List<IVolumeDatapoint> volumeDatapoints = ObjectModelHelper.toObject(reader, TYPE);
      
      volumeDatapoints.forEach( vdp -> {
    	  long volid;
    	  
    	  try {
    		  volid = SingletonConfigAPI.instance().api().getVolumeId( vdp.getVolumeName() );
    	  } catch (Exception e) {
    		  throw new IllegalStateException( "Volume does not have an ID associated with the name." );
    	  }
    	  
          vdp.setVolumeId( String.valueOf( volid ) );    	  
      });

      SingletonRepositoryManager.instance().getMetricsRepository().save(volumeDatapoints);
    }

    return new JsonResource(new JSONObject().put("status", "OK"));
  }
}
