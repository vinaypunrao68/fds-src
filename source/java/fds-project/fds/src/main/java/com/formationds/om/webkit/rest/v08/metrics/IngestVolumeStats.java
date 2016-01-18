/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.metrics;

import com.formationds.client.v08.model.SizeUnit;
import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.redis.RedisSingleton;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.google.gson.reflect.TypeToken;

import org.apache.avro.generic.GenericData;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.InputStreamReader;
import java.io.Reader;
import java.lang.reflect.Type;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

/**
 * @author ptinius
 */
public class IngestVolumeStats
  implements RequestHandler {

  private static final Logger logger =
    LoggerFactory.getLogger( IngestVolumeStats.class );
	
  private static final Type TYPE = new TypeToken<List<VolumeDatapoint>>(){}.getType();

  private final ConfigurationApi config;

  public IngestVolumeStats(final ConfigurationApi config) {
    this.config = config;
  }

  @SuppressWarnings( "Duplicates" )
  @Override
  public Resource handle(Request request, Map<String, String> routeParameters)
      throws Exception {
    try (final Reader reader = new InputStreamReader(request.getInputStream(), "UTF-8")) {

      final List<IVolumeDatapoint> volumeDatapoints = ObjectModelHelper.toObject(reader, TYPE);

      final List<String> volumeNames = new ArrayList<>( );
      volumeDatapoints.forEach( vdp -> {
    	  long volid;
    	  
    	  try {
              if( !volumeNames.contains( vdp.getVolumeName() ) )
              {
                  volumeNames.add( vdp.getVolumeName() );
              }

    		  volid = SingletonConfigAPI.instance().api().getVolumeId( vdp.getVolumeName() );
    	  } catch (Exception e) {
    		  throw new IllegalStateException( "Volume does not have an ID associated with the name." );
    	  }
    	  
          vdp.setVolumeId( String.valueOf( volid ) );
      });

      if( volumeNames.isEmpty() )
      {
          // prevent divide by zero
          volumeNames.add( "DummyVolumeName" );
      }

      /**
       * HACK ALERT!!
       *
       *  The stat stream is per volume; we need to have used bytes (UBYTES) included with every
       *  volume, we divide the used capacity across all volumes so when we sum it later we should
       *  have a total used capacity be correct, well at least close.
       */
      final Double usedCapacity = RedisSingleton.INSTANCE.api( )
                                                         .getDomainUsedCapacity( )
                                                         .getValue( SizeUnit.B )
                                                         .doubleValue( ) / volumeNames.size();
      for( final IVolumeDatapoint vdp : volumeDatapoints )
      {
          if( vdp.getKey().equalsIgnoreCase( Metrics.LBYTES.key() ) )
          {
              volumeDatapoints.add( new VolumeDatapoint( vdp.getTimestamp(),
                                                         vdp.getVolumeId( ),
                                                         vdp.getVolumeName( ),
                                                         Metrics.UBYTES.key( ),
                                                          usedCapacity ) );
              break;
          }
      }

      SingletonRepositoryManager.instance().getMetricsRepository().save(volumeDatapoints);
    }

    return new JsonResource(new JSONObject().put("status", "OK"));
  }
}
