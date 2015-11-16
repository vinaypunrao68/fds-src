/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.metrics;

import com.formationds.client.v08.model.TimeUnit;
import com.formationds.client.v08.model.stats.ContextType;
import com.formationds.client.v08.model.stats.StatDataPoint;
import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.exception.UnsupportedMetricException;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.om.repository.helper.FirebreakHelper;
import com.formationds.stats.client.StatsConnection;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.google.gson.reflect.TypeToken;

import org.apache.commons.io.IOUtils;
import org.apache.curator.utils.EnsurePath;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.Reader;
import java.lang.reflect.Type;
import java.nio.CharBuffer;
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
      
      // get a connection to the stats service ready
      StatsConnection tempConn = null;
      
      try {
    	  logger.debug( "Attempting to connect to the stats service..." );
    	  tempConn = StatsConnection.newConnection( "localhost", 11011, "admin", "admin" );
      } catch( Exception e ){
    	  logger.warn( "Failed to connect to stats service.", e );
      }
      
      // this stupidity is so that it can be accessed in the lambda function.
      final StatsConnection statConn = tempConn;
      
      logger.debug( "Successfully connected to the stats service." );
      
      volumeDatapoints.forEach( vdp -> {
    	  long volid;
    	  
    	  try {
    		  volid = SingletonConfigAPI.instance().api().getVolumeId( vdp.getVolumeName() );
    	  } catch (Exception e) {
    		  throw new IllegalStateException( "Volume does not have an ID associated with the name." );
    	  }
    	  
          vdp.setVolumeId( String.valueOf( volid ) );    
          
          // store them in the stats database
          StatDataPoint datapoint = new StatDataPoint();
          
          String metricName = "UNKNOWN";
          
          try {
        	  Metrics metric = Metrics.lookup( vdp.getKey() );
        	  metricName = metric.name();
          }
          catch( UnsupportedMetricException ume ){
        	  logger.warn( "Could not locate metric for key.", ume );
          }
          
          datapoint.setMetricName( metricName );
          datapoint.setReportTime( vdp.getTimestamp() );
          datapoint.setMetricValue( vdp.getValue() );
          datapoint.setContextType( ContextType.VOLUME );
          datapoint.setContextId( volid );
          datapoint.setCollectionPeriod( 1L );
          datapoint.setCollectionTimeUnit( TimeUnit.MINUTES );
          
          try {
        	  
        	// do this test because if it's not, that means an exception has already been logged.
        	// no need to spam
        	if ( statConn != null && statConn.isOpen() ){
        		
				statConn.publishStatistic( datapoint );
				logger.debug( "Published stat for " + datapoint.getMetricName() + " to the stats service." );
        	}
			
		} catch (Exception e) {

			logger.warn( "Could not publish datapoint for: " + datapoint.getMetricName() + " to the stats service." );
			logger.warn( e.getLocalizedMessage() );
			logger.debug( "Error:", e );
		}
      });

      SingletonRepositoryManager.instance().getMetricsRepository().save(volumeDatapoints);
      
      logger.info( "Statistics were collected and published successfully." );
    }

    return new JsonResource(new JSONObject().put("status", "OK"));
  }
}
