package com.formationds.om.webkit.rest.v08.metrics;

import java.io.IOException;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import org.json.JSONObject;

import com.formationds.stats_client.StatsConnection;
import com.formationds.stats_client.Subscription;
import com.formationds.stats_client.SubscriptionHandler;
import com.formationds.stats_client.model.StatDataPoint;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.google.common.cache.CacheBuilder;
import com.google.common.cache.CacheLoader;
import com.google.common.cache.LoadingCache;

public class StatsSocketHandler implements RequestHandler, SubscriptionHandler{

	private LoadingCache<Long, StatDataPoint> statCache;
	private Subscription subscription; 
	
	private static final String LAST_TIME = "lastTime";
	
	public StatsSocketHandler() {
		
		try {
			StatsConnection conn = StatsConnection.newConnection( "localhost", 4242, "guest", "guest" );
			
			subscription = conn.subscribe( "1.>", this );
			
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
		
		final Long lastTime = requiredLong( routeParameters, LAST_TIME );
		
		List<StatDataPoint> points = getStatCache().asMap().values().stream().filter( ( datapoint ) -> {
			
			if ( datapoint.getReportTime() > lastTime ){
				return true;
			}
			
			return false;
		}).collect( Collectors.toList() );
		
		JSONArray jArr = new JSONArray( points );
		JSONObject rtn = new JSONObject();
		rtn.put( "newPoints",  jArr );
		
		return new TextResource( rtn.toString() );
	}
	
	private LoadingCache<Long, StatDataPoint> getStatCache(){
		
		if ( statCache == null ){
			statCache = CacheBuilder.newBuilder()
					.maximumSize( 5000 )
					.expireAfterWrite( 5, TimeUnit.MINUTES )
					.build( new CacheLoader<Long, StatDataPoint>(){

						@Override
						public StatDataPoint load( Long  key ) throws Exception {
							// TODO Auto-generated method stub
							return null;
						}
						
					});
		}
		
		return statCache;
	}

	@Override
	public void messageReceived(StatDataPoint datapoint) {
		
		getStatCache().put( datapoint.getReportTime(), datapoint );
	}
	
	private Subscription getSubscription(){
		return subscription;
	}

}
